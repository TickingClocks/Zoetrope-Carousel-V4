#ifndef WS2812_H
#define WS2812_H

#include <stdint.h>
#include "stm32f4xx_hal.h"    // HAL types and functions

extern TIM_HandleTypeDef htim2;

//////////////////
////NEOPIXELS////
////////////////
#define neopixel_count 4

void neopixel_update(void);
void neopixel_setAllLEDColor(uint8_t red, uint8_t green, uint8_t blue);
void neopixel_setLEDColor(uint8_t LEDnum, uint8_t Red, uint8_t Green, uint8_t Blue);
void WS2812_PulseFinishedCallback(TIM_HandleTypeDef *htim);

#endif