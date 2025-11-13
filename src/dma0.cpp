#include <iikit.h>
#include "SimpleADC_DMA.h"

SimpleADC_DMA dma;

void osciloscope(uint16_t* samples, size_t count) {
   IIKit.WSerial.plot("adcValue", (uint32_t)10, samples, count);
}

void setup() {
    IIKit.setup();
    if (!dma.begin(ADC1_CHANNEL_6, 
                   1000,  //1 kHz 
                  1000  //1000 amostras → ~1 callbacks/s
                   )
        ) IIKit.WSerial.println("Falha ao iniciar SimpleADC_DMA");
    else dma.onData(osciloscope);
}

void loop() {
    IIKit.loop();
}
