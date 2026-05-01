#ifndef machine_interface_H
#define machine_interface_H

#include <stdint.h>
#include "stm32f4xx_hal.h"    // HAL types and functions
#include "drivers/TCA9534.h" //delete this line later?
#include "main.h" //for GPIO pins
#include "stm32f4xx_hal_tim.h"
#include "drivers/pn532.h"
#include "drivers/pn532_HAL.h"

//extern system_interface zoetrope; //main system instance
extern TCA9534_instance TCA_port1; //I2C I/O expander instances
extern TCA9534_instance TCA_port2;
extern TCA9534_instance TCA_port3;

extern PN532 pn532; //NFC instance

extern TIM_HandleTypeDef htim4; //strobe PWM timer

extern ADC_HandleTypeDef hadc1; //start ADC in interrupt mode
//extern TIM_HandleTypeDef htim2; //motor PWM step output - used to be TIM2, moved to TIM8
//extern TIM_HandleTypeDef htim8; //motor PWM step output - is TIM8 needed?

//struct 
typedef struct {

    ////////////////////////////////////////////////////////////////////
    //variables in this section are directly connected to hardware/////
    //////////////////////////////////////////////////////////////////

    //I2C IO expander variables - update triggered via GPIO interrupt, subsequent I2C read
    //U3 variables (include the interrupt later)
    uint8_t motor_enable;       //P0        ||  TMC2209 enable pin state - active LOW
    uint8_t motor_MS2;          //P1        ||  TMC2209 MS2 pin state
    uint8_t motor_spreadCycle;  //P2        ||  TMC2209 spreadCycle pin state
    uint8_t motor_direction;    //P3        ||  motor direction control
    uint8_t motor_MS1;          //P4        ||  TMC2209 MS1 pin state
    uint8_t esp32_2_ext1;       //P5        ||  ESP32 (round) connection 1
    uint8_t esp32_2_ext2;       //P6        ||  ESP32 (round) connection 2
    uint8_t U3_hearbeat;        //P7        ||  green smd 0603 LED
    uint8_t U3_interruptFlag;  //interrupt flag for U3 I2C I/O expander
    //U7 variables (include the interrupt later)
    uint8_t strobe_enable_switch; //P0      ||  strobe enable switch input
    uint8_t motor_enable_switch; //P1       ||  motor enable switch input
    uint8_t motor_direction_switch; //P2    ||  motor direction switch input
    uint8_t U7_P3; //P3                     ||  unused - connection on bottom of board
    uint8_t U7_P4; //P4                     ||  unused - connection on bottom of board
    uint8_t U7_P5; //P5                     ||  unused - connection on bottom of board
    uint8_t U7_P6; //P6                     ||  unused - connection on bottom of board
    uint8_t U7_hearbeat; //P7               ||  green smd 0603 LED
    uint8_t U7_interruptFlag;  //interrupt flag for U7 I2C I/O expander
    //U10 variables (include the interrupt later)
    uint8_t button_left;    //P0            ||  left pushbutton input
    uint8_t esp32_1_ext1;   //P1            ||  ESP32 (rectangular) connection 1
    uint8_t button_encoder; //P2            ||  rotary encoder pushbutton input
    uint8_t esp32_1_ext2;   //P3            ||  ESP32 (rectangular) connection 2
    uint8_t button_right;   //P4            ||  right pushbutton input
    uint8_t fan2_enable;    //P5            ||  fan 2 enable control
    uint8_t fan1_enable;    //P6            ||  fan 1 enable control
    uint8_t U10_hearbeat;   //P7            ||  green smd 0603 LED
    uint8_t U10_interruptFlag;  //interrupt flag for U10 I2C I/O expander

    //prev variables used for debouncing and interface
    uint8_t prev_button_left;
    uint8_t prev_button_right;
    uint8_t prev_button_encoder;

    //GPIO controlled outputs
    //0603 LEDs with GPIO control
    uint8_t greenLED; //this will likely be the heartbeat LED
    uint8_t redLED;
    uint8_t blueLED;

    //hall sensor inputs (GPIO input) - update via interrupt
    uint8_t hallSensor1; //rpm measurement - platter coupler magnet
    uint8_t hallSensor2; //platter index sensor - for special platters, platform sensor (not implemented yet)
    uint8_t hallSensor3; //platter ID sensor - for reading in the platter ID, platform sensor (concept, not implemented yet)
    uint8_t hallSensor4; //spare

    uint8_t platterSensor; //platter sensor input - not implemented (optical reflective sensor)
    
    uint8_t strobePinState; //current state of the strobe pin

    //updated via ADC read
    uint16_t motorSpeedPot; //potentiometer raw value

    //updated via timer interrupt
    uint32_t encoderTimerCount; //timer count value for rotary encoder
    uint32_t prevEncoderTimerCount; //previous timer count value for rotary encoder
    uint8_t encoderPulseDifferenceCalculated; //flag to indicate difference calculated

    //temperature variables
    uint16_t tempSensor1; //temperature sensor 1 value - not implemented
    uint16_t tempSensor2; //temperature sensor 2 value - not implemented

    //ambient light variables
    uint16_t ambientLightLevel1; //ambient light sensor value - I2C sensor - not implemented
    uint16_t ambientLightLevel2; //ambient light sensor value - traditional ADC type - not implementd

    ////////////////////////////////////////////////////////
    ////////////System Control Variables////////////////////
    //////The system runs based on these variables//////////
    ///the preceding variables are connected to hardware////
    ////////////////////////////////////////////////////////

    //heart beat pulse time - also used for I2C I/O output updates
    uint32_t heartbeatTime_ms; //time between heartbeats in milliseconds
    uint32_t heartbeatCurrentTime_ms; //current time for heartbeat tracking
    uint32_t prevHeartbeatTime_ms;    //previous time for heartbeat tracking
    uint8_t heartbeatState; //current state of heartbeat
    uint8_t heartbeatLEDEnabled; //heartbeat LED enabled/disabled

    //zoetrope variables
    uint8_t initialized;        //system initialized flag
    uint8_t strobeMode;        //strobe mode selection
    uint16_t sliceCount;     //number of slices for current zoetrope platter
    uint16_t sliceIndex;     //current slice index for zoetrope strobing
    uint8_t zoetropeEnabled;  //zoetrope mode enabled/disabled
    uint16_t sliceTime_ms;   //calculated time per slice in milliseconds
    uint32_t sliceTime_us;   //calculated time per slice in microseconds

    //system led variables
    uint8_t stateLEDStatus;
    uint16_t stateLEDPulseTime_ms;
    uint32_t prevStateLEDPulseTime_ms;

    uint8_t stateLED_firstTime;
    uint8_t stateLEDOK_R;
    uint8_t stateLEDOK_G;
    uint8_t stateLEDOK_B;

    uint8_t stateLEDIdle_R;
    uint8_t stateLEDIdle_G;
    uint8_t stateLEDIdle_B;

    uint8_t stateLEDError_R;
    uint8_t stateLEDError_G;
    uint8_t stateLEDError_B;

    uint8_t stateLEDBlank_R;
    uint8_t stateLEDBlank_G;
    uint8_t stateLEDBlank_B;

    uint32_t currentTime1;
    uint8_t neopixelBlinkState;

    uint8_t switchVariableLock; //lock certain variables locked when the system is not initialized

    //motor variables
    uint8_t motorMode;         //motor mode selection - logic not implemented yet, used for testing and different spinup and motor functions
    uint8_t motorEnabled;        //motor enabled/disabled
    uint8_t motorDirection;      //motor direction
    uint8_t motorMicrostepSetting; //motor microstep setting (full, half, quarter, eighth, sixteenth)
    uint8_t motorSpreadCycleEnabled; //motor spreadCycle enabled/disabled
    uint8_t motorUARTEnabled;    //motor UART control enabled/disabled
    uint8_t motorTMC2209Type; //indicates we are using TMC2209 stepper driver (not BLDC, which the system can support)

    //strobe variables
    uint8_t strobeEnabled;        //strobe enabled/disabled
    uint32_t strobeOnTime_ms;     //strobe ON time in milliseconds
    uint32_t strobeOnTime_us;    //strobe ON time in microseconds
    uint32_t strobeOffTime_ms;    //strobe OFF time in milliseconds
    uint32_t strobeOffTime_us;   //strobe OFF time in microseconds
    uint32_t prevStrobeChangeTime_ms; //previous time the strobe state changed - ms
    uint32_t prevStrobeChangeTime_us; //previous time the strobe state changed - us

    //v4 zoetrope PWM strobe variables (some crossover with above variables)
    uint16_t shadowStrobePWMArray[256]; //array to hold calculated PWM values for strobe control
    uint16_t StrobePWMArray[256]; //active PWM array for strobe control
    uint16_t strobeCCRValue; //calculated strobe CCR value for PWM output
    uint16_t strobeARRValue; //calculated strobe ARR value for PWM output
    uint16_t strobePSCValue; //calculated strobe PSC value for PWM output
    uint32_t strobeSrcClockValue; //timer source clock value for strobe PWM calculation
    uint16_t onTimeCCRValue; //user set strobe ON time CCR value
    uint8_t strobePWMCalculateRequired; //flag to indicate strobe PWM array needs to be recalculated - triggered at a HALL sensor read

    //esp32 screen variables (uart communication)
    uint32_t esp32_1_lastTXTime_ms; //last time data was sent to esp32 1
    uint32_t esp32_1_TXInterval_ms; //time interval between data sends to esp32 1
    uint8_t esp32_1_dataSendEnabled; //enable/disable data sending to esp32 1

    uint8_t esp32_1_verticalLineHighlight;
    uint8_t esp32_1_horizontalLineHighlight;
    uint8_t esp32_1_highlightFlag; // is the highlighter on?, toggled on by input, off by timer
    uint8_t esp32_1_selectFlag; //is an item selected (variable being changed)?

    //motor PID variables - inplementation in progress
    //stepper motor control variables
    float stepper_Kp; //proportional gain
    float stepper_Ki; //integral gain
    float stepper_Kd; //derivative gain
    float stepper_previousError; //previous error value
    float stepper_integral; //integral value
    //bldc motor control variables
    float bldc_Kp; //proportional gain
    float bldc_Ki; //integral gain
    float bldc_Kd; //derivative gain
    float bldc_previousError; //previous error value
    float bldc_integral; //integral value
    //motor selection
    uint8_t motorSelection; //0 == stepper, 1 == BLDC

    //////////////////////////////////////////////////////////////////
    /////////////////system calculated variables//////////////////////
    ///variables set based on readings, calculations, or state////////
    /////////////////////////////////////////////////////////////////

    //stystem variables
    uint8_t system_state;        //overall system state - used by the State neopixel LED

    //hall sensor 1 variables
    uint32_t hallSensor1LastTime_ms; //last time hall sensor 1 was triggered
    uint32_t hallSensor1Period_ms;   //calculated period between hall sensor 1 triggers
    uint32_t hallSensor1LastTime_us; //last time hall sensor 1 was triggered
    uint32_t hallSensor1Period_us;   //calculated period between hall sensor 1 triggers in microseconds
    
    //Motor/platter variables
    uint32_t platterRotationPeriod_us; //calculated rotation period in microseconds
    uint32_t platterRotationPeriod_ms; //calculated rotation period in milliseconds
    uint16_t platterSpeedRPM;          //calculated motor speed in RPM
    uint16_t motorSpeedoutput;       //motor speed output value for PWM control (determine range)
    uint8_t motorError;              //motor error flag

    //potentiometer variables - calculated by system
    uint32_t PWMMotorSpeedValue; //calculated PWM value for motor speed control based on potentiometer reading


    ///////////////////////////////////////
    ////////variables to be sorted later////
    ////////////////////////////////////////

    //0 = idle
    //10 = NFC searching Mode (slow spin, looking for tag)
    //11 = NFC Tag Found Mode (pause motor, read tag, set parameters)
    //15 = NFC write tag mode (**********)
    //100 = normal (non-NFC) operation mode (strobe running, motor running; all strobe and motor functions decided here)
    //101 = normal (NFC settings) operation mode (strobe running, motor running; all strobe and motor functions decided here)
    //200 = spindown mode (strobe off, motor spindown; mandatory for high speed platters)
    //1000 = error state
    uint32_t stateMachine_state;

    uint8_t NFCDetectedFlag; //flag to indicate an NFC tag has been detected
    uint8_t NFCFailFlag; //flag to indicate an NFC read/write failure
    int32_t uid_len; //length of detected NFC tag UID
    uint32_t uid_maxLength; //max length of UID buffer
    uint8_t uid[10]; //buffer to hold detected NFC tag UID
    uint32_t pn532_error; //PN532 error variable
    uint8_t nfcUserStartingblockNumber;
    uint8_t nfcDataReadV1_pageCount; //number of pages read from NFC tag for version 1 data

    uint8_t NFCReadBuffer[126][4]; //buffer to hold NFC read data - max 126 pages of 4 bytes each
    uint8_t NFCWriteBuffer[126][4]; //buffer to hold NFC write data - max 126 pages of 4 bytes each
    uint8_t DATA[4]; //for single page writes
    uint8_t DATA2[4]; //for single page reads 
    uint32_t pn532_error1;    
    uint8_t nfcSuccess;   
    uint8_t nfcSearchForTagFlag; //flag to indicate its time to search for an NFC tag
    uint8_t nfcHallSensorReadCount; //count of hall sensor reads during NFC search mode
    uint8_t nfcHallSensorReadTarget; //number of hall sensor reads to perform during NFC search mode - before giving up                                                                             
    
    //implementing new motor control loop
    uint8_t newMotorControl; //flag to indicate new motor control method being used
    uint8_t microstepSetting; //microstep setting for motor control
    
    uint32_t TIM8_CLK_HZ; //90000000 90 MHz
    uint16_t FULL_STEPS_PER_REV; //200 for standard stepper motor
    uint16_t MICROSTEPS; //8 for 1/8 microstepping
    uint32_t STEPS_PER_REV; //FULL_STEPS_PER_REV * MICROSTEPS
    uint32_t TIM_MAX_ARR; //65535 for 16 bit timer
    uint16_t TIM8_PSC; //timer prescaler value
    uint16_t TIM8_ARR; //timer auto-reload value
    uint16_t TIM8_CCR; //timer capture/compare value
    float    TIM8_StepFrequency; //calculated step frequency for motor control
    float    setRPM; //desired RPM for motor control
    float    currentRPM; //current RPM for motor control
    float    topEndRPM;
    float    bottomEndRPM;
    float    topMidPotRPM;
    float    nfcTopEndRPM;
    float    nfcBottomEndRPM;
    float    nfcTopMidPotRPM;

    uint8_t normalMotorEnableSwitch;



    //below are variables for the 5 digit 7 segment display mode/platter

    //what character is currently displayed in each digit
    uint8_t digit1; //single digit seconds
    uint8_t digit2; //tens digit seconds
    uint8_t digit3; //separator digit
    uint8_t digit4; //single digit minutes
    uint8_t digit5; //tens digit minutes

    uint8_t displayCharacterOffset; //for locating the display 

    //define which segments exist in each character type
    //each character is reprented by a byte, each bit is a segment (a-g). Digit 0b10000000, is never used and set to 0 0b0xxxxxxxx
    uint8_t character_0; //segments a,b,c,d,e,f
    uint8_t character_1; //segments b,c
    uint8_t character_2; //segments a,b,c,d,e,g
    uint8_t character_3; //segments a,b,c,d,f,g
    uint8_t character_4; //segments b,c,f,g
    uint8_t character_5; //segments a,c,d,f,g
    uint8_t character_6; //segments a,c,d,e,f,g
    uint8_t character_7; //segments a,b,c
    uint8_t character_8; //segments a,b,c,d,e,f,g
    uint8_t character_9; //segments a,b,c,d,f,g
    uint8_t character_dash; //segment g only

    uint8_t segmentLocations_A[3]; //locations of segment A on the platter
    uint8_t segmentLocations_B[3]; //locations of segment B on the platter
    uint8_t segmentLocations_C[3]; //locations of segment C on the platter
    uint8_t segmentLocations_D[3]; //locations of segment D on the platter
    uint8_t segmentLocations_E[3]; //locations of segment E on the platter
    uint8_t segmentLocations_F[3]; //locations of segment F on the platter
    uint8_t segmentLocations_G[3]; //locations of segment G on the platter

    uint16_t demoCounter; //for testing the 5 digit 7 segment display mode

} system_interface;

//PFP
//initializes hardware and reads/sets default system variables
void zoetrope_init(system_interface *sys);
//used to read and update switch, button, encoder and sensor inputs - calls other functions based on input
void zoetrope_assignHardwareI2CInputs(system_interface *sys, uint8_t interface, uint8_t data);
//assigns data read from I2C I/O expanders to system variables (pass the data in)
void zoetrope_assignI2CData(system_interface *sys, uint8_t portNum, uint8_t data);
//calls several running functions
void zoetrope_switchLightsAndState(system_interface *sys);
void zoetrope_run(system_interface *sys);

//update struct variables based on I2C input data

uint8_t zoetrope_I2CPortByte(system_interface *sys, uint8_t portNum);
void heartbeat(system_interface *sys);

long map(long x, long in_min, long in_max, long out_min, long out_max);

//void hall1_readAndMeasure(system_interface *sys); -NOW DONE IN AN INTERRUPT
void zoetrope_calculateFramePeriod(system_interface *sys);
//void zoetrope_strobeControlGPIOBasic(system_interface *sys); //testing zoetrope 'scrappy' strobe control

void zoetrope_sendESP32Data(system_interface *sys);

void strobe_PulseFinishedCallback(TIM_HandleTypeDef *htim);
void strobe_updatePWMArray(system_interface *sys);
void strobe_resetShadowArray(system_interface *sys);
void strobe_calculatePWMValues(system_interface *sys);
void strobe_applyPWMSettings(system_interface *sys);
void strobe_populateShadowArray(system_interface *sys);
void strobe_populateShadowArray_classic(system_interface *sys);
void strobe_timerPWMControl(system_interface *sys);
void zoetrope_encoderHandler(system_interface *sys);
void zoetrope_encoderButton(system_interface *sys);
void strobe_strobeModeClassic(system_interface *sys);
void zoetrope_leftButton(system_interface *sys);
void zoetrope_rightButton(system_interface *sys);
void zoetrope_hardwareRuntime(system_interface *sys);
void zoetrope_esp32DisplayRuntime(system_interface *sys);
void zoetrope_strobeModeSelection(system_interface *sys);

//testing motor control functions
void motor_pidControl(system_interface *sys);
void motor_stepperTIMControl(system_interface *sys);

//new testing below here (state machine stuff)
void zoetrope_stateMachine(system_interface *sys);
void zoetrope_motorModeSelection(system_interface *sys);
void zoetrope_NFCSearchMotorControl(system_interface *sys);
void zoetrope_scanNFCForTag(system_interface *sys, PN532 *pn532);
void zoetrope_WriteNFCTag(system_interface *sys, PN532 *pn532);
void zoetrope_ReadNFCTag(system_interface *sys, PN532 *pn532);
void zoetrope_resetNFCReadBuffer(system_interface *sys);
void zoetrope_setNFCWriteBuffer(system_interface *sys);
void zoetrope_applyNFCSettings(system_interface *sys);
void zoetrope_HALLSensorInterruptHandler(system_interface *sys);
void zoetrope_HALLSensorInterruptHandler2(system_interface *sys);

//new motor control functions
void zoetrope_stepperMotorMicroStepConfig(system_interface *sys);
void Zoetrope_Compute_TIM8_FromRPM(system_interface *sys);

//setting up 5 digit 7 segment display mode
void strobe_populateShadowArray_5Digit7Segment(system_interface *sys);
void strobe_strobeMode5Digit7SegmentDisplay(system_interface *sys);
uint8_t checkDigits(uint8_t character, uint8_t slice, system_interface *sys);
uint8_t segmentCheck(uint8_t seg, uint8_t slice, system_interface *sys);


#endif