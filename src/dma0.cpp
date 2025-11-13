#include <iikit.h>
#include "SimpleADC_DMA.h"

SimpleADC_DMA dma;

void onSamples(uint16_t* samples, size_t count) {
    // Exemplo: imprime só a primeira amostra de cada bloco
    IIKit.WSerial.print("Bloco com ");
    IIKit.WSerial.print(count);
    IIKit.WSerial.print(" amostras. Primeira = ");
    IIKit.WSerial.println(samples[0]);
}

void setup() {
    IIKit.setup();
    if (!dma.begin(ADC1_CHANNEL_6, 
                   1000,  //1 kHz 
                  1000  //1000 amostras → ~1 callbacks/s
                   )
        ) IIKit.WSerial.println("Falha ao iniciar SimpleADC_DMA");
    else dma.onData(onSamples);
}

void loop() {
    IIKit.loop();
}
