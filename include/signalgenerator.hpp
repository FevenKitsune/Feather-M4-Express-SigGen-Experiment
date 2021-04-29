#ifndef SIGNALGENERATOR_H
#define SIGNALGENERATOR_H

#include <Arduino.h>
#include "errorcodes.hpp"
#include "dmacdescriptor.hpp"

#define SIGNAL_GENERATOR_FREQUENCY_MINIMUM_HZ 125   // Minimum frequency that produces acceptible results.
#define SIGNAL_GENERATOR_FREQUENCY_MAXIMUM_HZ 36000 // Maximum frequency that produces acceptible results.
#define BUFFER_SIZE 8192                            // Waveform loopup table buffer size.
#define INITIAL_FREQUENCY 1000                      // Default frequency to initialize with.

class SignalGenerator
{
private:
    uint8_t channel;
    volatile uint32_t buffer_utilization;
    volatile uint16_t *buffer;
    volatile dmacdescriptor wrb[DMAC_CH_NUM] __attribute__((aligned(16)));       // Write-back DMAC descriptors
    dmacdescriptor descriptor_section[DMAC_CH_NUM] __attribute__((aligned(16))); // DMAC channel descriptors
    dmacdescriptor descriptor __attribute__((aligned(16)));                      // Place holder descriptor

public:
    SignalGenerator(uint8_t channel);
    int16_t populateBuffer(uint32_t frequency);
    int16_t initializeDMAC();
    int16_t refreshDMAC();
};

#endif