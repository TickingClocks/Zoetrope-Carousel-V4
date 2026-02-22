#include <stdint.h>
#include "stm32f4xx_hal.h"    // HAL types and functions
#include "app/machine interface.h"
#include "drivers/TMC2209.h"

void TMC2209_init(system_interface *sys, uint8_t motorMode, uint8_t spreadControl){
    //set the I2C GPIO pins for the desired settings on the TMC2209 motor driver
    //future expansion planned to use UART communication for more advanced control
    //for now, simple pin settings only - step is sent via timer PWM output elsewhere

    //motorMode: 0 = full step, 1 = half step, 2 = quarter step, 3 = eighth step, 4 = sixteenth step
    //VALIDATE THAT THE ABOVE IS TRUE
    switch(motorMode){
        case 0: //full step
            sys->motor_MS1 = 0;
            sys->motor_MS2 = 0;
            break;
        case 1: //half step
            sys->motor_MS1 = 1;
            sys->motor_MS2 = 0;
            break;
        case 2: //quarter step
            sys->motor_MS1 = 0;
            sys->motor_MS2 = 1;
            break;
        case 3: //eighth step
            sys->motor_MS1 = 1;
            sys->motor_MS2 = 1;
            break;
        default:
            //default to full step
            sys->motor_MS1 = 0;
            sys->motor_MS2 = 0;
            break;
    }

    //spreadControl: 0 = stealthchop, 1 = spreadCycle enabled
    if(spreadControl == 1){
        sys->motor_spreadCycle = 1; //enable spreadCycle
    }else{
        sys->motor_spreadCycle = 0; //disable spreadCycle - stealthChop mode
    }

    //set the TMC2209 enable pin to the state of the motor enable switch
    sys->motor_enable = !sys->motor_enable_switch; //invert logic - active low

    //set the TMC2209 direction pin to the state of the motor direction switch
    sys->motor_direction = sys->motor_direction_switch;
}
