#include "uart_protocol.h"
#include "ui_highlight.h"

#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <string.h>

//static const char *TAG = "UART_RX";

system_rx_data_t g_system_rx;

/* ---------- Internal ---------- */

static void handle_test_payload(uint8_t *payload)
{
    g_system_rx.initialized   = payload[0];              //0 - system initiallization state
    g_system_rx.system_state  = payload[1];              //1 - system state

    g_system_rx.platterRotationPeriod_ms =               //2-5 - platter rotation period
          ((uint32_t)payload[2])
        | ((uint32_t)payload[3] << 8)
        | ((uint32_t)payload[4] << 16)
        | ((uint32_t)payload[5] << 24);

    g_system_rx.sliceCount          = payload[6];       //6 - slice count
    g_system_rx.motorSpeedPot =                         //7-8 - motor speed pot
          ((uint32_t)payload[7])
        | ((uint32_t)payload[8] << 8);
    g_system_rx.strobeEnabled       = payload[9];       //9 - strobe enabled
    g_system_rx.motorEnabled        = payload[10];      //10 - motor enabled
    g_system_rx.motorDirection      = payload[11];      //11 - motor direction
    g_system_rx.verticalHighlight   = payload[12];      //12 - vertical highlight
    g_system_rx.horizontalHighlight = payload[13];      //13 - horizontal highlight
    g_system_rx.highlightFlag       = payload[14];      //14 - highlight flag
    g_system_rx.selectFlag          = payload[15];      //15 - select flag

    //newly added
    g_system_rx.strobeEnabledUART = payload[16];        //16 - strobe enabled UART
    g_system_rx.temperature1 =                          //17-18 - temperature 1
          ((uint16_t)payload[17])
        | ((uint16_t)payload[18] << 8);
    g_system_rx.temperature2 =                          //19-20 - temperature 2
          ((uint16_t)payload[19])
        | ((uint16_t)payload[20] << 8);
    g_system_rx.lightSensor1 =                          //21-22 - light sensor 1
          ((uint16_t)payload[21])
        | ((uint16_t)payload[22] << 8);
    g_system_rx.lightSensor2 =                          //23-24 - light sensor 2
          ((uint16_t)payload[23])
        | ((uint16_t)payload[24] << 8);
    g_system_rx.strobeMode            = payload[25];    //25 - strobe mode
    g_system_rx.motorMode             = payload[26];    //26 - motor mode
    g_system_rx.stepperMotorStepConfig= payload[27];    //27 - stepper motor step config
    g_system_rx.stepperMotorStealthChop= payload[28];   //28 - stepper motor stealthchop
    g_system_rx.platterInstalled      = payload[29];    //29 - platter installed
    g_system_rx.heartbeatPulseTime =                    //30-33 - heartbeat pulse time
          ((uint32_t)payload[30])
        | ((uint32_t)payload[31] << 8)
        | ((uint32_t)payload[32] << 16)
        | ((uint32_t)payload[33] << 24);
    g_system_rx.fan1State            = payload[34];     //34 - fan 1 state
    g_system_rx.fan2State            = payload[35];     //35 - fan 2 state

    //new - adding frame on time (CCR value) from STM32
    g_system_rx.strobeCCRValue =                        //36-37 - strobe CCR value
          ((uint16_t)payload[36])
        | ((uint16_t)payload[37] << 8);

    //next I will add the below into the data flow
    //uint32_t rotation period
    //uint8_t strobe enabled
    //uint16_t temperature 1
    //uint16_t temperature 2
    //uint16_t light sensor 1
    //uint16_t light sensor 2
    //uint8_t strobe mode
    //uint8_t motor mode
    //uint8_t stepper motor step config
    //uint8_t stepper motor stealthchop
    //uint8_t platter installed
    //uint32_t heartbeat pulse time
    //uint8_t fan 1 state
    //uint8_t fan 2 state

    //after we have received an updated set of variables, we can update the screen highlighting
    ui_highlight_update();
}

static void uart_rx_task(void *arg)
{
    uint8_t byte;
    uint8_t msg_type;
    uint8_t payload_len;
    uint8_t crc;
    uint8_t payload[UART_MAX_PAYLOAD_LEN];

    while (1)
    {
        /* 1. Wait for SOF */
        uart_read_bytes(UART_PORT, &byte, 1, portMAX_DELAY);
        if (byte != UART_SOF_BYTE)
            continue;

        /* 2. Read header */
        uart_read_bytes(UART_PORT, &msg_type, 1, portMAX_DELAY);
        uart_read_bytes(UART_PORT, &payload_len, 1, portMAX_DELAY);
        uart_read_bytes(UART_PORT, &crc, 1, portMAX_DELAY);  // CRC ignored for now

        /* 3. Validate payload length */
        if (payload_len > UART_MAX_PAYLOAD_LEN)
            continue;

        /* 4. Read payload */
        uart_read_bytes(UART_PORT, payload, payload_len, portMAX_DELAY);

        /* 5. Process test payload */
        if (msg_type == UART_MSG_TYPE_TEST && payload_len == 16){
            //handle_test_payload(payload);
        }
        handle_test_payload(payload); //always process payload? - test
    }
}

/* ---------- Public ---------- */

void uart_rx_init(void)
{
    uart_config_t cfg = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    uart_driver_install(UART_PORT, UART_BUF_SIZE, 0, 0, NULL, 0);
    uart_param_config(UART_PORT, &cfg);
    uart_set_pin(UART_PORT, UART_TX_PIN, UART_RX_PIN,
                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    memset(&g_system_rx, 0, sizeof(g_system_rx));

    xTaskCreate(uart_rx_task,
                "uart_rx_task",
                4096,
                NULL,
                12,
                NULL);
}
