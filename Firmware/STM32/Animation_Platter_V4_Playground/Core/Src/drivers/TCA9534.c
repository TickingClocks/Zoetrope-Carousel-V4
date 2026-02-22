#include "drivers/TCA9534.h"
#include "stm32f4xx_hal.h"   
#include "stm32f4xx_hal_i2c.h"
#include <stdint.h>

static I2C_HandleTypeDef *tca_i2c;

//HAL Helpers
HAL_StatusTypeDef HAL_Status_I2C = HAL_OK;

//register address'
uint8_t TCA9534_register_inputPort = 0x00;
uint8_t TCA9534_register_outputPort = 0x01;
uint8_t TCA9534_register_polarityInversion = 0x02;
uint8_t TCA9534_register_configuration = 0x03;

//write to a single TCA9534 register
HAL_StatusTypeDef TCA9534_WriteReg(I2C_HandleTypeDef *hi2c, uint8_t devAddr, uint8_t reg, uint8_t TCA9534_data){
    return HAL_I2C_Mem_Write(hi2c, devAddr << 1, reg, I2C_MEMADD_SIZE_8BIT, &TCA9534_data, 1, HAL_MAX_DELAY);
}

//read from a single TCA9534 register
HAL_StatusTypeDef TCA9534_ReadReg(I2C_HandleTypeDef *hi2c, uint8_t devAddr, uint8_t reg, uint8_t *TCA9534_data){
    return HAL_I2C_Mem_Read(hi2c, devAddr << 1, reg, I2C_MEMADD_SIZE_8BIT, TCA9534_data, 1, HAL_MAX_DELAY);
}

//write all outputs to TCA9534
HAL_StatusTypeDef TCA9534_WriteOutputs(I2C_HandleTypeDef *hi2c, uint8_t devAddr, uint8_t outputMask){
    return TCA9534_WriteReg(hi2c, devAddr, 0x01, outputMask);
}

//read current outputs from TCA9534
HAL_StatusTypeDef TCA9534_ReadOutputs(I2C_HandleTypeDef *hi2c, uint8_t devAddr, uint8_t *outputMask){
    return TCA9534_ReadReg(hi2c, devAddr, 0x01, outputMask);
}

//read input pins from TCA9534
HAL_StatusTypeDef TCA9534_ReadInputs(I2C_HandleTypeDef *hi2c, uint8_t devAddr, uint8_t *inputMask){
    return TCA9534_ReadReg(hi2c, devAddr, 0x00, inputMask);
}

//read single pin state from TCA9534
uint8_t TCA9534_ReadPin(I2C_HandleTypeDef *hi2c, uint8_t devAddr, uint8_t pin){
    uint8_t inputs;

    if (TCA9534_ReadInputs(hi2c, devAddr, &inputs) != HAL_OK){
    	return 0;
    }

    return (inputs >> pin) & 0x01;
}

//write single pin state from TCA9534
HAL_StatusTypeDef TCA9534_SetPin(I2C_HandleTypeDef *hi2c, uint8_t devAddr, uint8_t pin, uint8_t value){
    uint8_t port;

    //HAL_Status_I2C = TCA9534_ReadOutputs(hi2c, devAddr, &port);
    // Read current output latch
    if (TCA9534_ReadOutputs(hi2c, devAddr, &port) != HAL_OK){
    	return HAL_ERROR;
    }

    if (value > 0){
    	port |= (1 << pin);
    }else{
    	port &= ~(1 << pin);
    }

    return TCA9534_WriteOutputs(hi2c, devAddr, port);
}

//initialization and configuration of a TCA9534 I2C I/O expansion
//blocking code
//returns status of I2C
//pass in,  chip address, 
//          configuration of each pin, 
//          polarity setting for each pin, 
//          default output values for each pin
HAL_StatusTypeDef TCA9534_init(TCA9534_instance *dev, I2C_HandleTypeDef *hi2c, uint8_t user_address, uint8_t user_config, uint8_t user_polarity, uint8_t user_defVal){

    uint8_t deviceAddressSend = user_address << 1;
  
    dev->hi2c = hi2c; //store I2C handle for later use
    tca_i2c = hi2c; //store I2C handle for later use
    dev->address = user_address; //set address
    dev->config = user_config; //set input/output configuration
    dev->polarity = user_polarity; //set polarity inversion
    dev->defVal = user_defVal; //set default (output) values

    //configure the TCA9534 registers
    HAL_Status_I2C = HAL_I2C_Mem_Write(tca_i2c, deviceAddressSend, TCA9534_register_configuration, I2C_MEMADD_SIZE_8BIT, &dev->config, 1, HAL_MAX_DELAY);
    HAL_Status_I2C = HAL_I2C_Mem_Write(tca_i2c, deviceAddressSend, TCA9534_register_polarityInversion, I2C_MEMADD_SIZE_8BIT, &dev->polarity, 1, HAL_MAX_DELAY);
    HAL_Status_I2C = HAL_I2C_Mem_Write(tca_i2c, deviceAddressSend, TCA9534_register_outputPort, I2C_MEMADD_SIZE_8BIT, &dev->defVal, 1, HAL_MAX_DELAY);

    //return the status of the I2C operations
    return HAL_Status_I2C;
}

//read input pins from TCA9534
HAL_StatusTypeDef TCA9534_ReadInputs2(TCA9534_instance *dev, uint8_t *inputMask){
    return TCA9534_ReadReg(dev->hi2c, dev->address, TCA9534_register_inputPort, inputMask);
}

void TCA9534_updateData(TCA9534_instance *dev, uint8_t newData){
    dev->data = newData;
}
