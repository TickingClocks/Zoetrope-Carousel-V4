/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "drivers/TCA9534.h"
#include "drivers/WS2812.h"
#include "drivers/TMC2209.h"
#include "app/machine interface.h"
#include "stm32f446xx.h"
#include "stm32f4xx_hal_gpio.h"
#include "drivers/ESP_UART.h"
#include "drivers/VEML7700.h"

#include "drivers/pn532.h"
#include "drivers/pn532_HAL.h"
#include <stdint.h>


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
ADC_HandleTypeDef hadc1;

I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim4;
TIM_HandleTypeDef htim5;
TIM_HandleTypeDef htim8;
DMA_HandleTypeDef hdma_tim2_ch2_ch4;
DMA_HandleTypeDef hdma_tim4_ch1;

UART_HandleTypeDef huart4;
UART_HandleTypeDef huart5;
DMA_HandleTypeDef hdma_uart4_tx;
DMA_HandleTypeDef hdma_uart5_tx;

/* USER CODE BEGIN PV */




HAL_StatusTypeDef HAL_Status_I2C_2 = HAL_OK;

//system instance
system_interface zoetrope; //main system instance

//esp32 uart tx data instance
ESPTX_instance esp32_1; //main rectangle ESP32-S3 screen
ESPTX_instance esp32_2; //circle ESP32-S3 screen

//PCA9534 I2C I/O expansion variables and instances
//three instances
TCA9534_instance TCA_port1;
TCA9534_instance TCA_port2;
TCA9534_instance TCA_port3;

//settings for port1 (U3)
uint8_t port1Address = 0b0100000;
uint8_t port1Config = 0b01100000; //configuration - inputs for esp32 pins, outputs for motor control pins and heartbeat LED
uint8_t port1Polarity = 0b00000000; //TMC2209 enable pin is active low
uint8_t port1DefVal = 0b00000000; //defualt to OFF
//settings for port2 (U7)
uint8_t port2Address = 0b0100001;
uint8_t port2Config = 0b01111111; //configuration - inputs for switches and unused pins, outputs for heartbeat LED TEST
uint8_t port2Polarity = 0b00000111; //polarity inversion for switch inputs
uint8_t port2DefVal = 0b00000000; //default to OFF
//settings for port3 (U10)
uint8_t port3Address = 0b0100010;   
uint8_t port3Config = 0b00011111; //configuration - inputs for buttons, outputs for fans and heartbeat LED TEST
uint8_t port3Polarity = 0b00000000; //no polarity inversion
uint8_t port3DefVal = 0b00000000; //default to OFF


uint32_t heartbeatTime = 0;
uint32_t prevHeartbeatTime = 0;
uint16_t blinkTime = 250; //heartbeat blink tie
uint8_t heartbeatIndex = 0; 

volatile uint16_t tempVariable = 0;




//figuring out NFC below here////
uint8_t NFCReadBuffer[126][4]; //buffer to hold NFC read data - max 126 pages of 4 bytes each
uint8_t NFCWriteBuffer[126][4]; //buffer to hold NFC write data - max 126 pages of 4 bytes each
uint8_t DATA[] = {0x05, 0x06, 0x07, 0x08}; //for single page writes
uint8_t DATA2[] = {0x00, 0x00, 0x00, 0x00}; //for single page reads
uint8_t NFCUID[7]; //buffer to hold NFC UID - max 7 bytes
uint8_t NFCUIDLength = 0; //length of the NFC UID read
uint8_t NFCDetectedFlag = 0; //flag to indicate if an NFC tag has been detected
uint8_t NFCFailFlag = 0; //flag to indicate if an NFC operation has failed
uint8_t zoetrope_NFCPageCount = 10; //number of pages to read/write for zoetrope data, each page is 4 bytes - adjust as needed, starting at 10, 2 extra pages - for testing

//variable in the example code
uint8_t buff[255]; //look to optimize this out? - or integrate the DATA buffers into this
uint8_t uid[MIFARE_UID_MAX_LENGTH]; 
uint32_t pn532_error = PN532_ERROR_NONE; //reset error variable
int32_t uid_len = 0; //reset uid length variable
uint8_t block_number = 6; //this is where data can start being written on NTAG2xx tags

uint8_t buff[255];
uint8_t uid[MIFARE_UID_MAX_LENGTH];
//int32_t uid_len = 0; defined above
PN532 pn532;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_I2C1_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM1_Init(void);
static void MX_UART4_Init(void);
static void MX_TIM8_Init(void);
static void MX_TIM2_Init(void);
static void MX_UART5_Init(void);
static void MX_TIM4_Init(void);
static void MX_TIM5_Init(void);
/* USER CODE BEGIN PFP */

void I2C_IO_run(void);
void NFC_writeDataToSticker(PN532* pn532);
void setNFCWriteBuffer(void);
void resetNFCReadBuffer(void);

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
  MX_DMA_Init();
  MX_I2C1_Init();
  MX_ADC1_Init();
  MX_TIM1_Init();
  MX_UART4_Init();
  MX_TIM8_Init();
  MX_TIM2_Init();
  MX_UART5_Init();
  MX_TIM4_Init();
  MX_TIM5_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  //start microsecond (platter speed tracker) timer
  HAL_TIM_Base_Start(&htim5);
  
  //start the encoder timer
  HAL_TIM_Encoder_Start(&htim1, TIM_CHANNEL_ALL);
  //preload the encoder timer count - timer increments the CNT register based on rotation direction of the encoder - need to set roughly in the middle of the range
  TIM1->CNT = 32768; //somewhere in the middle of the 16-bit range

  zoetrope.encoderTimerCount = TIM1->CNT; //set the timer encoder value
  zoetrope.prevEncoderTimerCount = zoetrope.encoderTimerCount; //set the previous timer encoder value
  

  //initialize each I/O expander with default zoetrope carousel v4 settings
  HAL_Status_I2C_2 = TCA9534_init(&TCA_port1, &hi2c1, port1Address, port1Config, port1Polarity, port1DefVal);
  HAL_Status_I2C_2 = TCA9534_init(&TCA_port2, &hi2c1, port2Address, port2Config, port2Polarity, port2DefVal);
  HAL_Status_I2C_2 = TCA9534_init(&TCA_port3, &hi2c1, port3Address, port3Config, port3Polarity, port3DefVal);

  zoetrope_init(&zoetrope); //call this after all the other initializations except TMC2209_init()
  TMC2209_init(&zoetrope, 0, 0); //initialize the TMC2209 motor driver - full step, stealthChop

  HAL_Status_I2C_2 = HAL_ADC_Start_IT(&hadc1); //start ADC in interrupt mode - continuous conversion

  //stepper motor pwm
  HAL_TIMEx_PWMN_Start(&htim8, TIM_CHANNEL_1); //if this doesnt work, try CH1N

  //set all LEDs to OFF - to start
  HAL_GPIO_WritePin(GPIOC, testLED1_Pin, 0);
  HAL_GPIO_WritePin(GPIOC, testLED2_Pin, 0);
  HAL_GPIO_WritePin(GPIOC, testLED3_Pin, 0);
  neopixel_setAllLEDColor(0, 0, 0);
  neopixel_update();

  //keep the strobe off until it is implemented
  strobe_timerPWMControl(&zoetrope); //start the strobe PWM output //TURN THE STROBE OFF 


  //////////////////////////////////////////////////
  ////////NFC Initialize////////////////////////
  /////////////////////////////////////////////////
  
  PN532_I2C_Init(&pn532);
  PN532_GetFirmwareVersion(&pn532, buff);
  if (PN532_GetFirmwareVersion(&pn532, buff) == PN532_STATUS_OK) {
    buff[0] = buff[0]; //breakpoint spot
  } else {
    //return -1; //this will break things - kadin
  }
  PN532_SamConfiguration(&pn532); //put into read mode? - not sure what this does yet, but its needed

  //put zoetrope into default running state - for testing
  //zoetrope.stateMachine_state = 100; //default to running state
  zoetrope.stateMachine_state = 10; //NFC search mode
  //set temporary motor mode
  uint8_t tempMotorMode = 100; //default mode
  zoetrope.motorMode = tempMotorMode;

  while (1)
  {    

    //cant move this until I move the function itself from this file
    //checks if time to update I2C I/O expander inputs then reads the inputs if needed
    I2C_IO_run(); //call this function FIRST IN THE LOOP prior to chaning any I/O variables (it will overwrite changes otherwise)
    
    //everything but checking i2c inputs
    zoetrope_stateMachine(&zoetrope); //system state machine handler - all system functions and variables set here 

    //NFC TESTING - write and confirm good write (this slows things down ALOT)
    //NFC_writeDataToSticker(&pn532);

    
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 180;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Activate the Over-Drive mode
  */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = DISABLE;
  hadc1.Init.ContinuousConvMode = ENABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 400000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_Encoder_InitTypeDef sConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 0;
  htim1.Init.CounterMode = TIM_COUNTERMODE_DOWN;
  htim1.Init.Period = 65535;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  sConfig.EncoderMode = TIM_ENCODERMODE_TI1;
  sConfig.IC1Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC1Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC1Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC1Filter = 15;
  sConfig.IC2Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC2Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC2Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC2Filter = 15;
  if (HAL_TIM_Encoder_Init(&htim1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */

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

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 113-1;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);

}

/**
  * @brief TIM4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM4_Init(void)
{

  /* USER CODE BEGIN TIM4_Init 0 */

  /* USER CODE END TIM4_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM4_Init 1 */

  /* USER CODE END TIM4_Init 1 */
  htim4.Instance = TIM4;
  htim4.Init.Prescaler = 0;
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim4.Init.Period = 1000;
  htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim4, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM4_Init 2 */

  /* USER CODE END TIM4_Init 2 */
  HAL_TIM_MspPostInit(&htim4);

}

/**
  * @brief TIM5 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM5_Init(void)
{

  /* USER CODE BEGIN TIM5_Init 0 */

  /* USER CODE END TIM5_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM5_Init 1 */

  /* USER CODE END TIM5_Init 1 */
  htim5.Instance = TIM5;
  htim5.Init.Prescaler = 45-1;
  htim5.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim5.Init.Period = 4294967295;
  htim5.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim5.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim5) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim5, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim5, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM5_Init 2 */

  /* USER CODE END TIM5_Init 2 */

}

/**
  * @brief TIM8 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM8_Init(void)
{

  /* USER CODE BEGIN TIM8_Init 0 */

  /* USER CODE END TIM8_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM8_Init 1 */

  /* USER CODE END TIM8_Init 1 */
  htim8.Instance = TIM8;
  htim8.Init.Prescaler = 1000-1;
  htim8.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim8.Init.Period = 100-1;
  htim8.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim8.Init.RepetitionCounter = 0;
  htim8.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim8) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim8, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim8, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim8, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM8_Init 2 */

  /* USER CODE END TIM8_Init 2 */
  HAL_TIM_MspPostInit(&htim8);

}

/**
  * @brief UART4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_UART4_Init(void)
{

  /* USER CODE BEGIN UART4_Init 0 */

  /* USER CODE END UART4_Init 0 */

  /* USER CODE BEGIN UART4_Init 1 */

  /* USER CODE END UART4_Init 1 */
  huart4.Instance = UART4;
  huart4.Init.BaudRate = 115200;
  huart4.Init.WordLength = UART_WORDLENGTH_8B;
  huart4.Init.StopBits = UART_STOPBITS_1;
  huart4.Init.Parity = UART_PARITY_NONE;
  huart4.Init.Mode = UART_MODE_TX_RX;
  huart4.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart4.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN UART4_Init 2 */

  /* USER CODE END UART4_Init 2 */

}

/**
  * @brief UART5 Initialization Function
  * @param None
  * @retval None
  */
static void MX_UART5_Init(void)
{

  /* USER CODE BEGIN UART5_Init 0 */

  /* USER CODE END UART5_Init 0 */

  /* USER CODE BEGIN UART5_Init 1 */

  /* USER CODE END UART5_Init 1 */
  huart5.Instance = UART5;
  huart5.Init.BaudRate = 115200;
  huart5.Init.WordLength = UART_WORDLENGTH_8B;
  huart5.Init.StopBits = UART_STOPBITS_1;
  huart5.Init.Parity = UART_PARITY_NONE;
  huart5.Init.Mode = UART_MODE_TX_RX;
  huart5.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart5.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart5) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN UART5_Init 2 */

  /* USER CODE END UART5_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Stream0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream0_IRQn);
  /* DMA1_Stream4_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream4_IRQn);
  /* DMA1_Stream6_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream6_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream6_IRQn);
  /* DMA1_Stream7_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream7_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream7_IRQn);

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
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, testLED3_Pin|testLED2_Pin|testLED1_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : U10_int_Pin U7_int_Pin U3_int_Pin */
  GPIO_InitStruct.Pin = U10_int_Pin|U7_int_Pin|U3_int_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PC0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : testLED3_Pin testLED2_Pin testLED1_Pin */
  GPIO_InitStruct.Pin = testLED3_Pin|testLED2_Pin|testLED1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

void I2C_IO_run(void){

  //read I2C inputs from all three I/O expanders and update system variables
  //TCA9534_ReadInputs(&hi2c1, TCA_port1.address, &TCA_port1.data);
  //TCA9534_ReadInputs(&hi2c1, TCA_port2.address, &TCA_port2.data);
  //TCA9534_ReadInputs(&hi2c1, TCA_port3.address, &TCA_port3.data);

  //this pin didnt fire an interrupt for some reason, so just do a GPIO read to check its state - active low
  if(!HAL_GPIO_ReadPin(GPIOC, U3_int_Pin)){ //active low interrupt pin
    zoetrope.U3_interruptFlag = 1; //set flag to indicate I2C read needed
  }

  //check if we need to from the I2C I/O expanders - check flags set in interrupt
  if(zoetrope.U3_interruptFlag == 1){
    TCA9534_ReadInputs(&hi2c1, TCA_port1.address, &TCA_port1.data);
    zoetrope.U3_interruptFlag = 0; //clear flag
    zoetrope_assignHardwareI2CInputs(&zoetrope, 1, TCA_port1.data); //read U3 I2C I/O expander inputs and update system variables
  }

  //this pin didnt fire an interrupt for some reason, so just do a GPIO read to check its state - active low
  if(!HAL_GPIO_ReadPin(GPIOC, U7_int_Pin)){ //active low interrupt pin
    zoetrope.U7_interruptFlag = 1; //set flag to indicate I2C read needed
  }

  //interrupts have been a little hit or miss. If this continues, switch to a basic GPIO read method like U10
  //when interrupts work, they work really well. When they dont work, its usually just for a short time then it works great.
  if(zoetrope.U7_interruptFlag == 1){
    TCA9534_ReadInputs(&hi2c1, TCA_port2.address, &TCA_port2.data);
    zoetrope.U7_interruptFlag = 0; //clear flag
    zoetrope_assignHardwareI2CInputs(&zoetrope, 2, TCA_port2.data); //read U7 I2C I/O expander inputs and update system variables
  }

  //this pin didnt fire an interrupt for some reason, so just do a GPIO read to check its state - active low
  if(!HAL_GPIO_ReadPin(GPIOC, U10_int_Pin)){ //active low interrupt pin
    zoetrope.U10_interruptFlag = 1; //set flag to indicate I2C read needed
  }

  if(zoetrope.U10_interruptFlag == 1){
    TCA9534_ReadInputs(&hi2c1, TCA_port3.address, &TCA_port3.data);
    zoetrope.U10_interruptFlag = 0; //clear flag
    zoetrope_assignHardwareI2CInputs(&zoetrope, 3, TCA_port3.data); //read U10 I2C I/O expander inputs and update system variables
  }

  //zoetrope_readHardwareI2CInputs(&zoetrope, 1, TCA_port1.data); //read U3 I2C I/O expander inputs and update system variables
  //zoetrope_readHardwareI2CInputs(&zoetrope, 2, TCA_port2.data); //read U7 I2C I/O expander inputs and update system variables
  //zoetrope_readHardwareI2CInputs(&zoetrope, 3, TCA_port3.data); //read U10 I2C I/O expander inputs and update system variables
}


//////////////////////////
//////////NFC////////////
/////////////////////////
void NFC_writeDataToSticker(PN532* pn532){

  if(NFCDetectedFlag == 0){
    // Check if a card is available to read
    uid_len = PN532_ReadPassiveTarget(pn532, uid, PN532_MIFARE_ISO14443A, 1000);
    if (uid_len == PN532_STATUS_ERROR) {
      //printf(".");
    } else {
      //printf("Found card with UID: ");
      for (uint8_t i = 0; i < uid_len; i++) {
        //printf("%02x ", uid[i]);
      }
      NFCDetectedFlag = 1; //NFC sticker detected
      NFCFailFlag = 0;
    }
  }
  
  if(NFCDetectedFlag){ //write to sticker and read back to confirm good write
    block_number = 6; //start writing at page 6
    setNFCWriteBuffer(); //setup buffer we write to the NFC sticker
    for(uint8_t k=0; k<zoetrope_NFCPageCount; k++){
      //write one page at a time
      DATA[0] = NFCWriteBuffer[k][0];
      DATA[1] = NFCWriteBuffer[k][1];
      DATA[2] = NFCWriteBuffer[k][2];
      DATA[3] = NFCWriteBuffer[k][3];
      pn532_error = PN532_Ntag2xxWriteBlock(pn532, DATA, (block_number + k));
      if (pn532_error) {
        NFCFailFlag = 1;
      }
    }
    
    resetNFCReadBuffer(); //clear read buffer prior to reading back data
    for(uint8_t k=0; k<zoetrope_NFCPageCount; k++){
      //read one page at a time
      pn532_error = PN532_Ntag2xxReadBlock(pn532, DATA2, (block_number + k));
      if (pn532_error) {
        NFCFailFlag = 1;
      }
      NFCReadBuffer[k][0] = DATA2[0];
      NFCReadBuffer[k][1] = DATA2[1];
      NFCReadBuffer[k][2] = DATA2[2];
      NFCReadBuffer[k][3] = DATA2[3];
      //reset DATA2 for next read
      DATA2[0] = 0x00;
      DATA2[1] = 0x00;
      DATA2[2] = 0x00;
      DATA2[3] = 0x00;
    }

    //confirm good data read
    for(uint8_t k=0; k<zoetrope_NFCPageCount; k++){

      //check each page 1 at a time
      DATA[0] = NFCWriteBuffer[k][0];
      DATA[1] = NFCWriteBuffer[k][1];
      DATA[2] = NFCWriteBuffer[k][2];
      DATA[3] = NFCWriteBuffer[k][3];

      DATA2[0] = NFCReadBuffer[k][0];
      DATA2[1] = NFCReadBuffer[k][1];
      DATA2[2] = NFCReadBuffer[k][2];
      DATA2[3] = NFCReadBuffer[k][3];

      //check data for good read
      for (uint8_t i = 0; i < sizeof(DATA); i++) {
        if (DATA[i] != DATA2[i]) {
          NFCFailFlag = 1;
        }
      }

    }//end confirm good data read

    if(NFCFailFlag == 0){
      //successful read and write
      zoetrope.blueLED = 1; //turn on blue LED to indicate success
    }else{
      zoetrope.blueLED = 0; //turn off blue LED to indicate fail
    }
    //reset flag
    NFCDetectedFlag = 0;
    //reset data to have confidence in testing
    DATA2[0] = 0x00;
    DATA2[1] = 0x00;
    DATA2[2] = 0x00;
    DATA2[3] = 0x00;
  }
}

void setNFCWriteBuffer(void){
  for(uint8_t k=0; k<126; k++){
    NFCWriteBuffer[k][0] = k; //just an example pattern
    NFCWriteBuffer[k][1] = k;
    NFCWriteBuffer[k][2] = k;
    NFCWriteBuffer[k][3] = k;
  }
}

void resetNFCReadBuffer(void){
  for(uint8_t k=0; k<126; k++){
    NFCReadBuffer[k][0] = 0x00;
    NFCReadBuffer[k][1] = 0x00;
    NFCReadBuffer[k][2] = 0x00;
    NFCReadBuffer[k][3] = 0x00;
  }
}

/////////////////////////
//interrupt functions////
/////////////////////////


void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
	  zoetrope.motorSpeedPot = HAL_ADC_GetValue(&hadc1); //record the ADC value into the system variable
	  //zoetrope.PWMMotorSpeedValue = map(zoetrope.motorSpeedPot, 0, 65535, 0, 100); //I should read the actual min and max values and update here
}

//DMA complete callback function
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim){
  if(htim->Instance == TIM1){
        //encoder timer - do nothing
    }  
  
  if(htim->Instance == TIM2){
        //WS2812 PWM complete
        WS2812_PulseFinishedCallback(htim);
  }

  if(htim->Instance == TIM8){
      //motor PWM complete - do nothing
  }

  if(htim->Instance == TIM4){
    //strobe PWM complete - stop PWM transfer (transfer is re-activated on HALL sensor trigger)
    strobe_PulseFinishedCallback(htim);
  }
    
}

//void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
//{
	  //value = map(zoetrope.motorSpeedPot, 1700, 65535, 0, 100); //example - probably remove later
//}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
  //GPIO interrupts
  if (GPIO_Pin == GPIO_PIN_0) {
    //only use this when scanning for NFC tags (polling code otherwise)
    if(zoetrope.stateMachine_state == 10){ //NFC scan mode
      zoetrope_HALLSensorInterruptHandler2(&zoetrope); //call the HALL sensor interrupt handler
    }
  }
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
