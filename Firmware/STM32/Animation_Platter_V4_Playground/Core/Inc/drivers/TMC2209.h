#ifndef TMC2209_H
#define TMC2209_H

#include <stdint.h>
#include "stm32f4xx_hal.h"    // HAL types and functions
#include "app/machine interface.h"

//I will need to include a timer here later for the PWM out signal - refrence the WS2812 driver 


//extern system_interface zoetrope; //main system instance
extern TCA9534_instance TCA_port1; //I2C I/O expander instances - used for TMC2209 motor confirguration and esp32 comms

//I want to later include UART communication functions here for the TMC2209 motor driver - basic control for now

//PFP
void TMC2209_init(system_interface *sys, uint8_t motorMode, uint8_t spreadControl);






#endif