/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

TIM_HandleTypeDef htim2;

/* USER CODE BEGIN PV */
uint32_t currentTime = 0; //generic time keeping variable (used everywhere, changes a lot)


uint32_t heartbeatPulseTime = 100; //for pulse timing
uint32_t heartbeatLastTime = 0; //for time tracking

//strobe signal variables - strobe signal is active LOW
uint8_t strobeState = 0; //current value of the strobe state
uint8_t prevStrobeState = 0; //previous reading of the strobe state
uint32_t strobeSignalLastTime = 0; //for signal timing (reading)
uint32_t strobeSignalPeriodTime = 2000; //period between signal pulses, set to large value to default to idle mode
uint32_t strobeSignalRunningCounter = 0; //running timer so we can jump to idle mode if needed

uint32_t rotationRunningModeCuttoff = 500; //how fast does the platter need to spin to go to running mode
uint8_t mode = 0; //0=idle, 1=running, 2=error

//idle mode variables
uint32_t idleModeLastTime = 0; //for time tracking the idle mode animation
uint32_t idleModeAnimationTime = 100; //animation speed control for idle mode
uint8_t idleModeAnimationIndex = 0; //animation current index for idle mode animation
uint8_t prevIdleModeAnimationIndex = 0; //for tracking when the index changes, to prevent unnecessary display updates
uint8_t idleModeAnimationMaxIndex = 7; //max animation index value before triggering rollover

//display variables
uint16_t display[150][7]; //values for each LED in the array X x Y, indexed bottom left corner
uint16_t frameCount = 150; //one frame for each column in the display
uint16_t blade2FrameOffset = 75; //used for second strip of LEDs
uint16_t frameIndex = 0; //what is the frame currently displayed
uint16_t prevFrameIndex = 0; //for tracking when frame index changes - prevent unnecessary updates
uint8_t frameTurnedOff = 0; //ensure the lights are not on too long - controls motion blur (flag to indicate if the lights have been turned off)
uint32_t frameOnTime = 1; //how long should the LEDs be on for - us
uint32_t frameTime = 0; //indexing time for each frame - rotation time / frames, converted from ms to us
uint32_t frameIndexTime = 0; //for timing reasons - control when to turn the LEDs OFF
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */
void heartbeat(void); //basic heartbeat LED - also indicates if error
void readStrobe(void); //reads the strobe pin, sees if it has changed, records speed timing and sets mode
void readIR(void); //this sets the RED led to match whatever the output of the IR signal is

void resetBlades(void); //sets all the LEDs on the LED blades to OFF

void displayRuntime(uint8_t modeSelection); //display LEDs output and control (stroboscopic display and idle mode)
void idleMode(void); //LED animation for when the platter is not spinning fast enough for stroboscopic display
void idleModeAnimationControl(uint8_t index); //control of the LEDs in idle mode

void displayControl(void); //controls LEDs to create the display
void displayLEDControl(uint8_t frame); // sets appropriate LEDS ON for the current frame

void clearDisplayArray(void);

void displaySet_HelloWorld(void);

void scrappyTesting(void); //to test things without making a mess
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  //start the animation control timer
  HAL_TIM_Base_Start(&htim2);

  //set all blade LEDs to OFF
  resetBlades();

  //set display to "Hello World"
  displaySet_HelloWorld();
  
  /*
  //turning all the pixels ON
  for(uint8_t k=0; k<frameCount; k++){
    for(uint8_t e=0; e<7; e++){
      display[k][e] = 1;
    }
  }
  */


  while (1)
  {
    heartbeat(); //basic heartbeat LED - also indicates if error
    readStrobe(); //reads the strobe pin, sees if it has changed, records speed timing and sets mode
    readIR(); //this sets the RED led to match whatever the output of the IR signal is
    displayRuntime(mode); //display LEDs output and control (stroboscopic display and idle mode)


    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_FLASH_SET_LATENCY(FLASH_LATENCY_1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 48-1;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 4294967295;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, neopixel_Pin|led1_Pin|led2_Pin|led3_Pin
                          |led4_Pin|ledRED_Pin|ledBLUE_Pin|ledGREEN_Pin
                          |led8_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, led5_Pin|led6_Pin|led7_Pin|led9_Pin
                          |led10_Pin|led11_Pin|led12_Pin|led13_Pin
                          |led14_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : IR_input_Pin */
  GPIO_InitStruct.Pin = IR_input_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(IR_input_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : neopixel_Pin led1_Pin led2_Pin led3_Pin
                           led4_Pin ledRED_Pin ledBLUE_Pin ledGREEN_Pin
                           led8_Pin */
  GPIO_InitStruct.Pin = neopixel_Pin|led1_Pin|led2_Pin|led3_Pin
                          |led4_Pin|ledRED_Pin|ledBLUE_Pin|ledGREEN_Pin
                          |led8_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : led5_Pin led6_Pin led7_Pin led9_Pin
                           led10_Pin led11_Pin led12_Pin led13_Pin
                           led14_Pin */
  GPIO_InitStruct.Pin = led5_Pin|led6_Pin|led7_Pin|led9_Pin
                          |led10_Pin|led11_Pin|led12_Pin|led13_Pin
                          |led14_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : strobe_Pin */
  GPIO_InitStruct.Pin = strobe_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(strobe_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
//basic heartbeat function to indicate the system is alive
void heartbeat(void){
  currentTime = HAL_GetTick();
  if(currentTime > heartbeatLastTime + heartbeatPulseTime){
    heartbeatLastTime = currentTime;
    if(mode == 2){
      //error mode - blink the error LED
      HAL_GPIO_TogglePin(ledRED_GPIO_Port, ledRED_Pin);
      HAL_GPIO_TogglePin(ledGREEN_GPIO_Port, ledGREEN_Pin);
      HAL_GPIO_TogglePin(ledBLUE_GPIO_Port, ledBLUE_Pin);
    }else{
      if(mode == 0){
        //idle mode
        //normal heartbeat LED
        HAL_GPIO_TogglePin(ledRED_GPIO_Port, ledRED_Pin);
      }else{
        if(mode == 1){
          //running mode
          HAL_GPIO_TogglePin(ledGREEN_GPIO_Port, ledGREEN_Pin); //blink green LED to ensure we are in the right mode
        }
      }
    }
  }
}

//reads the strobe signal and tracks timing between signal pulses
void readStrobe(void){

  //temp test with the LED
  HAL_GPIO_WritePin(ledBLUE_GPIO_Port, ledBLUE_Pin, !HAL_GPIO_ReadPin(strobe_GPIO_Port, strobe_Pin));

  currentTime = HAL_GetTick(); //note the time

  strobeState = HAL_GPIO_ReadPin(strobe_GPIO_Port, strobe_Pin);
  if((strobeState != prevStrobeState) && (strobeState == 0)){
    //new pulse, strobe state is different than last time and it is also LOW
    TIM2->CNT = 0; //reset the timer used for animation (us timer)
    strobeSignalPeriodTime = currentTime - strobeSignalLastTime;
    strobeSignalLastTime = currentTime;
    strobeSignalRunningCounter = 0;
    frameTime = (strobeSignalPeriodTime * 1000) / frameCount; //frame time converted into us
    frameIndex = 0; //starting at frame 0, first column of the display

    if(strobeSignalPeriodTime < rotationRunningModeCuttoff){
      //rotating fast enough to turn on display
      mode = 1; //running mode
    }else{
      //not spinning fast enough - go to idle mode
      mode = 0; //idle mode
    }
  }else{
    //increment the frame index as necessary based on speed and frame time calculations
    if(mode == 1){
      //keep track of what index we are on
      currentTime = TIM2->CNT;
      if(currentTime > frameTime){
        frameIndex = currentTime / frameTime;
        frameIndexTime = TIM2->CNT; //note when we turn the LEDs ON, so we turn them OFF at the right time
      }else{
        frameIndex = 0;
      }
    }
  }
  prevStrobeState = strobeState; //update the prev state flag now that we have used its value

  //set a catch here for a running counter of the strobe signal so we can jump to idle if platter slows
  currentTime = HAL_GetTick();
  strobeSignalRunningCounter = currentTime - strobeSignalLastTime; //timer tracking how long since last pulse
  if(strobeSignalRunningCounter > rotationRunningModeCuttoff){
    //too long since seeing a strobe pulse - turn off the display and return to IDLE
    mode = 0; //idle mode
  }

}

void readIR(void){
  //really fast testing here to see how this works
  //mirror IR signl to the RED LED
  HAL_GPIO_WritePin(ledBLUE_GPIO_Port, ledBLUE_Pin, !HAL_GPIO_ReadPin(IR_input_GPIO_Port, IR_input_Pin));
}

//display LEDs output and control (stroboscopic display and idle mode)
void displayRuntime(uint8_t modeSelection){

  //we pass in the global variable because its easier to tell what is happening from main loop
  switch(modeSelection){
    case 0: //idle mode
      idleMode();
    break;
    case 1: //spinning, stroboscopic display mode
      //so something
      //resetBlades(); //turn off LEDs - for now
      displayControl(); //control LEDs for the display
    break;
    case 2: //error mode
      //do something
      resetBlades(); //turn off LEDs - for now
    break;
  }
}

void displayControl(void){
  //this will control the LEDs for the display
  if(frameIndex != prevFrameIndex){
    prevFrameIndex = frameIndex;
    //top of new frame
    frameTurnedOff = 0; //about to turn the LEDs ON, they have not yet been turned OFF

    //turn on the correct LEDs for this frame
    displayLEDControl(frameIndex);

  }else{
    //still the same frame as last time
    if(!frameTurnedOff){
      //the LEDs are still ON, check for when to turn them OFF
      currentTime = TIM2->CNT;
      if(currentTime > frameIndexTime + frameOnTime){
        //time to turn the LEDs OFF
        frameTurnedOff = 1; //set LEDs OFF flag
        resetBlades(); //turn OFF the display LEDs
      }
    }
  }
}

void displayLEDControl(uint8_t frame){

  uint16_t blade2Frame = 0;

  if(frame >= blade2FrameOffset){ //this is setting the second strip of LEDs - need to handle rollover
    //handle rollover
    blade2Frame = (frame + blade2FrameOffset) - frameCount;
  }else{
    //simple offset
    blade2Frame = frame + blade2FrameOffset;
  }

  //we set the LEDs that need to be on for the current frame - there is one frame for each column of the display
  
  if(frame < frameCount){
    
    //blade 1
    HAL_GPIO_WritePin(led1_GPIO_Port, led1_Pin, display[frame][0]); //led 1, bottom row
    HAL_GPIO_WritePin(led2_GPIO_Port, led2_Pin, display[frame][1]); //led 2
    HAL_GPIO_WritePin(led3_GPIO_Port, led3_Pin, display[frame][2]); //led 3
    HAL_GPIO_WritePin(led4_GPIO_Port, led4_Pin, display[frame][3]); //led 4
    HAL_GPIO_WritePin(led5_GPIO_Port, led5_Pin, display[frame][4]); //led 5
    HAL_GPIO_WritePin(led6_GPIO_Port, led6_Pin, display[frame][5]); //led 6
    HAL_GPIO_WritePin(led7_GPIO_Port, led7_Pin, display[frame][6]); //led 7

  }
  
  if(blade2Frame < frameCount){

    //blade 2 - 180 offset from blade 1, offset on display by 1/2 of display - 75 pixels. Need to also handle wrap around
    HAL_GPIO_WritePin(led14_GPIO_Port, led14_Pin, display[blade2Frame][0]); //led 14, bottom row, 180 out from blade 1, 75 pixels offset from blade 1
    HAL_GPIO_WritePin( led13_GPIO_Port, led13_Pin, display[blade2Frame][1]); //led 13
    HAL_GPIO_WritePin( led12_GPIO_Port, led12_Pin, display[blade2Frame][2]); //led 12
    HAL_GPIO_WritePin( led11_GPIO_Port, led11_Pin, display[blade2Frame][3]); //led 11
    HAL_GPIO_WritePin( led10_GPIO_Port, led10_Pin, display[blade2Frame][4]); //led 10
    HAL_GPIO_WritePin( led9_GPIO_Port, led9_Pin, display[blade2Frame][5]); //led 9
    HAL_GPIO_WritePin( led8_GPIO_Port, led8_Pin, display[blade2Frame][6]); //led 8

  }
    
}

void idleMode(void){

  currentTime = HAL_GetTick();
  if(currentTime > idleModeLastTime + idleModeAnimationTime){
    idleModeLastTime = currentTime;
    
    if(idleModeAnimationIndex > 0){
      idleModeAnimationIndex--;
    }else{
      idleModeAnimationIndex = idleModeAnimationMaxIndex - 1;
    }
    
    if(idleModeAnimationIndex != prevIdleModeAnimationIndex){ //this check is redundant
      idleModeAnimationControl(idleModeAnimationIndex);
    }
    prevIdleModeAnimationIndex = idleModeAnimationIndex; 
    
  }
}

void idleModeAnimationControl(uint8_t index){

  if(index > idleModeAnimationMaxIndex){
    //invalid entry
  }

  //reset the LEDs before setting the LEDs
  resetBlades();


  switch(index){
    case 0:
      //do something
      HAL_GPIO_WritePin(led1_GPIO_Port, led1_Pin, 1); //blade 1
      HAL_GPIO_WritePin(led14_GPIO_Port, led14_Pin, 1); //blade 2
    break;

    case 1:
      //do something
      HAL_GPIO_WritePin(led2_GPIO_Port, led2_Pin, 1); //blade 1
      HAL_GPIO_WritePin(led13_GPIO_Port, led13_Pin, 1); //blade 2
    break;

    case 2:
      //do something
      HAL_GPIO_WritePin(led3_GPIO_Port, led3_Pin, 1); //blade 1
      HAL_GPIO_WritePin(led12_GPIO_Port, led12_Pin, 1); //blade 2
    break;

    case 3:
      //do something
      HAL_GPIO_WritePin(led4_GPIO_Port, led4_Pin, 1); //blade 1
      HAL_GPIO_WritePin(led11_GPIO_Port, led11_Pin, 1); //blade 2
    break;

    case 4:
      //do something
      HAL_GPIO_WritePin(led5_GPIO_Port, led5_Pin, 1); //blade 1
      HAL_GPIO_WritePin(led10_GPIO_Port, led10_Pin, 1); //blade 2
    break;

    case 5:
      //do something
      HAL_GPIO_WritePin(led6_GPIO_Port, led6_Pin, 1); //blade 1
      HAL_GPIO_WritePin(led9_GPIO_Port, led9_Pin, 1); //blade 2
    break;

    case 6:
      //do something
      HAL_GPIO_WritePin(led7_GPIO_Port, led7_Pin, 1); //blade 1
      HAL_GPIO_WritePin(led8_GPIO_Port, led8_Pin, 1); //blade 2
    break;

    case 7:
      //do something
    break;
  }

}

void resetBlades(void){
  //blade 1
  HAL_GPIO_WritePin(led1_GPIO_Port, led1_Pin, 0);
  HAL_GPIO_WritePin(led2_GPIO_Port, led2_Pin, 0);
  HAL_GPIO_WritePin(led3_GPIO_Port, led3_Pin, 0);
  HAL_GPIO_WritePin(led4_GPIO_Port, led4_Pin, 0);
  HAL_GPIO_WritePin(led5_GPIO_Port, led5_Pin, 0);
  HAL_GPIO_WritePin(led6_GPIO_Port, led6_Pin, 0);
  HAL_GPIO_WritePin(led7_GPIO_Port, led7_Pin, 0);

  //blade 2
  HAL_GPIO_WritePin(led8_GPIO_Port, led8_Pin, 0);
  HAL_GPIO_WritePin(led9_GPIO_Port, led9_Pin, 0);
  HAL_GPIO_WritePin(led10_GPIO_Port, led10_Pin, 0);
  HAL_GPIO_WritePin(led11_GPIO_Port, led11_Pin, 0);
  HAL_GPIO_WritePin(led12_GPIO_Port, led12_Pin, 0);
  HAL_GPIO_WritePin(led13_GPIO_Port, led13_Pin, 0);
  HAL_GPIO_WritePin(led14_GPIO_Port, led14_Pin, 0);
}

void displaySet_HelloWorld(void){

  uint8_t startIndex = 10; //where does Hello World start on the display

  //set the display array to say "Hello World"
  //first clear the display
  clearDisplayArray();

  //resetBlades();

  //H
  for(uint8_t k=0; k<7; k++){
    display[startIndex][k] = 1; //turn on one pixel to test the system
  }
  startIndex++;
  display[startIndex][3] = 1;
  startIndex++;
  display[startIndex][3] = 1;
  startIndex++;
  display[startIndex][3] = 1;
  startIndex++;
  for(uint8_t k=0; k<7; k++){
    display[startIndex][k] = 1; //turn on one pixel to test the system
  }
  startIndex+=2; //add extra space after character

  //e
  for(uint8_t k=1; k<4; k++){
    display[startIndex][k] = 1; //turn on one pixel to test the system
  }
  startIndex++;
  display[startIndex][0] = 1;
  display[startIndex][1] = 0;
  display[startIndex][2] = 1;
  display[startIndex][3] = 0;
  display[startIndex][4] = 1;
  display[startIndex][5] = 0;
  display[startIndex][6] = 0;
  display[startIndex][7] = 0;
  startIndex++;
  display[startIndex][0] = 1;
  display[startIndex][2] = 1;
  display[startIndex][4] = 1;
  startIndex++;
  display[startIndex][0] = 1;
  display[startIndex][2] = 1;
  display[startIndex][3] = 1;
  startIndex+=2; //add extra space after character

  //l
  display[startIndex][0] = 1;
  display[startIndex][6] = 1;
  startIndex++;
  for(uint8_t k=0; k<7; k++){
    display[startIndex][k] = 1; //turn on one pixel to test the system
  }
  startIndex++;
  display[startIndex][0] = 1;
  startIndex+=2; //add extra space between characters

  //l
  display[startIndex][0] = 1;
  display[startIndex][6] = 1;
  startIndex++;
  for(uint8_t k=0; k<7; k++){
    display[startIndex][k] = 1; //turn on one pixel to test the system
  }
  startIndex++;
  display[startIndex][0] = 1;
  startIndex+=2; //add extra space between characters

  //o
  for(uint8_t k=1; k<4; k++){
    display[startIndex][k] = 1; //turn on one pixel to test the system
  }
  startIndex++;
  display[startIndex][0] = 1;
  display[startIndex][4] = 1;
  startIndex++;
  display[startIndex][0] = 1;
  display[startIndex][4] = 1;
  startIndex++;
  for(uint8_t k=1; k<4; k++){
    display[startIndex][k] = 1; //turn on one pixel to test the system
  }
  startIndex+=2; //add extra space between characters

  startIndex+=4; //add extra space between words

  //W
  for(uint8_t k=1; k<7; k++){
    display[startIndex][k] = 1; //turn on one pixel to test the system
  }
  startIndex++;
  display[startIndex][0] = 1;
  startIndex++;
  display[startIndex][0] = 1;
  startIndex++;
  for(uint8_t k=1; k<5; k++){
    display[startIndex][k] = 1; //turn on one pixel to test the system
  }
  startIndex++;
  display[startIndex][0] = 1;
  startIndex++;
  display[startIndex][0] = 1;
  startIndex++;
  for(uint8_t k=1; k<7; k++){
    display[startIndex][k] = 1; //turn on one pixel to test the system
  }
  startIndex+=2; //add extra space between characters

  //o
  for(uint8_t k=1; k<4; k++){
    display[startIndex][k] = 1; //turn on one pixel to test the system
  }
  startIndex++;
  display[startIndex][0] = 1;
  display[startIndex][4] = 1;
  startIndex++;
  display[startIndex][0] = 1;
  display[startIndex][4] = 1;
  startIndex++;
  for(uint8_t k=1; k<4; k++){
    display[startIndex][k] = 1; //turn on one pixel to test the system
  }
  startIndex+=2; //add extra space between characters

  //r
  for(uint8_t k=0; k<5; k++){
    display[startIndex][k] = 1; //turn on one pixel to test the system
  }
  startIndex++;
  display[startIndex][4] = 1;
  startIndex++;
  display[startIndex][4] = 1;
  startIndex++;
  display[startIndex][4] = 1;
  startIndex++;
  display[startIndex][3] = 1;
  startIndex+=2; //add extra space between characters

  //l
  display[startIndex][0] = 1;
  display[startIndex][6] = 1;
  startIndex++;
  for(uint8_t k=0; k<7; k++){
    display[startIndex][k] = 1; //turn on one pixel to test the system
  }
  startIndex++;
  display[startIndex][0] = 1;
  startIndex+=2; //add extra space between characters

  //d
  for(uint8_t k=1; k<4; k++){
    display[startIndex][k] = 1; //turn on one pixel to test the system
  }
  startIndex++;
  display[startIndex][0] = 1;
  display[startIndex][4] = 1;
  startIndex++;
  display[startIndex][0] = 1;
  display[startIndex][4] = 1;
  startIndex++;
  for(uint8_t k=0; k<7; k++){
    display[startIndex][k] = 1; //turn on one pixel to test the system
  }


}

void clearDisplayArray(void){
  //clearing the entire display array
  for(uint8_t k=0; k<frameCount; k++){
    for(uint8_t e=0; e<7; e++){
      display[k][e] = 0;
    }
  }
}

//for fast testing stuff
void scrappyTesting(void){

  //HAL_GPIO_TogglePin(ledGREEN_GPIO_Port, ledGREEN_Pin);
  //HAL_GPIO_TogglePin(ledBLUE_GPIO_Port, ledBLUE_Pin);
  HAL_GPIO_TogglePin(ledRED_GPIO_Port, ledRED_Pin);
  HAL_Delay(500);

  //led 1
  HAL_GPIO_TogglePin(led1_GPIO_Port, led1_Pin);
  HAL_Delay(250);
  HAL_GPIO_TogglePin(led1_GPIO_Port, led1_Pin);
  HAL_Delay(250);
  //led 2
  HAL_GPIO_TogglePin(led2_GPIO_Port, led2_Pin);
  HAL_Delay(250);
  HAL_GPIO_TogglePin(led2_GPIO_Port, led2_Pin);
  HAL_Delay(250);
  //led 3
  HAL_GPIO_TogglePin(led3_GPIO_Port, led3_Pin);
  HAL_Delay(250);
  HAL_GPIO_TogglePin(led3_GPIO_Port, led3_Pin);
  HAL_Delay(250);
  //led 4
  HAL_GPIO_TogglePin(led4_GPIO_Port, led4_Pin);
  HAL_Delay(250);
  HAL_GPIO_TogglePin(led4_GPIO_Port, led4_Pin);
  HAL_Delay(250);
  //led 5
  HAL_GPIO_TogglePin(led5_GPIO_Port, led5_Pin);
  HAL_Delay(250);
  HAL_GPIO_TogglePin(led5_GPIO_Port, led5_Pin);
  HAL_Delay(250);
  //led 6
  HAL_GPIO_TogglePin(led6_GPIO_Port, led6_Pin);
  HAL_Delay(250);
  HAL_GPIO_TogglePin(led6_GPIO_Port, led6_Pin);
  HAL_Delay(250);
  //led 7
  HAL_GPIO_TogglePin(led7_GPIO_Port, led7_Pin);
  HAL_Delay(250);
  HAL_GPIO_TogglePin(led7_GPIO_Port, led7_Pin);
  HAL_Delay(250);

  //led 8 (blade 2 pixel 7)
  HAL_GPIO_TogglePin(led8_GPIO_Port, led8_Pin);
  HAL_Delay(250);
  HAL_GPIO_TogglePin(led8_GPIO_Port, led8_Pin);
  HAL_Delay(250);
  //led 9 (blade 2 pixel 6)
  HAL_GPIO_TogglePin(led9_GPIO_Port, led9_Pin);
  HAL_Delay(250);
  HAL_GPIO_TogglePin(led9_GPIO_Port, led9_Pin);
  HAL_Delay(250);
  //led 10 (blade 2 pixel 5)
  HAL_GPIO_TogglePin(led10_GPIO_Port, led10_Pin);
  HAL_Delay(250);
  HAL_GPIO_TogglePin(led10_GPIO_Port, led10_Pin);
  HAL_Delay(250);
  //led 11 (blade 2 pixel 4)
  HAL_GPIO_TogglePin(led11_GPIO_Port, led11_Pin);
  HAL_Delay(250);
  HAL_GPIO_TogglePin(led11_GPIO_Port, led11_Pin);
  HAL_Delay(250);
  //led 12 (blade 2 pixel 3)
  HAL_GPIO_TogglePin(led12_GPIO_Port, led12_Pin);
  HAL_Delay(250);
  HAL_GPIO_TogglePin(led12_GPIO_Port, led12_Pin);
  HAL_Delay(250);
  //led 13 (blade 2 pixel 2)
  HAL_GPIO_TogglePin(led13_GPIO_Port, led13_Pin);
  HAL_Delay(250);
  HAL_GPIO_TogglePin(led13_GPIO_Port, led13_Pin);
  HAL_Delay(250);
  //led 14 (blade 2 pixel 1)
  HAL_GPIO_TogglePin(led14_GPIO_Port, led14_Pin);
  HAL_Delay(250);
  HAL_GPIO_TogglePin(led14_GPIO_Port, led14_Pin);
  HAL_Delay(250);
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
