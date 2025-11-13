#pragma once

#include <Arduino.h>
#include "driver/i2s.h"

class SimpleADC_DMA {
public:
    typedef void (*DataCallback)(uint16_t* samples, size_t count);

    SimpleADC_DMA()
        : _adc_channel(ADC1_CHANNEL_0),
          _port(I2S_NUM_0),
          _sample_rate(1000),
          _dma_block_len(256),
          _task(nullptr),
          _cb(nullptr),
          _running(false) {}

    bool begin(adc1_channel_t adc_channel,
            int sample_rate,
            size_t dma_block_len,
            i2s_port_t port = I2S_NUM_0)
    {
        _adc_channel   = adc_channel;
        _sample_rate   = sample_rate;
        _dma_block_len = dma_block_len;
        _port          = port;

        if (_dma_block_len == 0 || _sample_rate <= 0) {
            return false;
        }

        // --- Configura ADC1 ---
        adc1_config_width(ADC_WIDTH_BIT_12);
        esp_err_t err = adc1_config_channel_atten(_adc_channel, ADC_ATTEN_DB_12);
        if (err != ESP_OK) {
            return false;
        }

        // --- Configura I2S em modo ADC interno ---
        i2s_config_t i2s_config = {};
        i2s_config.mode = (i2s_mode_t)(
            I2S_MODE_MASTER |
            I2S_MODE_RX |              // <- ESSENCIAL PARA ADC + DMA
            I2S_MODE_ADC_BUILT_IN
        );
        i2s_config.sample_rate = _sample_rate;
        i2s_config.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
        i2s_config.channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT;
        i2s_config.communication_format = I2S_COMM_FORMAT_STAND_I2S;
        i2s_config.dma_buf_count = 4;
        i2s_config.dma_buf_len = _dma_block_len;
        i2s_config.use_apll = false;

        err = i2s_driver_install(_port, &i2s_config, 0, NULL);
        if (err != ESP_OK) {
            return false;
        }

        err = i2s_set_adc_mode(ADC_UNIT_1, _adc_channel);
        if (err != ESP_OK) {
            i2s_driver_uninstall(_port);
            return false;
        }

        i2s_adc_enable(_port);

        _running = true;

        BaseType_t res = xTaskCreatePinnedToCore(
            taskThunk,
            "SimpleADC_DMA_Task",
            4096,
            this,
            1,
            &_task,
            0  // core 0
        );

        if (res != pdPASS) {
            _running = false;
            i2s_adc_disable(_port);
            i2s_driver_uninstall(_port);
            return false;
        }

        return true;
    }

    void onData(DataCallback cb) {
        _cb = cb;
    }

    void stop() {
        if (!_running) return;
        _running = false;

        // espere a task finalizar loop e se auto-deletar
        delay(50);

        i2s_adc_disable(_port);
        i2s_driver_uninstall(_port);
    }

private:
    adc1_channel_t _adc_channel;
    i2s_port_t     _port;
    int            _sample_rate;
    size_t         _dma_block_len;

    TaskHandle_t   _task;
    DataCallback   _cb;
    volatile bool  _running;

    static void taskThunk(void* arg) {
        reinterpret_cast<SimpleADC_DMA*>(arg)->taskLoop();
    }

    void taskLoop() {
        const size_t bytes = _dma_block_len * sizeof(uint16_t);
        uint16_t* buf = (uint16_t*)malloc(bytes);
        if (!buf) {
            _running = false;
            vTaskDelete(nullptr);
            return;
        }

        while (_running) {
            size_t readBytes = 0;

            esp_err_t res = i2s_read(
                _port,
                buf,
                bytes,
                &readBytes,
                portMAX_DELAY
            );

            if (res != ESP_OK) {
                // só tenta de novo
                continue;
            }

            if (readBytes > 0 && _cb) {
                size_t samples = readBytes / sizeof(uint16_t);
                _cb(buf, samples);
            }
        }

        free(buf);
        vTaskDelete(nullptr);
    }
};