#ifndef ESP_UART_H
#define ESP_UART_H

#include <stdint.h>
#include "stm32f4xx_hal.h"    // HAL types and functions
#include "app/machine interface.h"


extern UART_HandleTypeDef huart4; //uart for the rectangle esp32

//struct 

//data sent to ESP32 via UART
typedef struct __attribute__((packed)){
    //the data that is sent to the ESP32 via UART
    uint8_t startByte; //start byte to indicate beginning of data packet
    uint32_t platterSpeed; //units TBD
    //uint8_t frameCount; //how many frames are on the platter
    //uint32_t frameOnTime; //time the frame is ON in microseconds
} ESPTX_instance;



//PFP
void send_packet(system_interface *sys);
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart);
void buildTestPacket(system_interface *sys);

HAL_StatusTypeDef packet_send(system_interface *sys, uint8_t type, uint16_t len);

#endif