#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "driver/uart.h"

/* UART config */
#define UART_PORT      UART_NUM_1
#define UART_TX_PIN    43
#define UART_RX_PIN    44
#define UART_BUF_SIZE  256

#define UART_SOF_BYTE        0xAA
#define UART_MSG_TYPE_TEST   0x10
#define UART_MAX_PAYLOAD_LEN 64

/* ================= DATA STRUCT ================= */

typedef struct
{
    uint8_t  initialized;
    uint8_t  system_state;
    uint32_t platterRotationPeriod_ms;
    uint8_t  sliceCount;
    uint16_t motorSpeedPot; //just added (raw adc value?)
    uint8_t  strobeEnabled;
    uint8_t  motorEnabled;
    uint8_t  motorDirection;
    uint8_t  verticalHighlight;
    uint8_t  horizontalHighlight;
    uint8_t  highlightFlag;
    uint8_t  selectFlag;

    //newly added
    uint8_t  strobeEnabledUART;
    uint16_t temperature1;
    uint16_t temperature2;
    uint16_t lightSensor1;
    uint16_t lightSensor2;
    uint8_t  strobeMode;
    uint8_t  motorMode;
    uint8_t  stepperMotorStepConfig;
    uint8_t  stepperMotorStealthChop;
    uint8_t  platterInstalled;
    uint32_t heartbeatPulseTime;
    uint8_t  fan1State;
    uint8_t  fan2State;

} system_rx_data_t;

/* Global RX data */
extern system_rx_data_t g_system_rx;

/* Initialize UART RX task */
void uart_rx_init(void);
