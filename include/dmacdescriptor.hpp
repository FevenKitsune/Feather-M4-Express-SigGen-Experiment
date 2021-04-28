#ifndef DMACDESCRIPTOR_H
#define DMACDESCRIPTOR_H

#include <Arduino.h>

// https://forum.arduino.cc/t/samd51-dac-using-dma-seems-too-fast/678418/4
typedef struct // DMAC descriptor structure
{
    uint16_t btctrl;
    uint16_t btcnt;
    uint32_t srcaddr;
    uint32_t dstaddr;
    uint32_t descaddr;
} dmacdescriptor;

#endif