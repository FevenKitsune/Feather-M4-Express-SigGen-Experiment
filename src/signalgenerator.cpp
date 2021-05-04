#include "signalgenerator.hpp"
#define TC_CHECK (dac == DAC0 ? TC0 : TC1)
#define DACN_CHECK (dac == DAC0 ? 0 : 1)
#define OVF_CHECK (dac == DAC0 ? TC0_DMAC_ID_OVF : TC1_DMAC_ID_OVF)

SignalGenerator::SignalGenerator(uint8_t desired_dmac_channel, uint8_t desired_dac)
{
    dac = desired_dac;
    dmac_channel = desired_dmac_channel;
    buffer = new uint16_t[BUFFER_SIZE];
}

int16_t SignalGenerator::populateBuffer(uint32_t frequency)
{
    float value;
    buffer_utilization = (1000000 / frequency > BUFFER_SIZE) ? BUFFER_SIZE : 1000000 / frequency;
    for (uint16_t index = 0; index < buffer_utilization; index++)
    {
        // buffer[index] = round((4095.0 / 2.0) * sin((2.0 * PI * 1.0 * float(i)) / sample_count) + (4095.0 / 2.0));
        value = 2047.5 * (sin((6.2831853071795864769252867665590 * (float)index) / buffer_utilization) + 1); // Because round() is a preprocessor directive, this method will only compute the value once.
        buffer[index] = round(value);                                                                        // TODO: Check if this actually makes a difference.
    }
    return ERR_NONE;
}
int16_t SignalGenerator::initializeDMAC()
{
    populateBuffer(INITIAL_FREQUENCY);
    DMAC->BASEADDR.reg = (uint32_t)descriptor_section;           // Specify the location of the descriptors
    DMAC->WRBADDR.reg = (uint32_t)wrb;                           // Specify the location of the write back descriptors
    DMAC->CTRL.reg = DMAC_CTRL_DMAENABLE | DMAC_CTRL_LVLEN(0xf); // Enable the DMAC peripheral
    analogWriteResolution(12);                                   // Set the DAC's resolution to 12-bits
    analogWrite(dac, 0);                                         // Initialise DACn

    DMAC->Channel[dmac_channel].CHCTRLA.reg = DMAC_CHCTRLA_TRIGSRC(OVF_CHECK) |             // Set DMAC to trigger when TCn timer overflows
                                              DMAC_CHCTRLA_TRIGACT_BURST;                   // DMAC burst transfer
    descriptor.descaddr = (uint32_t)&descriptor_section[dmac_channel];                      // Set up a circular descriptor
    descriptor.srcaddr = (uint32_t)&buffer[0] + buffer_utilization * sizeof(uint16_t);      // Read the current value in the sine table
    descriptor.dstaddr = (uint32_t)&DAC->DATA[DACN_CHECK].reg;                              // Copy it into the DAC data register
    descriptor.btcnt = buffer_utilization;                                                  // This takes the number of sine table entries.
    descriptor.btctrl = DMAC_BTCTRL_BEATSIZE_HWORD |                                        // Set the beat size to 16-bits (Half Word)
                        DMAC_BTCTRL_SRCINC |                                                // Increment the source address every beat
                        DMAC_BTCTRL_VALID;                                                  // Flag the descriptor as valid
    memcpy((void *)&descriptor_section[dmac_channel], &descriptor, sizeof(dmacdescriptor)); // Copy to the channel #channel descriptor

    GCLK->PCHCTRL[(dac ? TC1_GCLK_ID : TC0_GCLK_ID)].reg = GCLK_PCHCTRL_CHEN |     // Enable perhipheral channel for TCn
                                                           GCLK_PCHCTRL_GEN_GCLK1; // Connect generic clock 1 at 48MHz

    TC_CHECK->COUNT16.WAVE.reg = TC_WAVE_WAVEGEN_MFRQ; // Set TC0 to Match Frequency (MFRQ) mode
    TC_CHECK->COUNT16.CC[0].reg = 47;                  // Set the sine wave frequency to 1kHz: (48MHz / (buffer_utilization * freq)) - 1. 47 seems to be the magic number.
    while (TC_CHECK->COUNT16.SYNCBUSY.bit.CC0)
        ; // Wait for synchronization

    TC_CHECK->COUNT16.CTRLA.bit.ENABLE = 1; // Enable the TCn timer
    while (TC_CHECK->COUNT16.SYNCBUSY.bit.ENABLE)
        ; // Wait for synchronization

    DMAC->Channel[dmac_channel].CHCTRLA.bit.ENABLE = 1; // Enable DMAC on channel #channel
    return ERR_NONE;
}

int16_t SignalGenerator::refreshDMAC()
{
    TC_CHECK->COUNT16.CTRLA.bit.ENABLE = 0; // Disable the TCn timer
    while (!TC_CHECK->COUNT16.SYNCBUSY.bit.ENABLE)
        ;                                                                                   // Wait for synchronization
    DMAC->Channel[dmac_channel].CHCTRLA.bit.ENABLE = 0;                                     // Disable DMAC on channel #channel
    descriptor.descaddr = (uint32_t)&descriptor_section[dmac_channel];                      // Set up a circular descriptor
    descriptor.srcaddr = (uint32_t)&buffer[0] + buffer_utilization * sizeof(uint16_t);      // Update the source address of the sine table.
    descriptor.dstaddr = (uint32_t)&DAC->DATA[DACN_CHECK].reg;                              // Copy it into the DAC data register
    descriptor.btcnt = buffer_utilization;                                                  // Update the beat-count to match the new sample-count.
    memcpy((void *)&descriptor_section[dmac_channel], &descriptor, sizeof(dmacdescriptor)); // Copy updated dmacdescriptor to the channel #channel descriptor
    TC_CHECK->COUNT16.CTRLA.bit.ENABLE = 1;                                                 // Enable the TCn timer
    while (TC_CHECK->COUNT16.SYNCBUSY.bit.ENABLE)
        ;                                               // Wait for synchronization
    DMAC->Channel[dmac_channel].CHCTRLA.bit.ENABLE = 1; // Enable DMAC on channel #channel
    return ERR_NONE;
}
