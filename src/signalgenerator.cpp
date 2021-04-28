#include "signalgenerator.hpp"

int16_t populateBuffer(uint32_t frequency)
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
int16_t initializeDMAC()
{
    populateBuffer(INITIAL_FREQUENCY);
    DMAC->BASEADDR.reg = (uint32_t)descriptor_section;           // Specify the location of the descriptors
    DMAC->WRBADDR.reg = (uint32_t)wrb;                           // Specify the location of the write back descriptors
    DMAC->CTRL.reg = DMAC_CTRL_DMAENABLE | DMAC_CTRL_LVLEN(0xf); // Enable the DMAC peripheral
    analogWriteResolution(12);                                   // Set the DAC's resolution to 12-bits
    analogWrite(A0, 0);                                          // Initialise DAC0

    DMAC->Channel[5].CHCTRLA.reg = DMAC_CHCTRLA_TRIGSRC(TC0_DMAC_ID_OVF) |             // Set DMAC to trigger when TC0 timer overflows
                                   DMAC_CHCTRLA_TRIGACT_BURST;                         // DMAC burst transfer
    descriptor.descaddr = (uint32_t)&descriptor_section[5];                            // Set up a circular descriptor
    descriptor.srcaddr = (uint32_t)&buffer[0] + buffer_utilization * sizeof(uint16_t); // Read the current value in the sine table
    descriptor.dstaddr = (uint32_t)&DAC->DATA[0].reg;                                  // Copy it into the DAC data register
    descriptor.btcnt = buffer_utilization;                                             // This takes the number of sine table entries = 1000 beats
    descriptor.btctrl = DMAC_BTCTRL_BEATSIZE_HWORD |                                   // Set the beat size to 16-bits (Half Word)
                        DMAC_BTCTRL_SRCINC |                                           // Increment the source address every beat
                        DMAC_BTCTRL_VALID;                                             // Flag the descriptor as valid
    memcpy((void *)&descriptor_section[5], &descriptor, sizeof(dmacdescriptor));       // Copy to the channel 5 descriptor

    GCLK->PCHCTRL[TC0_GCLK_ID].reg = GCLK_PCHCTRL_CHEN |     // Enable perhipheral channel for TC0
                                     GCLK_PCHCTRL_GEN_GCLK1; // Connect generic clock 1 at 48MHz

    TC0->COUNT16.WAVE.reg = TC_WAVE_WAVEGEN_MFRQ; // Set TC0 to Match Frequency (MFRQ) mode
    TC0->COUNT16.CC[0].reg = 47;                  // Set the sine wave frequency to 1kHz: (48MHz / (buffer_utilization * freq)) - 1. 47 seems to be the magic number.
    while (TC0->COUNT16.SYNCBUSY.bit.CC0)
        ; // Wait for synchronization

    TC0->COUNT16.CTRLA.bit.ENABLE = 1; // Enable the TC0 timer
    while (TC0->COUNT16.SYNCBUSY.bit.ENABLE)
        ; // Wait for synchronization

    DMAC->Channel[5].CHCTRLA.bit.ENABLE = 1; // Enable DMAC on channel 5
    return ERR_NONE;
}

int16_t refreshDMAC()
{
    DMAC->Channel[5].CHCTRLA.bit.ENABLE = 0;                                           // Disable DMAC on channel 5
    descriptor.srcaddr = (uint32_t)&buffer[0] + buffer_utilization * sizeof(uint16_t); // Update the source address of the sine table.
    descriptor.btcnt = buffer_utilization;                                             // Update the beat-count to match the new sample-count.
    memcpy((void *)&descriptor_section[5], &descriptor, sizeof(dmacdescriptor));       // Copy updated dmacdescriptor to the channel 5 descriptor
    DMAC->Channel[5].CHCTRLA.bit.ENABLE = 1;                                           // Enable DMAC on channel 5
    return ERR_NONE;
}