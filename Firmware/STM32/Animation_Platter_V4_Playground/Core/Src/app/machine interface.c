#include "app/machine interface.h"
#include "drivers/TCA9534.h"
#include "drivers/WS2812.h"
#include "stm32f446xx.h"
#include "stm32f4xx_hal.h"   
#include "stm32f4xx_hal_def.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_i2c.h"
#include <stdint.h>

#include "drivers/pn532_HAL.h"
#include "drivers/pn532.h"

#include "drivers/ESP_UART.h"
#include "drivers/VEML7700.h"

void zoetrope_strobeModeSelection(system_interface *sys){
    //select strobe mode based on system variable
    //0 == classic strobe mode
    //1 == 5 digit 7 segment display mode
    switch(sys->strobeMode){
        case 0:
            //classic mode
            strobe_strobeModeClassic(sys);
            break;
        case 1:
            //5 digit 7 segment display mode
            strobe_strobeMode5Digit7SegmentDisplay(sys);
            //strobe_strobeModeClassic(sys); //temp test
            break;
        default:
            strobe_strobeModeClassic(sys);
            break;
    }
}

void zoetrope_hardwareRuntime(system_interface *sys){
    
    //interfacing hardware section
    //strobe light catch - always off when needed to be OFF
    //when the strobe is supposed to be OFF, make sure it is OFF, dont wait for the next speed update
    if(!sys->strobeEnabled){ //this is a catch to ensure strobe is off when it should be - convienent and fast place to put this
        strobe_timerPWMControl(sys); //ensure strobe is off
    }

    //I2C_IO_run(); //call this function prior to chaning any I/O variables (it will overwrite changes otherwise)
    zoetrope_encoderHandler(sys); //encoder rotation handler
    zoetrope_encoderButton(sys); //encoder button handler
    zoetrope_leftButton(sys); //left button handler
    zoetrope_rightButton(sys); //right button handler
    zoetrope_run(sys); //handles neopixels
    sys->fan1_enable = sys->motor_enable_switch; //fan 1 enabled when motor enabled switch is ON
    heartbeat(sys); //run the heartbeat function - also controls the I2C I/O expander outputs
}

void zoetrope_esp32DisplayRuntime(system_interface *sys){
    //display update section
    sys->esp32_1_highlightFlag = 1; //highlight is locked on from this line
    zoetrope_sendESP32Data(sys); //send data to the ESP32 screens via UART
}

void zoetrope_init(system_interface *sys){
    //LEDs defaults
    sys->heartbeatTime_ms = 50; //default heartbeat time
    sys->greenLED = 0; //default off
    sys->redLED = 0; //default off
    sys->blueLED = 0; //default off

    sys->stateLEDStatus = 1; //default state LED ON
    sys->stateLEDPulseTime_ms = 500; //blinking time - default is 500ms
    sys->prevStateLEDPulseTime_ms = 0; //used for blinking time keeping

    sys->currentTime1 = 0;

    sys->stateLED_firstTime = 1; //first time flag

    sys->system_state = 1; //default to idle state - unsure how stable this one is right now

    sys->stateMachine_state = 0; //default to idle state - this will control the overal system behavior

    sys->stateLEDOK_R = 0; //OK green value
    sys->stateLEDOK_G = 5; //OK blue value
    sys->stateLEDOK_B = 0; //OK red value

    sys->stateLEDIdle_R = 0; //Idle red value
    sys->stateLEDIdle_G = 0; //Idle green value
    sys->stateLEDIdle_B = 5; //Idle blue value

    sys->stateLEDError_R = 25; //Error red value
    sys->stateLEDError_G = 0; //Error green value
    sys->stateLEDError_B = 0; //Error blue value

    sys->stateLEDBlank_R = 0; //Blank red value
    sys->stateLEDBlank_G = 0; //Blank green value
    sys->stateLEDBlank_B = 0; //Blank blue value

    sys->switchVariableLock = 1; //system starts not initialized, so lock certain variables

    //switches and buttons defaults
    sys->strobe_enable_switch = 0;
    sys->motor_enable_switch = 0;
    sys->motor_direction_switch = 0;
    sys->button_left = 0;
    sys->button_right = 0;
    sys->button_encoder = 0;
    sys->prev_button_left = 0;
    sys->prev_button_right = 0;
    sys->prev_button_encoder = 0;

    //heartbeat defaults
    sys->heartbeatCurrentTime_ms = 0;
    sys->prevHeartbeatTime_ms = 0;
    sys->heartbeatState = 0;
    sys->heartbeatLEDEnabled = 1; //enable heartbeat LED by default

    //I2C I/O expander defaults
    sys->U3_interruptFlag = 0; //indicates if an I2C read is needed
    sys->U7_interruptFlag = 0;
    sys->U10_interruptFlag = 0;

    //sensor variable defaults
    sys->hallSensor1 = 0;
    sys->hallSensor1LastTime_ms = 0;
    sys->hallSensor1Period_us = 0;
    sys->hallSensor1Period_ms = 0;
    sys->hallSensor2 = 0; //not implemented
    sys->hallSensor3 = 0; //not implemented
    sys->hallSensor4 = 0; //not implemented
    sys->platterSensor = 0; //not implemented
    sys->tempSensor1 = 0; //not implemented
    sys->tempSensor2 = 0; //not implemented
    sys->ambientLightLevel1 = 0; //not implemented
    sys->ambientLightLevel2 = 0; //not implemented

    //fans defaults
    sys->fan1_enable = 0; //fan 1 disabled by default
    sys->fan2_enable = 0; //fan 2 disabled by default

    //zoetrope defaults
    sys->zoetropeEnabled = 0; //zoetrope disabled by default
    sys->strobeOnTime_ms = 0;
    sys->strobeOnTime_us = 0;
    sys->strobeOffTime_ms = 0; //this is calculated with sliceTime and is dependint on total slice time and strobe on time
    sys->sliceCount = 20; //default to 20 slices
    sys->sliceIndex = 0; //start at slice 0
    sys->platterRotationPeriod_ms = 0;
    sys->platterRotationPeriod_us = 0;
    sys->sliceTime_ms = 0;
    //new strobe mode variables
    sys->strobeSrcClockValue = 45000000; //45MHz clock source for timer 4 (full speed, APB1)
    sys->strobePSCValue = 0; //calculated strobe PSC value for PWM output
    sys->strobeARRValue = 1000; //calculated strobe ARR value for PWM output - attempting to keep this at 1000 for simpler CCR control
    sys->strobeCCRValue = 10; //calculated strobe CCR value for PWM output - value 0 to ARR
    sys->strobeMode = 0; //default strobe mode (classic)
    sys->strobeEnabled = 0; //strobe disabled by default
    //reset the strobe PWM arrays
    for(int i = 0; i < 256; i++){
        sys->shadowStrobePWMArray[i] = 0;
        sys->StrobePWMArray[i] = 0;
    }

    //esp32 defaults
    sys->esp32_1_dataSendEnabled = 1; //enable sending data to esp32 1 by default
    sys->esp32_1_lastTXTime_ms = 0; //reset this timer
    sys->esp32_1_TXInterval_ms = 50; //ms, send data at 20Hz rate
    sys->esp32_1_verticalLineHighlight = 0; //start at line 6 for testing
    sys->esp32_1_horizontalLineHighlight = 0; //start at label side
    sys->esp32_1_highlightFlag = 1; //highlight is on for testing

    //motor drive defaults
    sys->motorSelection = 0; //0 == stepper motor by default, 1 == BLDC motor
    sys->stepper_Kp = 0.0f; //set this later
    sys->stepper_Ki = 0.0f;
    sys->stepper_Kd = 0.0f;
    sys->stepper_previousError = 0.0f;
    sys->stepper_integral = 0.0f;
    sys->bldc_Kp = 0.0f; //set this later
    sys->bldc_Ki = 0.0f;
    sys->bldc_Kd = 0.0f;
    sys->bldc_previousError = 0.0f;
    sys->bldc_integral = 0.0f;

    //motor defaults
    sys->motorTMC2209Type = 1; //indicates we are using TMC2209 stepper driver
    sys->PWMMotorSpeedValue = 0; //default PWM motor speed value
    sys->motorMicrostepSetting = 0; //default motor microstep setting (full step)
    sys->motorEnabled = 0; //motor disabled by default
    sys->motor_spreadCycle = 0; //spreadCycle disabled by default - use stealthchop


    sys->newMotorControl = 1; //set this to 1 to enable the new motor control
    sys->microstepSetting = 1; //default microstep setting variable - configures the MS1, MS2 pins
    zoetrope_stepperMotorMicroStepConfig(sys); //apply the microstep configuration
    sys->TIM8_CLK_HZ = 90000000; //90MHz clock for motor control timer 
    sys->FULL_STEPS_PER_REV = 200; //default full steps per revolution for stepper motor
    //sys->MICROSTEPS = 8; //not using this one, use microstepsetting
    sys->STEPS_PER_REV = 1600; //default steps per revolution for stepper motor at 1/8 microstepping
    sys->TIM_MAX_ARR = 65535; //16 bit timer max ARR value
    sys->TIM8_PSC = 0; //timer prescaler value
    sys->TIM8_ARR = 0; //timer auto-reload value
    sys->TIM8_CCR = 0; //timer capture/compare value
    sys->TIM8_StepFrequency = 0.0f; //calculated step frequency for motor control



    //motor mode sets the control mode for the motor - different spinup and control methods
    //it also may control the set speed in specific modes
    //0 == default mode - PID control, default system settings
    //1 == nfc search mode - specific spinup and speed for searching NFC tags
    //2 == high speed mode - specific spinup and speed for high speed operation
    //3 == nfc mode - motor speed settings are set from NFC read data - uses PID control
    //100 == testing mode - direct motor control from potentiometer, no PID
    sys->motorMode = 0; //default motor mode (LOGIC NOT YET IMPLEMENTED)

    //NFC defaults
    sys->NFCDetectedFlag = 0; //no NFC tag detected by default
    sys->NFCFailFlag = 1; //no NFC read/write failure by default
    sys->uid_len = 0; //length of detected NFC tag UID
    sys->pn532_error1 = 0; //reset PN532 error variable
    sys->nfcUserStartingblockNumber = 6; //default starting block number for user data
    sys->nfcDataReadV1_pageCount = 10; //number of pages read from NFC tag for version 1 data
    sys->nfcSuccess = 0; //no success by default
    zoetrope_resetNFCReadBuffer(sys); //reset the NFC read buffer
    sys->nfcSearchForTagFlag = 1; //search for NFC tag on startup (first time spinning motor)
    sys->nfcHallSensorReadCount = 0; //reset hall sensor read count
    sys->nfcHallSensorReadTarget = 2; //number of hall sensor reads to perform during NFC search mode - before giving up

    sys->normalMotorEnableSwitch = 1; //by default, motor enable switch works normally


    //setting up 5 digit 7 segment display strobe mode variables

    //defining character patterns for the 7 segment display
    sys->character_0 = 0b01111110; //segments a,b,c,d,e,f
    sys->character_1 = 0b00110000; //segments b,c
    sys->character_2 = 0b01101101; //segments a,b,c,d,e,g
    sys->character_3 = 0b01111001; //segments a,b,c,d,f,g
    sys->character_4 = 0b00110011; //segments b,c,f,g
    sys->character_5 = 0b01011011; //segments a,c,d,f,g
    sys->character_6 = 0b01011111; //segments a,c,d,e,f,g
    sys->character_7 = 0b01110000; //segments a,b,c
    sys->character_8 = 0b01111111; //segments a,b,c,d,e,f,g
    sys->character_9 = 0b01111011; //segments a,b,c,d,f,g
    sys->character_dash = 0b00000001; //segment g only

    sys->displayCharacterOffset = 6; //for locating the display

    //locations of the segments on the 5 digit seven segment display platter (3 locations each)
    sys->segmentLocations_A[0] = 0;
    sys->segmentLocations_A[1] = 35;
    sys->segmentLocations_A[2] = 70;
    sys->segmentLocations_B[0] = 5;
    sys->segmentLocations_B[1] = 40;
    sys->segmentLocations_B[2] = 75;
    sys->segmentLocations_C[0] = 10;
    sys->segmentLocations_C[1] = 45;
    sys->segmentLocations_C[2] = 80;
    sys->segmentLocations_D[0] = 15;
    sys->segmentLocations_D[1] = 50;
    sys->segmentLocations_D[2] = 85;
    sys->segmentLocations_E[0] = 20;
    sys->segmentLocations_E[1] = 55;
    sys->segmentLocations_E[2] = 90;
    sys->segmentLocations_F[0] = 25;
    sys->segmentLocations_F[1] = 60;
    sys->segmentLocations_F[2] = 95;
    sys->segmentLocations_G[0] = 30;
    sys->segmentLocations_G[1] = 65;
    sys->segmentLocations_G[2] = 100;

    uint8_t tempTest = 7;
    sys->digit1 = tempTest; //single digit seconds
    sys->digit2 = tempTest; //tens digit seconds
    sys->digit3 = tempTest; //separator digit
    sys->digit4 = tempTest; //single digit minutes
    sys->digit5 = tempTest; //tens digit minutes

    sys->digit1 = 10; //single digit seconds
    sys->digit2 = 1; //tens digit seconds
    sys->digit3 = 2; //separator digit
    sys->digit4 = 3; //single digit minutes
    sys->digit5 = 10; //tens digit minutes

    /* //for when I finish testing
    sys->digit1 = 3; //single digit seconds
    sys->digit2 = 3; //tens digit seconds
    sys->digit3 = 3; //separator digit
    sys->digit4 = 3; //single digit minutes
    sys->digit5 = 3; //tens digit minutes
    */

    sys->demoCounter = 0; //for testing the 5 digit 7 segment display mode - incremented in heartbeat function (maybe neopixel function)

}

void zoetrope_run(system_interface *sys){
    zoetrope_switchLightsAndState(sys);
}

uint8_t zoetrope_I2CPortByte(system_interface *sys, uint8_t portNum){
    uint8_t data = 0;
    switch(portNum){
        case 1: //U3
            data =  (!sys->motor_enable << 0) | //TMC2209 enable is active low
                    (sys->motor_MS2 << 1) |
                    (sys->motor_spreadCycle << 2) |
                    (sys->motor_direction << 3) |
                    (sys->motor_MS1 << 4) |
                    (sys->esp32_2_ext1 << 5) |
                    (sys->esp32_2_ext2 << 6) |
                    (sys->U3_hearbeat << 7);
            break;
        case 2: //U7
            data =  (sys->strobe_enable_switch << 0) |
                    (sys->motor_enable_switch << 1) |
                    (sys->motor_direction_switch << 2) |
                    (sys->U7_P3 << 3) |
                    (sys->U7_P4 << 4) |
                    (sys->U7_P5 << 5) |
                    (sys->U7_P6 << 6) |
                    (sys->U7_hearbeat << 7);
            break;
        case 3: //U10
            data =  (sys->button_left << 0) |
                    (sys->esp32_1_ext1 << 1) |
                    (sys->button_encoder << 2) |
                    (sys->esp32_1_ext2 << 3) |
                    (sys->button_right << 4) |
                    (sys->fan2_enable << 5) |
                    (sys->fan1_enable << 6) |
                    (sys->U10_hearbeat << 7);
            break;
        default:
            data = 0;
            break;
    }
    return data;
}


void zoetrope_switchLightsAndState(system_interface *sys){
    sys->currentTime1 = HAL_GetTick(); //note current time for blinking feature
    if((sys->currentTime1 - sys->prevStateLEDPulseTime_ms) >= sys->stateLEDPulseTime_ms){
        sys->prevStateLEDPulseTime_ms = sys->currentTime1;
        if(sys->neopixelBlinkState){
            sys->neopixelBlinkState = 0;
        }else{
            sys->neopixelBlinkState = 1;

            //for testing the 5 digit 7 segment display mode
            if(sys->motor_enable){
                sys->demoCounter++;

                //assign demoCounter value to digits
                if(sys->demoCounter >= 1000){
                    sys->demoCounter = 0; //roll over
                }
                if(sys->demoCounter >= 100){
                    sys->digit1 = 10; //blank for space
                    sys->digit2 = (sys->demoCounter / 100) % 10;
                    sys->digit3 = (sys->demoCounter / 10) % 10;
                    sys->digit4 = sys->demoCounter % 10;
                    sys->digit5 = 10; //blank for space
                }else{
                    if(sys->demoCounter >= 10){
                        sys->digit1 = 10; //blank for space
                        sys->digit2 = sys->demoCounter / 10;
                        sys->digit3 = sys->demoCounter % 10;
                        sys->digit4 = 10; //blank for space
                        sys->digit5 = 10; //blank for space
                    }else{
                        sys->digit1 = 10; //blank for space
                        sys->digit2 = sys->demoCounter;
                        sys->digit3 = 10; //blank for space
                        sys->digit4 = 10; //blank for space
                        sys->digit5 = 10; //blank for space
                    }
                }
            }
        }
    }

    //set the value for neopixel 0 - state LED
    switch(sys->system_state){
        case 0: //running state
            neopixel_setLEDColor(0, sys->stateLEDOK_R, sys->stateLEDOK_G, sys->stateLEDOK_B); //green neopixel state LED
            break;

        case 1: //not-initialized state
            if(sys->neopixelBlinkState){
                neopixel_setLEDColor(0, sys->stateLEDIdle_R, sys->stateLEDIdle_G, sys->stateLEDIdle_B); //blue neopixel state LED
            }else{
                neopixel_setLEDColor(0, sys->stateLEDBlank_R, sys->stateLEDBlank_G, sys->stateLEDBlank_B); //blue neopixel state LED
            }
            break;
        
        default:
            //error state
            neopixel_setLEDColor(0, sys->stateLEDError_R, sys->stateLEDError_G, sys->stateLEDError_B); //red neopixel state LED
            sys->redLED = 1; //turn on the red smd LED
            break;
    }

    //set the value for neopixel 1 - strobe enable switch
    if(sys->strobe_enable_switch){
        //check that system is initialized before turning on the strobe LED
        if(sys->initialized){
            if(sys->strobe_enable_switch == sys->strobeEnabled){
                //setting has taken
                neopixel_setLEDColor(1, 0, 5, 0); //green
            }else{
                //setting has not yet taken
                if(sys->neopixelBlinkState){
                    neopixel_setLEDColor(1, 0, 5, 0);
                }else{
                    neopixel_setLEDColor(1, sys->stateLEDBlank_R, sys->stateLEDBlank_G, sys->stateLEDBlank_B); //blue neopixel state LED
                }
            }
        }else{
            //blink the LED to show setting has not taken
            if(sys->neopixelBlinkState){
                neopixel_setLEDColor(1, 0, 5, 0); 
            }else{
                neopixel_setLEDColor(1, sys->stateLEDBlank_R, sys->stateLEDBlank_G, sys->stateLEDBlank_B); //blue neopixel state LED
            }
        }
      
    }else{
      neopixel_setLEDColor(1, 5, 0, 0); //red
    }

    //set the value for neopixel 2 - motor enable switch
    if(sys->motor_enable_switch){
        //check that system is initialized
        if(sys->initialized){
            neopixel_setLEDColor(2, 0, 5, 0);
        }else{
            //blink the LED to show setting has not taken
            if(sys->neopixelBlinkState){
                neopixel_setLEDColor(2, 0, 5, 0); 
            }else{
                neopixel_setLEDColor(2, sys->stateLEDBlank_R, sys->stateLEDBlank_G, sys->stateLEDBlank_B); //blue neopixel state LED
            }
        }
    }else{
      neopixel_setLEDColor(2, 5, 0, 0);
    }

    //set the value for neopixel 3 - motor direction switch
    if(sys->motor_direction_switch){
        //check that the motor is not enabled
        if(sys->motor_enable_switch){
            if((sys->neopixelBlinkState) && (sys->motor_direction_switch != sys->motor_direction)){ //will need to add a second condition here
                neopixel_setLEDColor(3, sys->stateLEDBlank_R, sys->stateLEDBlank_G, sys->stateLEDBlank_B); //red - to indicate setting has not taken
            }else{
                neopixel_setLEDColor(3, 5, 5, 0);
            }
        }else{
            neopixel_setLEDColor(3, 5, 5, 0);
        }
        
    }else{
        //check that the motor is not enabled
        if(sys->motor_enable_switch){
            if((sys->neopixelBlinkState) && (sys->motor_direction_switch != sys->motor_direction)){ //will need to add a second condition here
                neopixel_setLEDColor(3, sys->stateLEDBlank_R, sys->stateLEDBlank_G, sys->stateLEDBlank_B); //red - to indicate setting has not taken
            }else{
                neopixel_setLEDColor(3, 5, 0, 5);
            }
        }else{
            neopixel_setLEDColor(3, 5, 0, 5);
        } 
    }
    neopixel_update(); //update the neopixel LEDs
}

void zoetrope_assignHardwareI2CInputs(system_interface *sys, uint8_t interface, uint8_t data){
    
    //interface == 0: TBD
    //interface == 1: U3 I2C I/O expander
    //interface == 2: U7 I2C I/O expander
    //interface == 3: U10 I2C I/O expander

    switch(interface){
        case 2: //U7
            zoetrope_assignI2CData(sys, interface, data); //sending from port 2 (U7)
            break;

        case 3: //U10
            zoetrope_assignI2CData(sys, interface, data); //sending from port 3 (U10)
            break;

        default:
            break;
    }
}


void zoetrope_assignI2CData(system_interface *sys, uint8_t portNum, uint8_t data){
    //disale everything that should be an output - keeps it from being overwitten before the output is updated?
    switch(portNum){
        case 1: //U3
            //sys->motor_enable = (data >> 0) & 0x01; //output - this reads output state
            //sys->motor_MS2 = (data >> 1) & 0x01; //output - this reads output state
            //sys->motor_spreadCycle = (data >> 2) & 0x01; //output - this reads output state
            //sys->motor_direction = (data >> 3) & 0x01; //output - this reads output state
            //sys->motor_MS1 = (data >> 4) & 0x01; //output - this reads output state
            sys->esp32_2_ext1 = (data >> 5) & 0x01; //not used yet
            sys->esp32_2_ext2 = (data >> 6) & 0x01; //not used yet
            //sys->U3_hearbeat = (data >> 7) & 0x01; //output - this reads output state
            break;
        case 2: //U7
            sys->strobe_enable_switch = (data >> 0) & 0x01;
            //sys->strobe_enable_switch = !sys->strobe_enable_switch; //invert switch logic (active low)
            sys->motor_enable_switch = (data >> 1) & 0x01;
            //sys->motor_enable_switch = !sys->motor_enable_switch; //invert switch logic (active low)
            sys->motor_direction_switch = (data >> 2) & 0x01;
            //sys->motor_direction_switch = !sys->motor_direction_switch; //invert switch logic (active low)
            sys->U7_P3 = (data >> 3) & 0x01; //not used - configured as inputs
            sys->U7_P4 = (data >> 4) & 0x01; //not used - configured as inputs
            sys->U7_P5 = (data >> 5) & 0x01; //not used - configured as inputs
            sys->U7_P6 = (data >> 6) & 0x01; //not used - configured as inputs
            //sys->U7_hearbeat = (data >> 7) & 0x01; //output - this reads output state
            break;
        case 3: //U10
            sys->button_left = (data >> 0) & 0x01;
            sys->button_left = !sys->button_left; //invert switch logic (active low)
            sys->esp32_1_ext1 = (data >> 1) & 0x01; //not used yet - configured as input
            sys->button_encoder = (data >> 2) & 0x01;
            sys->button_encoder = !sys->button_encoder; //invert switch logic (active low)
            sys->esp32_1_ext2 = (data >> 3) & 0x01; //not used yet - configured as input
            sys->button_right = (data >> 4) & 0x01;
            sys->button_right = !sys->button_right; //invert switch logic (active low)  
            //sys->fan2_enable = (data >> 5) & 0x01; //output - this reads output state
            //sys->fan1_enable = (data >> 6) & 0x01; //output - this reads output state
            //sys->U10_hearbeat = (data >> 7) & 0x01; //output - this reads output state
            break;
        default:
            break;
    }
}


///////////////
//heartbeat////
///////////////
void heartbeat(system_interface *sys){
    
    sys->heartbeatCurrentTime_ms = HAL_GetTick();
    if(sys->heartbeatCurrentTime_ms - sys->prevHeartbeatTime_ms >= sys->heartbeatTime_ms){
      
      sys->prevHeartbeatTime_ms = sys->heartbeatCurrentTime_ms;
      sys->heartbeatState = !sys->heartbeatState;

      if(sys->heartbeatLEDEnabled){
        //green LED as the heartbeat indicator - main STM32 heartbeat
        sys->greenLED = sys->heartbeatState;
        sys->U3_hearbeat = sys->heartbeatState;
        sys->U7_hearbeat = sys->heartbeatState;
        sys->U10_hearbeat = sys->heartbeatState;
      }

      //everything above this line is adjusting variables that may affect I2C outputs

      //write to the I/O expander outputs and LEDs
      HAL_GPIO_WritePin(GPIOC, testLED1_Pin, sys->blueLED);
      HAL_GPIO_WritePin(GPIOC, testLED2_Pin, sys->redLED);
      HAL_GPIO_WritePin(GPIOC, testLED3_Pin, sys->greenLED);

      //update the data value for all three I2C I/O expanders
      TCA9534_updateData(&TCA_port1, zoetrope_I2CPortByte(sys, 1));
      TCA9534_updateData(&TCA_port2, zoetrope_I2CPortByte(sys, 2));
      TCA9534_updateData(&TCA_port3, zoetrope_I2CPortByte(sys, 3));

      //send the updated data to the I2C I/O expanders
      TCA9534_WriteOutputs(TCA_port1.hi2c, TCA_port1.address, TCA_port1.data);
      TCA9534_WriteOutputs(TCA_port2.hi2c, TCA_port2.address, TCA_port2.data);
      TCA9534_WriteOutputs(TCA_port3.hi2c, TCA_port3.address, TCA_port3.data);

    }
}

//simple map function
long map(long x, long in_min, long in_max, long out_min, long out_max){
  return (x - in_min) * (out_max - out_min + 1) / (in_max - in_min + 1) + out_min;
}

//basic zoetope GPIO control and sensing
void hall1_readAndMeasure(system_interface *sys){
    uint32_t currentTime_us = TIM5->CNT; //check microsecond timer
    //uint32_t currentTime_ms = HAL_GetTick(); //note current time coming into function - used for period measurement
    uint8_t prevhallSensor1State = 0;

    //note the prev sensor state
    prevhallSensor1State = sys->hallSensor1;
    //read the state of hall sensor 1 and assign the system variable
    sys->hallSensor1 = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_0); //PC0 == hall sensor 1 input

    //see if the state of the sensor has changed
    if(sys->hallSensor1 != prevhallSensor1State){
        //sensor state has changed
        if(sys->hallSensor1 == 0){ //falling edge detected - signal is active high
            //calculate the period since the last rising edge
            sys->hallSensor1Period_us = currentTime_us - sys->hallSensor1LastTime_us;
            sys->hallSensor1LastTime_us = currentTime_us; //update the last time variable
            sys->platterRotationPeriod_us = sys->hallSensor1Period_us; //for now, platter period == hall sensor period
            zoetrope_calculateFramePeriod(sys); //calculate the frame pulse period using system variables
            sys->strobePWMCalculateRequired = 1; //flag to recalculate strobe PWM settings -- do in interrupt in future
        }
    }
}


void zoetrope_calculateFramePeriod(system_interface *sys){
    //calculate the frame pulse period using system variables
    sys->sliceTime_us = sys->platterRotationPeriod_us / sys->sliceCount;
    //calculate the slice OFF time based on the current slice ON time setting
    sys->strobeOffTime_us = sys->sliceTime_us - sys->strobeOnTime_us;
}

void zoetrope_sendESP32Data(system_interface *sys){
    
    uint32_t currentTime_ms = HAL_GetTick(); //note current time
    //check if it is time to send data to the ESP32
    if(sys->esp32_1_dataSendEnabled){
        if(currentTime_ms - sys->esp32_1_lastTXTime_ms >= sys->esp32_1_TXInterval_ms){
            //time to send data to ESP32 1
            sys->esp32_1_lastTXTime_ms = currentTime_ms; //update last TX time

            packet_send(sys, 0x10, 16+34);
        }
    }
}

void zoetrope_rightButton(system_interface *sys){
    //sys->blueLED = sys->button_right; //blue LED on when right button pressed - DISABLED SINCE IT INTERFERES WITH NFC
    
    //right button code
    if(sys->prev_button_right != sys->button_right){
        sys->prev_button_right = sys->button_right;

        if(sys->button_right){

            if(!sys->motor_enable_switch){
                //if the motor switch is not enabled, this button will hold motor in place (for platter installation)
                TIM8->CCR1 = 0; //set motor speed to zero
                sys->motor_enable = 1; //temporarily enable the motor

                sys->normalMotorEnableSwitch = 0; //set flag to indicate normal motor enable switch is disabled
            }else{
                //in case we want to do something when the motor is enabled later
            }
        }else{
            //button released
            sys->normalMotorEnableSwitch = 1; //if this button is not pressed, the motor enable switch will always work normally

            if(!sys->motor_enable_switch){
                //if the motor switch is not enabled, this button will hold motor in place (for platter installation)
                //disable the motor
                sys->motor_enable = 0; //disable the motor
            }else{
                //in case we want to do something when the motor is enabled later
            }
        }
    }
}

void zoetrope_leftButton(system_interface *sys){

    //left button code
    if(sys->prev_button_left != sys->button_left){
      sys->prev_button_left = sys->button_left;

      if(sys->button_left){
        //button pressed - first pass through
        if(sys->initialized == 0){
            sys->initialized = 1; //set initialized flag
            sys->system_state = 0; //set system state to running
        }else{
            sys->initialized = 0; //clear initialized flag
            sys->system_state = 1; //set system state to idle
        }
      }
    }
}

void zoetrope_encoderButton(system_interface *sys){
    //encoder button code
    if(sys->prev_button_encoder != sys->button_encoder){
        sys->prev_button_encoder = sys->button_encoder;

        if(sys->button_encoder){
            //button pressed - first pass through
            //check if a line is currently highlighted
            if(sys->esp32_1_highlightFlag){
                //check if we are on a line that can be selected
                if((sys->esp32_1_verticalLineHighlight == 2) ||  (sys->esp32_1_verticalLineHighlight == 3)){
                    //valid line for selection - change horizontal selection
                    if(sys->esp32_1_horizontalLineHighlight == 0){
                        sys->esp32_1_horizontalLineHighlight = 1; //value side
                    }else{
                        sys->esp32_1_horizontalLineHighlight = 0; //label side
                    }
                }
            }
        }
    }
}

void zoetrope_encoderHandler(system_interface *sys){
    //handle encoder input - to be implemented
    //encoder input handling
    sys->encoderTimerCount = TIM1->CNT; //read the encoder timer count register
    if(sys->encoderTimerCount != sys->prevEncoderTimerCount){
      //we need to check what horizontal column is highlighted
      if(sys->esp32_1_horizontalLineHighlight){
        //variable selected, now update variable with encoder, rather than highlighted line
        //determine direction
        if(sys->encoderTimerCount > sys->prevEncoderTimerCount){ //determine direction
            //calculate the difference
            sys->encoderPulseDifferenceCalculated = sys->encoderTimerCount - sys->prevEncoderTimerCount;
            if(sys->encoderPulseDifferenceCalculated >= 2){
              //determine variable
              switch(sys->esp32_1_verticalLineHighlight){
                case 2:
                  //frame count
                  if(sys->sliceCount > 1){
                    sys->sliceCount--; 
                  }
                  
                break;

                case 3:
                  //stobe one time
                  if(sys->strobeCCRValue > 0){
                    sys->strobeCCRValue--; //this isnt used in the algorithm, or passed ot the screen yet
                  }
                break;

                default:
                  //do nothing
                break;
              }

              //update the previous count value - only if enough movement to make a change
              sys->prevEncoderTimerCount = sys->encoderTimerCount;

            }
        }else{
            //turned the other way
            sys->encoderPulseDifferenceCalculated = sys->prevEncoderTimerCount - sys->encoderTimerCount;
            if(sys->encoderPulseDifferenceCalculated >= 2){
                //determine variable
                switch(sys->esp32_1_verticalLineHighlight){
                    case 2:
                    //frame count
                    sys->sliceCount++;
                    break;

                    case 3:
                    //stobe one time
                    sys->strobeCCRValue++; //this isnt used in the algorithm, or passed ot the screen yet
                    break;

                    default:
                    //do nothing
                    break;
                }

                //update the previous count value - only if enough movement to make a change
                sys->prevEncoderTimerCount = sys->encoderTimerCount;
            }
        }

        }else{ //line selection mode
            //determine direction
            if(sys->encoderTimerCount > sys->prevEncoderTimerCount){ //determine direction
                //turned one way
                //calculate the difference
                sys->encoderPulseDifferenceCalculated = sys->encoderTimerCount - sys->prevEncoderTimerCount;
                //we only want to move the highlight for every x2 pulses
                if(sys->encoderPulseDifferenceCalculated >= 2){
                    sys->encoderPulseDifferenceCalculated = sys->encoderPulseDifferenceCalculated / 2; //reduce the value for highlight movement
                    //move the highlighted line
                    //sys->esp32_1_verticalLineHighlight -= sys->encoderPulseDifferenceCalculated;
                    if(sys->esp32_1_verticalLineHighlight > 0){
                        sys->esp32_1_verticalLineHighlight--; //see if this works better
                    }else{
                        sys->esp32_1_verticalLineHighlight = 0; //min line
                    }
                    //make sure the line is actually highlighted
                    sys->esp32_1_highlightFlag = 1;

                    //update the previous count value - only if enough movement to make a change
                    sys->prevEncoderTimerCount = sys->encoderTimerCount;
                }
            }else{
                //turned the other way
                sys->encoderPulseDifferenceCalculated = sys->prevEncoderTimerCount - sys->encoderTimerCount;
                //we only want to move the highlight for every x2 pulses
                if(sys->encoderPulseDifferenceCalculated >= 2){
                    sys->encoderPulseDifferenceCalculated = sys->encoderPulseDifferenceCalculated / 2; //reduce the value for highlight movement
                    //move the highlighted line
                    //sys->esp32_1_verticalLineHighlight += sys->encoderPulseDifferenceCalculated;
                    sys->esp32_1_verticalLineHighlight++; //see if this works better
                    if(sys->esp32_1_verticalLineHighlight > 22){
                        sys->esp32_1_verticalLineHighlight = 20; //max line
                    }
                    //make sure the line is actually highlighted
                    sys->esp32_1_highlightFlag = 1;

                    //update the previous count value - only if enough movement to make a change
                    sys->prevEncoderTimerCount = sys->encoderTimerCount;
                }
            }
        } 
    }
}

void strobe_strobeMode5Digit7SegmentDisplay(system_interface *sys){
    //check flag to start/update PWM output
    if(sys->strobePWMCalculateRequired){//this flag is set when a new hall sensor read is detected
      strobe_updatePWMArray(sys);//update the PWM array with the values in the shadow array (applying previous calculations)
      strobe_applyPWMSettings(sys); //this must be called before calculating the next values
      //start the PWM output of the strobe based on previously calculated values from the previous speed 
      strobe_timerPWMControl(sys); //start the strobe PWM output (CCR sent via DMA)
      //calculate the new pwm values based on the new speed reading - update the shadow array
      strobe_resetShadowArray(sys);
      strobe_calculatePWMValues(sys); //calculates the PSC and ARR values based on current speed (frequency and duty cycle of the strobe PWM signal)
      //strobe_populateShadowArray(sys); //writes the PWM pattern for one rotation to shadow array for next HALL sensor pass
      strobe_populateShadowArray_5Digit7Segment(sys); //new PWM pattern for 5 digit 7 segment display
      sys->strobePWMCalculateRequired = 0; //clear the flag
    }
}

void strobe_strobeModeClassic(system_interface *sys){
    
    //check flag to start/update PWM output
    if(sys->strobePWMCalculateRequired){//this flag is set when a new hall sensor read is detected
      strobe_updatePWMArray(sys);//update the PWM array with the values in the shadow array (applying previous calculations)
      strobe_applyPWMSettings(sys); //this must be called before calculating the next values
      //start the PWM output of the strobe based on previously calculated values from the previous speed 
      strobe_timerPWMControl(sys); //start the strobe PWM output
      //calculate the new pwm values based on the new speed reading - update the shadow array
      strobe_resetShadowArray(sys);
      strobe_calculatePWMValues(sys); //calculates the PSC and ARR values based on current speed (frequency and duty cycle of the strobe PWM signal)
      strobe_populateShadowArray(sys); //writes the PWM pattern for one rotation to shadow array for next HALL sensor pass
      sys->strobePWMCalculateRequired = 0; //clear the flag
    }
}

void strobe_PulseFinishedCallback(TIM_HandleTypeDef *htim){
    if(htim->Instance != TIM4){
		return; //not our timer
	}
    //this is where a lot of the strobe PWM magic happens. Quick breakdown:
    //stop the current PWM output
    //HAL_TIM_PWM_Stop_DMA(&htim4, TIM_CHANNEL_1); //stopping here causes a glitch, so we dont stop it
    
}

void strobe_updatePWMArray(system_interface *sys){
    //copy shadow array to active array
    for(int i = 0; i < 256; i++){
        sys->StrobePWMArray[i] = sys->shadowStrobePWMArray[i];
    }
}

void strobe_resetShadowArray(system_interface *sys){
    //reset the shadow PWM array
    for(int i = 0; i < 256; i++){
        sys->shadowStrobePWMArray[i] = 0;
    }
}

void strobe_calculatePWMValues(system_interface *sys){
    float pwmFrequency = 0.0;
    float sliceTime_s = 0.0;
    float calculateDenominator = 0.0;

    //sliceTime_s = sys->sliceTime_ms / 1000.0; //convert slice time to seconds - float for accuracy (important)
    sliceTime_s = sys->sliceTime_us / 1000000.0; //convert slice time to seconds - float for accuracy (important)
    pwmFrequency = 1.0 / sliceTime_s; //calculate desired PWM frequency in Hz
    calculateDenominator = (pwmFrequency) * sys->strobeARRValue; //calculate denominator for PSC calculation (not fully sure why the /2.0 is needed but it is)
    sys->strobePSCValue = sys->strobeSrcClockValue / calculateDenominator; //calculate PSC value
    if(sys->strobePSCValue > 0){
        sys->strobePSCValue -= 1; //subtract 1 from PSC value to account for zero indexing
    }

}

void strobe_applyPWMSettings(system_interface *sys){
    //apply the calculated PSC and ARR values to the timer
    TIM4->PSC = sys->strobePSCValue;
    TIM4->ARR = sys->strobeARRValue; //this is default 1000 (0.1% increments), sets on time per frame
    //TIM4->CCR1 = sys->strobeCCRValue; CCR is sent as a DMA array
}

void strobe_populateShadowArray(system_interface *sys){
    //populate the shadow array based on calculations and strobe mode
    switch(sys->strobeMode){
        case 0: //classic strobe mode
            strobe_populateShadowArray_classic(sys);
            break;

        default:
            strobe_populateShadowArray_classic(sys);
            break;
    }

}

void strobe_populateShadowArray_classic(system_interface *sys){
    //classic strobe mode - single pulse per frame, consistent
    for(uint8_t k=0; k<255; k++){
        //just fill whole array since we only send the frame count and we always reset all to 0 first
        sys->shadowStrobePWMArray[k] = sys->strobeCCRValue; //for classic mode, all values are the same
    }
}

void strobe_populateShadowArray_5Digit7Segment(system_interface *sys){
    //5 digit 7 segment display strobe mode 
    uint8_t digit1CurrentSlice = 0; //what currently exists in each digit space (physically)
    uint8_t digit2CurrentSlice = 0;
    uint8_t digit3CurrentSlice = 0;
    uint8_t digit4CurrentSlice = 0;
    uint8_t digit5CurrentSlice = 0;

    uint8_t digit1Check = 0; //flag if segment exists in displayed character - set strobe
    uint8_t digit2Check = 0;
    uint8_t digit3Check = 0;
    uint8_t digit4Check = 0;
    uint8_t digit5Check = 0;

    //105 slices per rotation for 7 segment display
    for(uint8_t k=0; k<255; k++){ 
        //need to check each slice position of the platter for each digit and determine if the strobe needs to be on or not

        //note which slice currently exists in each digit position
        digit1CurrentSlice = k + sys->displayCharacterOffset + 0; //offset will positin the clock display on the platter
        digit2CurrentSlice = k + sys->displayCharacterOffset + 1;
        digit3CurrentSlice = k + sys->displayCharacterOffset + 2;
        digit4CurrentSlice = k + sys->displayCharacterOffset + 3;
        digit5CurrentSlice = k + sys->displayCharacterOffset + 4;

        //digit1CurrentSlice = k + sys->displayCharacterOffset + 0; //offset will positin the clock display on the platter
        
        /*
        if(k > 3){
            digit1CurrentSlice = k + sys->displayCharacterOffset - 0;
            digit2CurrentSlice = k + sys->displayCharacterOffset - 1;
            digit3CurrentSlice = k + sys->displayCharacterOffset - 2;
            digit4CurrentSlice = k + sys->displayCharacterOffset - 3;
            digit5CurrentSlice = k + sys->displayCharacterOffset - 4;
        }else{
            if(k>2){
                digit1CurrentSlice = k + sys->displayCharacterOffset - 0;
                digit2CurrentSlice = k + sys->displayCharacterOffset - 1;
                digit3CurrentSlice = k + sys->displayCharacterOffset - 2;
                digit4CurrentSlice = k + sys->displayCharacterOffset - 3;
                //digit5CurrentSlice = k + sys->displayCharacterOffset - 4;
                digit5CurrentSlice = 104;
            }else{
                if(k>1){
                    digit1CurrentSlice = k + sys->displayCharacterOffset - 0;
                    digit2CurrentSlice = k + sys->displayCharacterOffset - 1;
                    digit3CurrentSlice = k + sys->displayCharacterOffset - 2;
                    digit4CurrentSlice = 104;
                    digit5CurrentSlice = 103;
                    
                    //digit4CurrentSlice = k + sys->displayCharacterOffset - 3;
                    //digit5CurrentSlice = k + sys->displayCharacterOffset - 4;
                }else{
                    if(k>0){
                        digit1CurrentSlice = k + sys->displayCharacterOffset - 0;
                        digit2CurrentSlice = k + sys->displayCharacterOffset - 1;
                        digit3CurrentSlice = 104;
                        digit4CurrentSlice = 103;
                        digit5CurrentSlice = 102;
                        //digit3CurrentSlice = k + sys->displayCharacterOffset - 2;
                        //digit4CurrentSlice = k + sys->displayCharacterOffset - 3;
                        //digit5CurrentSlice = k + sys->displayCharacterOffset - 4;                                                               
                    }else{
                        //first time through
                        digit1CurrentSlice = k + sys->displayCharacterOffset - 0;
                        digit2CurrentSlice = 104;
                        digit3CurrentSlice = 103;
                        digit4CurrentSlice = 102;
                        digit5CurrentSlice = 101;
                        //digit2CurrentSlice = k + sys->displayCharacterOffset - 1;
                        //digit3CurrentSlice = k + sys->displayCharacterOffset - 2;
                        //digit4CurrentSlice = k + sys->displayCharacterOffset - 3;
                        //digit5CurrentSlice = k + sys->displayCharacterOffset - 4; 
                    }
                }
            }
        }
        */
        
        
        //manage wrap around of slice positions
        if(digit1CurrentSlice >= sys->sliceCount){
            digit1CurrentSlice -= sys->sliceCount;
        }
        if(digit2CurrentSlice >= sys->sliceCount){
            digit2CurrentSlice -= sys->sliceCount;
        }
        if(digit3CurrentSlice >= sys->sliceCount){
            digit3CurrentSlice -= sys->sliceCount;
        }
        if(digit4CurrentSlice >= sys->sliceCount){
            digit4CurrentSlice -= sys->sliceCount;
        }
        if(digit5CurrentSlice >= sys->sliceCount){
            digit5CurrentSlice -= sys->sliceCount;
        }
        

        //reset check variables
        digit1Check = 0;
        digit2Check = 0;
        digit3Check = 0;
        digit4Check = 0;
        digit5Check = 0;

        if(k==3){
            digit1Check = 0; //debut
        }

        digit1Check = checkDigits(sys->digit1, digit1CurrentSlice, sys);
        digit2Check = checkDigits(sys->digit2, digit2CurrentSlice, sys);
        digit3Check = checkDigits(sys->digit3, digit3CurrentSlice, sys);
        digit4Check = checkDigits(sys->digit4, digit4CurrentSlice, sys);
        digit5Check = checkDigits(sys->digit5, digit5CurrentSlice, sys);

        //set strobe on or off for this frame
        if(digit1Check || digit2Check || digit3Check || digit4Check || digit5Check){
            if(k<sys->sliceCount){
                sys->shadowStrobePWMArray[k] = sys->strobeCCRValue; //set strobe ON for this slice
            }else{
                sys->shadowStrobePWMArray[k] = sys->strobeCCRValue; //set strobe OFF for this slice
                //sys->shadowStrobePWMArray[k] = sys->strobeCCRValue; //testing
            }
            
        }else{
            sys->shadowStrobePWMArray[k] = 0; //set strobe OFF for this slice
            //sys->shadowStrobePWMArray[k] = sys->strobeCCRValue; //this turns everything on - not truly what I want, just test
        }
    }
}

uint8_t segmentCheck(uint8_t seg, uint8_t slice, system_interface *sys){

    uint8_t segmentExists = 0;

    //segment is used in this character - check if slice has this segment
    switch(seg){
        //0b0ABCDEFG //character pattern for 7 segment display
        case 0: //segment g
            if(slice == sys->segmentLocations_G[0] || slice == sys->segmentLocations_G[1] || slice == sys->segmentLocations_G[2]){
                segmentExists = 1; //segment exists at this slice
            }
        break;

        case 1: //segment f
            if(slice == sys->segmentLocations_F[0] || slice == sys->segmentLocations_F[1] || slice == sys->segmentLocations_F[2]){
                segmentExists = 1; //segment exists at this slice
            }
        break;

        case 2: //segment e
            if(slice == sys->segmentLocations_E[0] || slice == sys->segmentLocations_E[1] || slice == sys->segmentLocations_E[2]){
                segmentExists = 1; //segment exists at this slice
            }
        break;

        case 3: //segment d
            if(slice == sys->segmentLocations_D[0] || slice == sys->segmentLocations_D[1] || slice == sys->segmentLocations_D[2]){
                segmentExists = 1; //segment exists at this slice
            }
        break;

        case 4: //segment c
            if(slice == sys->segmentLocations_C[0] || slice == sys->segmentLocations_C[1] || slice == sys->segmentLocations_C[2]){
                segmentExists = 1; //segment exists at this slice
            }
        break;

        case 5: //segment b
            if(slice == sys->segmentLocations_B[0] || slice == sys->segmentLocations_B[1] || slice == sys->segmentLocations_B[2]){
                segmentExists = 1; //segment exists at this slice
            }
        break;

        case 6: //segment a
            if(slice == sys->segmentLocations_A[0] || slice == sys->segmentLocations_A[1] || slice == sys->segmentLocations_A[2]){
                segmentExists = 1; //segment exists at this slice
            }
        break;
    }

    return segmentExists;
}

//checks if a given character has a segment at the given slice position - for five digit 7 segment display
uint8_t checkDigits(uint8_t character, uint8_t slice, system_interface *sys){

    uint8_t check = 0;

    uint8_t segmentExists = 0;
    
    //what character are we checking
    switch(character){
        case 0: //0
            //scan through all segments for a 0
            for(uint8_t k=0; k<7; k++){
                // bit_index: 0 = LSB, 7 = MSB
                check = (sys->character_0 & (1U << k)) != 0;
                if(check){
                    segmentExists += segmentCheck(k, slice, sys); //the += allows multiple segments to be checked without resetting and potententially erasing an old positive find
                }
            }
        break;

        case 1:
            //scan through all segments for a 1
            for(uint8_t k=0; k<7; k++){
                // bit_index: 0 = LSB, 7 = MSB
                check = (sys->character_1 & (1U << k)) != 0;
                if(check){
                    segmentExists += segmentCheck(k, slice, sys); //the += allows multiple segments to be checked without resetting and potententially erasing an old positive find
                }
            }
        break;

        case 2:
            //scan through all segments for a 2
            for(uint8_t k=0; k<7; k++){
                // bit_index: 0 = LSB, 7 = MSB
                check = (sys->character_2 & (1U << k)) != 0;
                if(check){
                    segmentExists += segmentCheck(k, slice, sys); //the += allows multiple segments to be checked without resetting and potententially erasing an old positive find
                }
            }
        break;

        case 3:
            //scan through all segments for a 3
            for(uint8_t k=0; k<7; k++){
                // bit_index: 0 = LSB, 7 = MSB
                check = (sys->character_3 & (1U << k)) != 0;
                if(check){
                    segmentExists += segmentCheck(k, slice, sys); //the += allows multiple segments to be checked without resetting and potententially erasing an old positive find
                }
            }
        break;

        case 4:
            //scan through all segments for a 4
            for(uint8_t k=0; k<7; k++){
                // bit_index: 0 = LSB, 7 = MSB
                check = (sys->character_4 & (1U << k)) != 0;
                if(check){
                    segmentExists += segmentCheck(k, slice, sys); //the += allows multiple segments to be checked without resetting and potententially erasing an old positive find
                }
            }
        break;

        case 5:
            //scan through all segments for a 5
            for(uint8_t k=0; k<7; k++){
                // bit_index: 0 = LSB, 7 = MSB
                check = (sys->character_5 & (1U << k)) != 0;
                if(check){
                    segmentExists += segmentCheck(k, slice, sys); //the += allows multiple segments to be checked without resetting and potententially erasing an old positive find
                }
            }
        break; 

        case 6:
            //scan through all segments for a 6
            for(uint8_t k=0; k<7; k++){
                // bit_index: 0 = LSB, 7 = MSB
                check = (sys->character_6 & (1U << k)) != 0;
                if(check){
                    segmentExists += segmentCheck(k, slice, sys); //the += allows multiple segments to be checked without resetting and potententially erasing an old positive find
                }
            }
        break;

        case 7:
            //scan through all segments for a7
            for(uint8_t k=0; k<7; k++){
                // bit_index: 0 = LSB, 7 = MSB
                check = (sys->character_7 & (1U << k)) != 0;
                if(check){
                    segmentExists += segmentCheck(k, slice, sys); //the += allows multiple segments to be checked without resetting and potententially erasing an old positive find
                }
            }
        break;

        case 8:
            //scan through all segments for a 8
            for(uint8_t k=0; k<7; k++){
                // bit_index: 0 = LSB, 7 = MSB
                check = (sys->character_8 & (1U << k)) != 0;
                if(check){
                    segmentExists += segmentCheck(k, slice, sys); //the += allows multiple segments to be checked without resetting and potententially erasing an old positive find
                }
            }
        break;

        case 9:
            //scan through all segments for a 9
            for(uint8_t k=0; k<7; k++){
                // bit_index: 0 = LSB, 7 = MSB
                check = (sys->character_9 & (1U << k)) != 0;
                if(check){
                    segmentExists += segmentCheck(k, slice, sys); //the += allows multiple segments to be checked without resetting and potententially erasing an old positive find
                }
            }
        break;

        case 10: //blank
            //blank will always have the strobe off
            segmentExists = 0;
        break;

        case 11: //dash
            //scan through all segments for a -
            for(uint8_t k=0; k<7; k++){
                // bit_index: 0 = LSB, 7 = MSB
                check = (sys->character_dash & (1U << k)) != 0;
                if(check){
                    segmentExists += segmentCheck(k, slice, sys); //the += allows multiple segments to be checked without resetting and potententially erasing an old positive find
                }
            }
        break;

        default: //blank
            //blank will always have the strobe off
            segmentExists = 0;
        break;
    }

    return segmentExists;
}

void strobe_timerPWMControl(system_interface *sys){
    //check if strobe is enabled
    if(sys->strobeEnabled){
        //strobe is enabled - start or restart PWM with DMA
        HAL_TIM_PWM_Start_DMA(&htim4, TIM_CHANNEL_1, (uint32_t *)sys->StrobePWMArray, sys->sliceCount);
    }else{
        //strobe is disabled - ensure PWM is stopped
        HAL_TIM_PWM_Start_DMA(&htim4, TIM_CHANNEL_1, (uint32_t *)0, 1);
        HAL_TIM_PWM_Stop_DMA(&htim4, TIM_CHANNEL_1); 
    }
}

//sets and controls motor control algorithm
void zoetrope_motorModeSelection(system_interface *sys){

    //all motor settings and functions here (previously in heartbeat)
    switch(sys->motorMode){
        case 0: //zero speed mode - motor can still be enabled, speed is just set to zero here
            
            TIM8->CCR1 = 0; //set PWM duty cycle to 0 (stop sending steps to motor)

            if(!sys->motor_enable){ //only allow changing motor settings if the motor is disabled
                sys->motor_direction = sys->motor_direction_switch;
                
                //remove this after the platter sensor sensor is implemented
                //search for NFC after motor has been disabled - jump to NFC search state
                sys->stateMachine_state = 10; //NFC search mode
                sys->nfcSuccess = 0; //clear NFC success flag
                sys->nfcSearchForTagFlag = 1;
                sys->nfcHallSensorReadCount = 0;
                sys->NFCDetectedFlag = 0;
                sys->NFCFailFlag = 0;
                sys->nfcSuccess = 0;
                //stateMachine_state 10 will reset speed to zero, setting the calculate variable here so the value is updated
                sys->platterRotationPeriod_us = 0;
                sys->platterRotationPeriod_ms = 0;
                sys->strobePWMCalculateRequired = 1; //flag to recalculate strobe PWM settings -- do in interrupt in future
                sys->demoCounter = 0; //reset demo counter
            }   

            break;
        case 1: //nfc search mode - specific spinup and speed for searching NFC tags

            zoetrope_NFCSearchMotorControl(sys);

            break;
        case 2: //high speed mode - specific spinup and speed for high speed operation
            //do something
            break;
        case 3: //nfc mode - motor speed settings are set from NFC read data - uses PID control
            //do something
            break;
        case 11: //old stepper mode
            //add current code here
            break;
        case 100: //testing mode - direct motor control from potentiometer, no PID
            //do something

            if(!sys->motor_enable){ //only allow changing motor settings if the motor is disabled
                sys->motor_direction = sys->motor_direction_switch;
                
                //remove this after the platter sensor sensor is implemented
                //search for NFC after motor has been disabled - jump to NFC search state
                sys->stateMachine_state = 10; //NFC search mode
                sys->nfcSuccess = 0; //clear NFC success flag
                sys->nfcSearchForTagFlag = 1;
                sys->nfcHallSensorReadCount = 0;
                sys->NFCDetectedFlag = 0;
                sys->NFCFailFlag = 0;
                sys->nfcSuccess = 0;
                //stateMachine_state 10 will reset speed to zero, setting the calculate variable here so the value is updated
                sys->platterRotationPeriod_us = 0;
                sys->platterRotationPeriod_ms = 0;
                sys->strobePWMCalculateRequired = 1; //flag to recalculate strobe PWM settings -- do in interrupt in future
                sys->demoCounter = 0; //reset demo counter
            }       

            //put moving and strobing variables behind the initialized variable
            if(sys->initialized){
                if(sys->normalMotorEnableSwitch){
                    sys->motor_enable = sys->motor_enable_switch;
                }
                 
                sys->sliceTime_ms = sys->sliceTime_us / 1000; //convert (we measure in uS now)
                if(sys->sliceTime_ms < 93){ //prevent seizures 
                    sys->strobeEnabled = sys->strobe_enable_switch;
                }else{
                    //disable the strobe if the slice time is too long
                    //exception - if the light is turned down low enough, allow strobe to function at low speeds
                    if(sys->strobeCCRValue < 6){
                        //allow the strobe to still function if strobe duty cycle is very low
                        sys->strobeEnabled = sys->strobe_enable_switch;
                    }else{
                        sys->strobeEnabled = 0;
                        strobe_timerPWMControl(sys); //start the strobe PWM output //TURN THE STROBE OFF
                    } 
                }

                //disable the strobe if the motor is disabled
                if(!sys->motor_enable){
                    sys->strobeEnabled = 0;
                    strobe_timerPWMControl(sys); //start the strobe PWM output //TURN THE STROBE OFF 
                }
                
            }else{
                sys->strobeEnabled = 0;
                sys->motor_enable = 0; 
                strobe_timerPWMControl(sys); //start the strobe PWM output //TURN THE STROBE OFF 
            }

            //read from the potentiometer - interrupt ADC mode
            HAL_StatusTypeDef HAL_Status_I2C_2 = HAL_OK;
            HAL_Status_I2C_2 = HAL_ADC_Start_IT(&hadc1); //start ADC in interrupt mode
            if(HAL_Status_I2C_2 != HAL_OK){
                //sys->system_state = 1; //set an error state if I2C write fails - this system state value should be updated to reflect ADC error
            }

            if(sys->newMotorControl){ //new control method
                uint16_t topRPM;
                uint16_t botRPM;

                if(sys->strobeMode == 1){
                    //5 digit 7 segment - fast range
                    topRPM = 900/2; //set the top RPM for testing mode
                    botRPM = 40/2;
                }else{
                    //classic normal range
                    topRPM = 300/2; //set the top RPM for testing mode
                    botRPM = 40/2;
                }

                //top speed testing mode
                //topRPM = 1100/2; //set the top RPM for testing mode
                //botRPM = 200/2;

                //desired testing RPM range: 50 - 300 RPM
                //to start, we could map the potentiometer reading directly to RPM value
                sys->setRPM = map(sys->motorSpeedPot, 0, 4092, botRPM, topRPM); 

                if(sys->normalMotorEnableSwitch){ //this keeps the motor from starting when the right button is used to hold the motor in place
                    //apply timer settings for set RPM
                    Zoetrope_Compute_TIM8_FromRPM(sys);
                }

            }else{ //old conrol method

                //if the motor is enabled and we are using a stepper driver, update the output speed PWM setting
                if((sys->motor_enable) && (sys->motorTMC2209Type) && (sys->normalMotorEnableSwitch)){
                    //convert the potentiometer reading (0-4092) to a PWM duty cycle value (0-100)
                    sys->PWMMotorSpeedValue = map(sys->motorSpeedPot, 0, 4092, 0, 4092); 
                    sys->PWMMotorSpeedValue = ((4092 + 160) - sys->PWMMotorSpeedValue); //inverted logic - lower pot value = higher speed
                    
                    TIM8->PSC = sys->PWMMotorSpeedValue;
                    TIM8->CCR1 = (TIM8->ARR / 2); //set PWM duty cycle to 50%
                    //enable the motor PWM timer

                }else{
                    //disale the motor PWM timer - set speed == 0
                    TIM8->CCR1 = 0; //set PWM duty cycle to 0 - may need to set this to 100% - since this is TIM8 CH1N
                    //for testing
                    sys->demoCounter = 0; //reset the demo counter for five digit seven segment display
                }
            }

            break;

        default:
            // do something - probably mode 0
            break;
    }
}

//motor PID control function
void motor_pidControl(system_interface *sys){
    //controls how quickly the speed changes as we approach the target speed

}

//stepper motor TIM control function
void motor_stepperTIMControl(system_interface *sys){
    //this function controls the timer register values to control the stp pin output

}

void zoetrope_stateMachine(system_interface *sys){
    //basic state machine for zoetrope operation
    //0 = idle
    //10 = NFC searching Mode (slow spin, looking for tag)
    //11 = NFC Tag Found Mode (pause motor, read tag, set parameters)
    //15 = NFC write tag mode (**********)
    //100 = normal (non-NFC) operation mode (strobe running, motor running; all strobe and motor functions decided here)
    //101 = normal (NFC settings) operation mode (strobe running, motor running; all strobe and motor functions decided here)
    //200 = spindown mode (strobe off, motor spindown; mandatory for high speed platters)
    //1000 = error state

    switch(sys->stateMachine_state){
        case 0: //idle state
            //add code here
            break;
        
        case 1: //initialization (reseting state)
            //add code here
            break;

        case 10: //NFC searching Mode
            sys->motorMode = 1; //NFC search mode
            zoetrope_motorModeSelection(sys); //rotate motor at a slow speed looing for nfc tag
            zoetrope_hardwareRuntime(sys); //zoetrope system buttons, encoder, neopixels, stepper fan, heartbeat
            zoetrope_esp32DisplayRuntime(sys); //esp32 displays update functions

            //scan for NFC tag - if motor is enabled
            if(sys->motor_enable){
                zoetrope_scanNFCForTag(sys, &pn532); //if this gives weird behavior, I may need to pass the pn532 struct into this function rather tha point to it
                //poll HALL sensor to count the rotations while we search for NFC tag
                //hall1_readAndMeasure(sys); //read HALL sensor and measure period - NOW DONE IN INTERRUPT
                //I'm not going to worry about speed updates since the platter spins at constant speed in this mode
                //when the HALL sensor is seen the strobePWMCalculateRequired flag is set by the above function
                //I will use that (for now) to count the number of rotations while searching for NFC tag
                if(sys->strobePWMCalculateRequired){ //new hall sensor read detected

                    sys->strobePWMCalculateRequired = 0; //clear the flag

                    //increment the NFC hall sensor read count
                    sys->nfcHallSensorReadCount++;
                    //check if we need to give up on nfc searching
                    if(sys->nfcHallSensorReadCount > sys->nfcHallSensorReadTarget){ //not >= becuase we enable this variable when flipping the switch to reset rotation time to 0
                        //give up on searching for NFC tag
                        sys->stateMachine_state = 100; //move to normal operation (testing) mode
                    }
                }
            }else{
                //motor is not enabled - reset some variables
                sys->NFCDetectedFlag = 0; //clear NFC detected flag
                sys->blueLED = 0; //turn off blue LED to indicate NFC tag not found
                //sys->strobePWMCalculateRequired = 1; //flag to recalculate strobe PWM settings -- do in interrupt in future
                sys->nfcHallSensorReadCount = 0; //clear hall sensor read count for NFC scanning
                //reset speed
                sys->platterRotationPeriod_us = 0;
                sys->platterRotationPeriod_ms = 0;
                zoetrope_calculateFramePeriod(sys); //recalculate frame period (set to 0)
            }

            if(sys->NFCDetectedFlag == 0){
                //NFC tag is not detected yet - keep spinning the motor
                sys->blueLED = 0; //turn off blue LED to indicate NFC tag not found
            }else{
                //NFC tag detected - stop the motor and change modes
                sys->blueLED = 1; //turn on blue LED to indicate NFC tag found
                sys->NFCDetectedFlag = 0; //clear NFC detected flag - rescan right before reading again
                sys->stateMachine_state = 11; //move to NFC tag found mode
                sys->nfcSuccess = 0; //clear NFC success flag for reading
            }
            break;

        case 11: //NFC Tag Found Mode
            sys->motorMode = 0; //hold motor in one spot
            sys->nfcSuccess = 0; //clear NFC success flag for reading
            sys->NFCDetectedFlag = 1; //set NFC detected flag
            //we stay in this loop either until we successfully read the NFC tag or the read fails (NFCDetectedFlag cleared)
            while((sys->nfcSuccess == 0) && (sys->NFCDetectedFlag == 1)){
                //read the NFC tag data
                sys->blueLED = 1; //keep blue LED on to indicate NFC tag reading in progress
                zoetrope_motorModeSelection(sys); //rotate motor at a slow speed looing for nfc tag
                zoetrope_hardwareRuntime(sys); //zoetrope system buttons, encoder, neopixels, stepper fan, heartbeat
                zoetrope_esp32DisplayRuntime(sys); //esp32 displays update functions

                //write to the NFC tag and read the data - for testing, will change to reading only later
                //zoetrope_WriteNFCTag(sys, &pn532); //write data to NFC - USED for setting up new platters (testing monde right now, until I make its own settings page)
                zoetrope_ReadNFCTag(sys, &pn532); //read data from NFC AND apply settings
            }

            sys->blueLED = 0; //turn off blue LED to indicate NFC tag read complete
            sys->redLED = 0; //turn off red LED to indicate no error (for now)
            //FOR NOW - just enter general mode
            sys->stateMachine_state = 100; //move to normal operation mode - change to NFC settings mode later

            break;

        case 15: //NFC write tag mode
            
            sys->motorMode = 0; //hold motor in one spot
            while(sys->nfcSuccess == 0){
                //read the NFC tag data
                sys->blueLED = 1; //keep blue LED on to indicate NFC tag reading in progress
                zoetrope_motorModeSelection(sys); //rotate motor at a slow speed looing for nfc tag
                zoetrope_hardwareRuntime(sys); //zoetrope system buttons, encoder, neopixels, stepper fan, heartbeat
                zoetrope_esp32DisplayRuntime(sys); //esp32 displays update functions

                //write to the NFC tag and read the data - for testing, will change to reading only later
                //zoetrope_WriteNFCTag(sys, &pn532); //write data to NFC - USED for setting up new platters (testing monde right now, until I make its own settings page)
                zoetrope_ReadNFCTag(sys, &pn532); //read data from NFC AND apply settings

            }

            sys->blueLED = 0; //turn off blue LED to indicate NFC tag read complete
            sys->redLED = 0; //turn off red LED to indicate no error (for now)
            //FOR NOW - just enter general mode
            sys->stateMachine_state = 100; //move to normal operation mode - change to NFC settings mode later

            break;

        case 100: //normal (testing) operation mode
            
            sys->motorMode = 100; //motorMode 100 wil switch speed range automatically for strobe mode 0 and 1
            
            zoetrope_hardwareRuntime(sys); //zoetrope system buttons, encoder, neopixels, stepper fan, heartbeat
            zoetrope_esp32DisplayRuntime(sys); //esp32 displays update functions
            //sets strobe mode data and polls the HALL sensor (for now, move the HALL to an interrupt)
            zoetrope_strobeModeSelection(sys); //strobe mode selection
            zoetrope_motorModeSelection(sys); //motor mode selection -used to be in heartbeat

            //poll the HALL sensor
            hall1_readAndMeasure(sys); 

            break;

        case 101: //normal (NFC settings) operation mode
            //add code here
            break;

        case 200: //spindown mode
            //add code here
            break;

        case 1000: //error state 1 mode
            //add code here
            break;
        
        default:
            //error state
            break;
    }
}

void zoetrope_NFCSearchMotorControl(system_interface *sys){
    //controls motor while looking for NFC tag mode

    if(!sys->motor_enable){ //only allow changing motor settings if the motor is disabled
        sys->motor_direction = sys->motor_direction_switch;
    }       

    sys->strobeEnabled = 0;
    strobe_timerPWMControl(sys); //start the strobe PWM output //TURN THE STROBE OFF

    //put moving and strobing variables behind the initialized variable
    if(sys->initialized){
        if(sys->normalMotorEnableSwitch){
            sys->motor_enable = sys->motor_enable_switch; 
        }
    }else{
        sys->strobeEnabled = 0;
        sys->motor_enable = 0; 
        strobe_timerPWMControl(sys); //start the strobe PWM output //TURN THE STROBE OFF 
    }

    //if the motor is enabled and we are using a stepper driver, update the output speed PWM setting
    if((sys->motor_enable) && (sys->motorTMC2209Type) && (sys->normalMotorEnableSwitch)){

        TIM8->PSC = 2600; //3500
        TIM8->CCR1 = (TIM8->ARR / 2); //set PWM duty cycle to 50%
        TIM8->ARR = 99;
        //enable the motor PWM timer

    }else{
        //disale the motor PWM timer - set speed == 0
        TIM8->CCR1 = 0; //set PWM duty cycle to 0 - may need to set this to 100% - since this is TIM8 CH1N
    }
    
}

void zoetrope_scanNFCForTag(system_interface *sys, PN532 *pn532){
    //scans for an NFC tag - to be implemented
    if(sys->NFCDetectedFlag == 0){
    // Check if a card is available to read
    sys->uid_len = PN532_ReadPassiveTarget(pn532, sys->uid, PN532_MIFARE_ISO14443A, 1000);
    if (sys->uid_len == PN532_STATUS_ERROR) {
      //printf(".");
    } else {
      //printf("Found card with UID: ");
      for (uint8_t i = 0; i < sys->uid_len; i++) {
        //printf("%02x ", uid[i]);
      }
      sys->NFCDetectedFlag = 1; //NFC sticker detected
      sys->NFCFailFlag = 0;
    }
  }
}

void zoetrope_ReadNFCTag(system_interface *sys, PN532 *pn532){

    if(sys->NFCDetectedFlag == 0){
        sys->NFCFailFlag = 0; //reset fail flag
        // Check if a card is available to read
        sys->uid_len = PN532_ReadPassiveTarget(pn532, sys->uid, PN532_MIFARE_ISO14443A, 1000);
        if (sys->uid_len == PN532_STATUS_ERROR) {
            //printf(".");
            sys->redLED = 1;
        } else {
            //printf("Found card with UID: ");
            for (uint8_t i = 0; i < sys->uid_len; i++) {
                //printf("%02x ", uid[i]);
            }
            sys->NFCDetectedFlag = 1; //NFC sticker detected
            sys->NFCFailFlag = 0;                                           
        }       
    }

    if(sys->NFCDetectedFlag){ //read to sticker and read back to confirm good write
        sys->nfcUserStartingblockNumber = 6; //start writing at page 6

        zoetrope_resetNFCReadBuffer(sys); // clear read buffer prior to reading back data
        for(uint8_t k=0; k<sys->nfcDataReadV1_pageCount; k++){
            //read one page at a time
            sys->pn532_error1 = PN532_Ntag2xxReadBlock(pn532, sys->DATA2, (sys->nfcUserStartingblockNumber + k));
            if (sys->pn532_error1) {
                sys->NFCFailFlag = 1;
            }
            sys->NFCReadBuffer[sys->nfcUserStartingblockNumber + k][0] = sys->DATA2[0];
            sys->NFCReadBuffer[sys->nfcUserStartingblockNumber + k][1] = sys->DATA2[1];
            sys->NFCReadBuffer[sys->nfcUserStartingblockNumber + k][2] = sys->DATA2[2];
            sys->NFCReadBuffer[sys->nfcUserStartingblockNumber + k][3] = sys->DATA2[3];
            //reset DATA2 for next read
            sys->DATA2[0] = 0x00;
            sys->DATA2[1] = 0x00;
            sys->DATA2[2] = 0x00;
            sys->DATA2[3] = 0x00;               
        }

        if(sys->NFCFailFlag == 0){
            //successful read and write
            sys->blueLED = 1; //turn on blue LED to indicate success
            zoetrope_applyNFCSettings(sys); //apply the read NFC settings to the system
            sys->nfcSuccess = 1; //I will need to reset this after a platter has changed
        }else{
            sys->blueLED = 0; //turn off blue LED to indicate fail
            sys->nfcSuccess = 0;
            sys->redLED = 1; //turn on red LED to indicate error
            //bounce out of read state and back to searching state
            sys->NFCFailFlag = 0; //reset fail flag for next read
            sys->nfcHallSensorReadCount = 0; //clear hall sensor read count for NFC scanning
            sys->NFCDetectedFlag = 0; //clear NFC detected flag
            sys->stateMachine_state = 10; //back to NFC searching mode

        }
        //reset flag
        sys->NFCDetectedFlag = 0; 
        //reset data to have confidence in testing
        sys->DATA2[0] = 0x00;
        sys->DATA2[1] = 0x00;
        sys->DATA2[2] = 0x00;
        sys->DATA2[3] = 0x00;
    }
}

void zoetrope_WriteNFCTag(system_interface *sys, PN532 *pn532){
    
    if(sys->NFCDetectedFlag == 0){
        // Check if a card is available to read
        sys->uid_len = PN532_ReadPassiveTarget(pn532, sys->uid, PN532_MIFARE_ISO14443A, 1000);
        if (sys->uid_len == PN532_STATUS_ERROR) {
            //printf(".");
            sys->redLED = 1;
        } else {
            //printf("Found card with UID: ");
            for (uint8_t i = 0; i < sys->uid_len; i++) {
                //printf("%02x ", uid[i]);
            }
            sys->NFCDetectedFlag = 1; //NFC sticker detected
            sys->NFCFailFlag = 0;
        }
    }

  if(sys->NFCDetectedFlag){ //write to sticker and read back to confirm good write
    sys->nfcUserStartingblockNumber = 6; //start writing at page 6
    zoetrope_setNFCWriteBuffer(sys); //setup buffer we write to the NFC sticker - ITS ONLY AN BASIC PATTERN RIGHT NOW FOR TESTING
    for(uint8_t k=0; k<sys->nfcDataReadV1_pageCount; k++){
        //write one page at a time
        sys->DATA[0] = sys->NFCWriteBuffer[sys->nfcUserStartingblockNumber + k][0];
        sys->DATA[1] = sys->NFCWriteBuffer[sys->nfcUserStartingblockNumber + k][1];
        sys->DATA[2] = sys->NFCWriteBuffer[sys->nfcUserStartingblockNumber + k][2];
        sys->DATA[3] = sys->NFCWriteBuffer[sys->nfcUserStartingblockNumber + k][3];
        sys->pn532_error1 = PN532_Ntag2xxWriteBlock(pn532, sys->DATA, (sys->nfcUserStartingblockNumber + k));
        if (sys->pn532_error1) {
            sys->NFCFailFlag = 1;
        }
    }
    
    //zoetrope_resetNFCReadBuffer(sys); // clear read buffer prior to reading back data
    for(uint8_t k=0; k<sys->nfcDataReadV1_pageCount; k++){
        //read one page at a time
        sys->pn532_error1 = PN532_Ntag2xxReadBlock(pn532, sys->DATA2, (sys->nfcUserStartingblockNumber + k));
        if (sys->pn532_error1) {
            sys->NFCFailFlag = 1;
        }
        sys->NFCReadBuffer[sys->nfcUserStartingblockNumber + k][0] = sys->DATA2[0];
        sys->NFCReadBuffer[sys->nfcUserStartingblockNumber + k][1] = sys->DATA2[1];
        sys->NFCReadBuffer[sys->nfcUserStartingblockNumber + k][2] = sys->DATA2[2];
        sys->NFCReadBuffer[sys->nfcUserStartingblockNumber + k][3] = sys->DATA2[3];
        //reset DATA2 for next read
        sys->DATA2[0] = 0x00;
        sys->DATA2[1] = 0x00;
        sys->DATA2[2] = 0x00;
        sys->DATA2[3] = 0x00;
    }

    //confirm good data read
    for(uint8_t k=0; k<sys->nfcDataReadV1_pageCount; k++){

        //check each page 1 at a time
        sys->DATA[0] = sys->NFCWriteBuffer[sys->nfcUserStartingblockNumber + k][0];
        sys->DATA[1] = sys->NFCWriteBuffer[sys->nfcUserStartingblockNumber + k][1];
        sys->DATA[2] = sys->NFCWriteBuffer[sys->nfcUserStartingblockNumber + k][2];
        sys->DATA[3] = sys->NFCWriteBuffer[sys->nfcUserStartingblockNumber + k][3];

        sys->DATA2[0] = sys->NFCReadBuffer[sys->nfcUserStartingblockNumber + k][0];
        sys->DATA2[1] = sys->NFCReadBuffer[sys->nfcUserStartingblockNumber + k][1];
        sys->DATA2[2] = sys->NFCReadBuffer[sys->nfcUserStartingblockNumber + k][2];
        sys->DATA2[3] = sys->NFCReadBuffer[sys->nfcUserStartingblockNumber + k][3];

        //check data for good read
        for (uint8_t i = 0; i < sizeof(sys->DATA); i++) {
            if (sys->DATA[i] != sys->DATA2[i]) {
                sys->NFCFailFlag = 1;
            }
        }
    }//end confirm good data read

    if(sys->NFCFailFlag == 0){
      //successful read and write
      sys->blueLED = 1; //turn on blue LED to indicate success
      sys->nfcSuccess = 1; //I will need to reset this after a platter has changed
    }else{
      sys->blueLED = 0; //turn off blue LED to indicate fail
      sys->nfcSuccess = 0;
      sys->redLED = 1; //turn on red LED to indicate error
    }
    //reset flag
    sys->NFCDetectedFlag = 0; 
    //reset data to have confidence in testing
    sys->DATA2[0] = 0x00;
    sys->DATA2[1] = 0x00;
    sys->DATA2[2] = 0x00;
    sys->DATA2[3] = 0x00;
  }


}

void zoetrope_resetNFCReadBuffer(system_interface *sys){
  for(uint8_t k=0; k<126; k++){
    sys->NFCReadBuffer[k][0] = 0x00;
    sys->NFCReadBuffer[k][1] = 0x00;
    sys->NFCReadBuffer[k][2] = 0x00;
    sys->NFCReadBuffer[k][3] = 0x00;
  }
}

void zoetrope_setNFCWriteBuffer(system_interface *sys){
    for(uint8_t k=0; k<126; k++){
        sys->NFCWriteBuffer[k][0] = k; //just an example pattern
        sys->NFCWriteBuffer[k][1] = k;
        sys->NFCWriteBuffer[k][2] = k;
        sys->NFCWriteBuffer[k][3] = k;
    }

    //deliberate data setting here

    //NFC version and page count (page 6)
    sys->NFCWriteBuffer[6][0] = 0x00; //NFC data minor version number
    sys->NFCWriteBuffer[6][1] = 0x01; //NFC data major version number
    sys->NFCWriteBuffer[6][2] = 10; //number of pages to read for NFC (V1) data
    sys->NFCWriteBuffer[6][3] = 0x00; //not used yet

    //platter revision information (page 7)
    sys->NFCWriteBuffer[7][0] = 0x00; //platter settings minor version number
    sys->NFCWriteBuffer[7][1] = 0x01; //platter settings major version number
    sys->NFCWriteBuffer[7][2] = 0x00; //platter physical minor revision 
    sys->NFCWriteBuffer[7][3] = 0x00; //platter physical major revision

    //motor top and bottom speed range (page 8)
    sys->NFCWriteBuffer[8][0] = 0x00; //motor min frequency (LSB)
    sys->NFCWriteBuffer[8][1] = 0x00; //motor min frequency (MSB)
    sys->NFCWriteBuffer[8][2] = 0x00; //motor max frequency (LSB)
    sys->NFCWriteBuffer[8][3] = 0x00; //motor max frequency (MSB)

    //ideal motor speed (potentiometer center point value) (page 9)
    sys->NFCWriteBuffer[9][0] = 0x00; //ideal motor frequency (LSB)
    sys->NFCWriteBuffer[9][1] = 0x00; //ideal motor frequency (MSB)
    sys->NFCWriteBuffer[9][2] = 0x00; //not used yet
    sys->NFCWriteBuffer[9][3] = 0x00; //not used yet

    //frame_1 strobe settings (page 10)
    sys->NFCWriteBuffer[10][0] = 20; //frame count (slice count)
    sys->NFCWriteBuffer[10][1] = 0x00; //strobe mode -0=classic mode, 1=5digit7seg mode
    sys->NFCWriteBuffer[10][2] = 0x00; //ON time (CCR value) LSB
    sys->NFCWriteBuffer[10][3] = 0x00; //ON time (CCR value) MSB

    //frame_2 strobe settings (page 11)
    sys->NFCWriteBuffer[11][0] = 0x00; //rotation direction (0 = CCW, 1 = CW)
    sys->NFCWriteBuffer[11][1] = 0x00; //not used yet
    sys->NFCWriteBuffer[11][2] = 0x00; //not used yet
    sys->NFCWriteBuffer[11][3] = 0x00; //not used yet

    //PID_P parameter (page 12)
    sys->NFCWriteBuffer[12][0] = 0x00; //
    sys->NFCWriteBuffer[12][1] = 0x00; //
    sys->NFCWriteBuffer[12][2] = 0x00; //
    sys->NFCWriteBuffer[12][3] = 0x00; //

    //PID_I parameter (page 13)
    sys->NFCWriteBuffer[13][0] = 0x00; //
    sys->NFCWriteBuffer[13][1] = 0x00; //
    sys->NFCWriteBuffer[13][2] = 0x00; //
    sys->NFCWriteBuffer[13][3] = 0x00; //

    //PID_D parameter (page 14)
    sys->NFCWriteBuffer[14][0] = 0x00; //
    sys->NFCWriteBuffer[14][1] = 0x00; //
    sys->NFCWriteBuffer[14][2] = 0x00; //
    sys->NFCWriteBuffer[14][3] = 0x00; //

    //brightness parameter (page 15)
    sys->NFCWriteBuffer[15][0] = 0x00; //
    sys->NFCWriteBuffer[15][1] = 0x00; //
    sys->NFCWriteBuffer[15][2] = 0x00; //
    sys->NFCWriteBuffer[15][3] = 0x00; //

    //reserved for future use (page 16)
    sys->NFCWriteBuffer[16][0] = 0x00; //
    sys->NFCWriteBuffer[16][1] = 0x00; //
    sys->NFCWriteBuffer[16][2] = 0x00; //
    sys->NFCWriteBuffer[16][3] = 0x00; //

    //adding any more data, will need to update the page count variable
}


void zoetrope_applyNFCSettings(system_interface *sys){

    //apply settings read from NFC to system variables
    sys->sliceCount = sys->NFCReadBuffer[10][0]; //read slice count from NFC data page 10 byte 0

    //we need to pull the strobe mode from NFC data as well
    sys->strobeMode = sys->NFCReadBuffer[10][1]; //strobe mode is frame 10 byte 1

    //add more as we use more settings
    //I should write out a full list of settings and their locations for easier reference

}

void zoetrope_HALLSensorInterruptHandler2(system_interface *sys){
    uint32_t currentTime = TIM5->CNT; //check microsecond timer

    if(currentTime > 1000000){
        sys->hallSensor1Period_us = currentTime - sys->hallSensor1LastTime_us;
        sys->platterRotationPeriod_us = sys->hallSensor1Period_us;
        zoetrope_calculateFramePeriod(sys);
        sys->strobePWMCalculateRequired = 1;
        TIM5->CNT = 0;
        sys->hallSensor1LastTime_us = 0;
    }
}

void zoetrope_HALLSensorInterruptHandler(system_interface *sys){

    uint32_t currentTime_us = TIM5->CNT; //check microsecond timer
    //uint32_t currentTime_ms = HAL_GetTick(); //note current time coming into function - used for period measurement
    //uint8_t prevhallSensor1State = 0;

    //SHOULDNT NEED TO TRACK STATE, SINCE THIS IS RISING EDGE TRIGGER (STATE IS INCLUDED IN THE INTERRUPT)
    //note the current sensor state
    //prevhallSensor1State = sys->hallSensor1;
    //read the state of hall sensor 1 and assign the system variable
    //sys->hallSensor1 = HAL_GPIO_ReadPin(GPIOC, hall1_Pin);

    //see if the state of the sensor has changed
    //if(sys->hallSensor1 != prevhallSensor1State){
        //sensor state has changed
        //if(sys->hallSensor1 == 1){ //rising edge detected
            //calculate the period since the last rising edge
            sys->hallSensor1Period_us = currentTime_us - sys->hallSensor1LastTime_us;
            if(sys->platterRotationPeriod_us < 1000000){
                //are we spinning at least this fast?

                if(sys->platterRotationPeriod_us < 300000){
                    if(sys->hallSensor1Period_us > 50000){ //debounce the sensor a bit
                        //sys->hallSensor1LastTime_us = currentTime_us; //update the last time variable
                        sys->platterRotationPeriod_us = sys->hallSensor1Period_us; //for now, platter period == hall sensor period
                        zoetrope_calculateFramePeriod(sys); //calculate the frame pulse period using system variables
                        sys->strobePWMCalculateRequired = 1; //flag to recalculate strobe PWM settings -- do in interrupt in future
                        //reset tim5 in case that is my problem
                        TIM5->CNT = 0;
                        sys->hallSensor1LastTime_us = 0;
                    }
                }else{
                    //need a bigger debounce time at medium speeds
                    if(sys->hallSensor1Period_us > 200000){ //debounce the sensor a bit
                        //sys->hallSensor1LastTime_us = currentTime_us; //update the last time variable
                        sys->platterRotationPeriod_us = sys->hallSensor1Period_us; //for now, platter period == hall sensor period
                        zoetrope_calculateFramePeriod(sys); //calculate the frame pulse period using system variables
                        sys->strobePWMCalculateRequired = 1; //flag to recalculate strobe PWM settings -- do in interrupt in future
                        //reset tim5 in case that is my problem
                        TIM5->CNT = 0;
                        sys->hallSensor1LastTime_us = 0;
                    }
                }
                
            }else{
                //need a bigger debounce time at low speeds
                if(sys->hallSensor1Period_us > 750000){ //debounce the sensor a bit
                    //sys->hallSensor1LastTime_us = currentTime_us; //update the last time variable
                    sys->platterRotationPeriod_us = sys->hallSensor1Period_us; //for now, platter period == hall sensor period
                    zoetrope_calculateFramePeriod(sys); //calculate the frame pulse period using system variables
                    sys->strobePWMCalculateRequired = 1; //flag to recalculate strobe PWM settings -- do in interrupt in future
                    //reset tim5 in case that is my problem
                    TIM5->CNT = 0;
                    sys->hallSensor1LastTime_us = 0;
                }
            }
            
            
        //}
    //}

}

void zoetrope_stepperMotorMicroStepConfig(system_interface *sys){

    switch (sys->microstepSetting){
        case 0: //1/8 step
            //MS1 = LOW
            //MS2 = LOW
            sys->motor_MS1 = 0;
            sys->motor_MS2 = 0;
            break;
        case 1: //1/16 step
            //MS1 = HIGH
            //MS2 = HIGH
            sys->motor_MS1 = 1;
            sys->motor_MS2 = 1;
            break;
        case 2: //1/32 step
            //MS1 = HIGH
            //MS2 = LOW
            sys->motor_MS1 = 1;
            sys->motor_MS2 = 0;
            break;      
        case 3: //1/64 step
            //MS1 = LOW
            //MS2 = HIGH
            sys->motor_MS1 = 0;
            sys->motor_MS2 = 1;
            break;
    }

}

void Zoetrope_Compute_TIM8_FromRPM(system_interface *sys){

    //for if set speed is 0 or less
    if (sys->setRPM <= 0.0f) {
        sys->TIM8_PSC = 0;
        sys->TIM8_ARR = 0;
        sys->TIM8_CCR = 0;
        sys->TIM8_StepFrequency = 0.0f;
        return; //leave the function
    }

    // Step frequency in Hz
    float step_freq = (sys->setRPM * sys->STEPS_PER_REV) / 60.0f;
    sys->TIM8_StepFrequency = step_freq;

    uint32_t tim_clk = sys->TIM8_CLK_HZ;
    uint32_t psc = 0;
    uint32_t arr;

    // Find smallest PSC that gives ARR within range
    while (1) { //hopefully this while loop doesnt cause issues????
        arr = (uint32_t)((tim_clk / ((psc + 1) * step_freq)) - 1);

        if (arr <= sys->TIM_MAX_ARR) {
            break;
        }

        psc++;
        if (psc > sys->TIM_MAX_ARR) {
            // Cannot generate this slow frequency
            arr = sys->TIM_MAX_ARR;
            break;
        }
    }

    if(sys->normalMotorEnableSwitch){
        sys->TIM8_PSC = (uint16_t)psc;
        sys->TIM8_ARR = (uint16_t)arr;
        sys->TIM8_CCR = sys->TIM8_ARR / 2;  // 50% duty

        TIM8->PSC = sys->TIM8_PSC;
        TIM8->ARR = sys->TIM8_ARR;
        TIM8->CCR1 = sys->TIM8_CCR;
    }

}