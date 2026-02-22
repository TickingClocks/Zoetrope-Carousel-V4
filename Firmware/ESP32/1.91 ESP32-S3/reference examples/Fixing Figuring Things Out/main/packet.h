#pragma once
#include <stdint.h>

typedef struct __attribute__((packed))
{
    uint32_t period_ms;
} uart_packet_t;


//#define PACKET_START_BYTE  0xAA
