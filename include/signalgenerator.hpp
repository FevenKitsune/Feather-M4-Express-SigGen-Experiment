#ifndef SIGNALGENERATOR_H
#define SIGNALGENERATOR_H

#include <Arduino.h>
#include "errorcodes.hpp"

#define SIGNAL_GENERATOR_FREQUENCY_MINIMUM_HZ 125   // Minimum frequency that produces acceptible results.
#define SIGNAL_GENERATOR_FREQUENCY_MAXIMUM_HZ 36000 // Maximum frequency that produces acceptible results.
#define BUFFER_SIZE 8192                            // Waveform loopup table buffer size.
#define INITIAL_FREQUENCY 1000                      // Default frequency to initialize with.

volatile uint32_t buffer_utilization;
volatile extern uint16_t buffer[BUFFER_SIZE];

// https://forum.arduino.cc/t/samd51-dac-using-dma-seems-too-fast/678418/4
typedef struct // DMAC descriptor structure
{
    uint16_t btctrl;
    uint16_t btcnt;
    uint32_t srcaddr;
    uint32_t dstaddr;
    uint32_t descaddr;
} dmacdescriptor;

volatile dmacdescriptor wrb[DMAC_CH_NUM] __attribute__((aligned(16)));       // Write-back DMAC descriptors
dmacdescriptor descriptor_section[DMAC_CH_NUM] __attribute__((aligned(16))); // DMAC channel descriptors
dmacdescriptor descriptor __attribute__((aligned(16)));                      // Place holder descriptor

int16_t populateBuffer(uint32_t frequency);
int16_t initializeDMAC();
int16_t refreshDMAC();

#endif