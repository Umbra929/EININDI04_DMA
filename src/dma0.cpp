#include <iikit.h>
#include <util/AdcDmaEsp.h>

// Instância global
AdcDmaEsp adcDma;

// Buffer para leitura no loop
static int16_t readBuffer[256];

void setup() {
    IIKit.setup();
    delay(1000);
    wserial::println();
    wserial::println("=== Teste ADC DMA ESP32 ===");
    // Exemplo: ADC1_CHANNEL_1 → GPIO39
    //          ADC1_CHANNEL_0 → GPIO36
    if (!adcDma.begin(ADC1_CHANNEL_1, 1000)) {  // 1 kHz
        wserial::println("Falha ao iniciar ADC DMA");
        while (true) {
            delay(1000);
        }
    }
    wserial::println("ADC DMA iniciado.");
}

void loop() {
    IIKit.loop();
    // 1) Coleta o que o DMA já produziu
    adcDma.poll();
    // 2) De tempos em tempos, leia tudo o que chegou
    static uint32_t lastPrint = 0;
    uint32_t now = millis();
    if (now - lastPrint >= 100) {  // a cada 100 ms
        lastPrint = now;
        size_t n = adcDma.read(readBuffer, 256);
        if (n > 0) {
            wserial::plot("adcValue", 10, readBuffer, n);
        }
    }
}
