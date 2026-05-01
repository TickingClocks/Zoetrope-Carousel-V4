#include "drivers/WS2812.h"
#include "stm32f4xx_hal.h"   
#include "stm32f4xx_hal_def.h"
#include <stdint.h>

uint8_t LED_Data[neopixel_count][4]; //RGB color for each LED 8 bit per color, index, red, green, blue
uint32_t pwmData[(24*neopixel_count)+100]; //contains the PWM data to send to neopixels including the end frame
uint16_t CCR_HIGH_val = 56; //79 56 CCR value for HIGH (1) transmission
uint16_t CCR_LOW_val = 32; //34 32 CCR value for LOW (0) transmission
uint8_t datasentflag = 1; //flag for when the DMA is done sending PWM data (currently the code blocks while it waits for the data to be send)

//////////////
///NEOPIXELS///
//////////////

HAL_StatusTypeDef HAL_Status_PWM = HAL_OK;

//set the color of a single LED in the color array
void neopixel_setLEDColor(uint8_t LEDnum, uint8_t Red, uint8_t Green, uint8_t Blue){
	LED_Data[LEDnum][0] = LEDnum;
	LED_Data[LEDnum][1] = Green;
	LED_Data[LEDnum][2] = Red;
	LED_Data[LEDnum][3] = Blue;
}

//sets all LEDs to the same color
void neopixel_setAllLEDColor(uint8_t red, uint8_t green, uint8_t blue){
  for(uint8_t k=0; k<neopixel_count; k++){
    neopixel_setLEDColor(k, red, green, blue);
  }
}


void neopixel_update(void){
	uint32_t indx=0;
	uint32_t color;

	for (int i= 0; i<neopixel_count; i++){

		color = ((LED_Data[i][1]<<16) | (LED_Data[i][2]<<8) | (LED_Data[i][3]));

		for (int i=23; i>=0; i--){
			if (color&(1<<i)){
				pwmData[indx] = CCR_HIGH_val;  // 2/3 of 90 CCR_HIGH_val
			}else{
        pwmData[indx] = CCR_LOW_val;  // 1/3 of 90 CCR_LOW_val
      		}
			indx++;
		}

	}

	for (int i=0; i<100; i++){
		pwmData[indx] = 0;
		indx++;
	}

	HAL_Status_PWM = HAL_TIM_PWM_Start_DMA(&htim2, TIM_CHANNEL_2, pwmData, indx);
	
  //this while technically makes the code blocking code - I NEED TO REMOVE THIS
  while (!datasentflag){}; //this flag is set in the HAL callback function
	datasentflag = 0;
}

//DMA complete callback function
void WS2812_PulseFinishedCallback(TIM_HandleTypeDef *htim){
	if(htim->Instance != TIM2){
		return; //not our timer
	}
	HAL_TIM_PWM_Stop_DMA(&htim2, TIM_CHANNEL_2);
	datasentflag=1;
}

//converting the below to a centralized callback from the main C file
/*
//DMA complete callback function
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim){
	if(htim->Instance != TIM2){
		return; //not our timer
	}
	HAL_TIM_PWM_Stop_DMA(&htim2, TIM_CHANNEL_2);
	datasentflag=1;
}
*/