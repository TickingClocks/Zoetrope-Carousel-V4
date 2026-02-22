#ifndef TCA9534_H
#define TCA9534_H

#include <stdint.h>
#include "stm32f4xx_hal.h"    // HAL types and functions

//struct 
typedef struct {
    I2C_HandleTypeDef *hi2c; // I2C handle
    uint8_t address;         // I2C address of the TCA9534
    uint8_t config;          // Configuration register value
    uint8_t polarity;        // Polarity inversion register value
    uint8_t defVal;          // Default value register value (most relevant for outputs)
    uint8_t data;           // Current output/input data
} TCA9534_instance;

//PFP
HAL_StatusTypeDef TCA9534_init(TCA9534_instance *dev, I2C_HandleTypeDef *hi2c, uint8_t user_address, uint8_t user_config, uint8_t user_polarity, uint8_t user_defVal);
HAL_StatusTypeDef TCA9534_WriteOutputs(I2C_HandleTypeDef *hi2c, uint8_t devAddr, uint8_t outputMask);
HAL_StatusTypeDef TCA9534_ReadInputs(I2C_HandleTypeDef *hi2c, uint8_t devAddr, uint8_t *inputMask);
HAL_StatusTypeDef TCA9534_ReadInputs2(TCA9534_instance *dev, uint8_t *inputMask);

void TCA9534_updateData(TCA9534_instance *dev, uint8_t newData);

#endif