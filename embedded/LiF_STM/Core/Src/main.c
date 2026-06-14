/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  * @author			: Nick Kapuka
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
// ------------- NK UPDATE -------------:

// alternate between the different boards; used to setup faster FLASHING
#define CC_BOARD	0					// Cab STM32
#define F1_BOARD	1					// Floor 1 STM32
#define F2_BOARD	2					// Floor 2 STM32
#define F3_BOARD	3					// Floor 3 STM32

// ********************************* CHANGE PER BOARD *********************************
#define SELECTED_BOARD	CC_BOARD		// currently selected board; change when flashing
// ************************************************************************************

// CAN IDs:
#define SC_ID				0x100		// Supervisor Controller; R-Pi
#define EC_ID				0x101		// Elevator Controller; arduino
#define CC_ID				0x200		// Cab Controller; STM32
#define F1_ID				0x201		// Floor 1 controller; STM32
#define F2_ID				0x202		// Floor 2 controller; STM32
#define F3_ID				0x203		// Floor 3 controller; STM32

// variable names for each floor
#define NO_FLOOR			0			// no floor; used for Cab
#define FLOOR_1				1
#define FLOOR_2				2
#define FLOOR_3				3

// pre-comp code block for configuring STM32 FLASHING
#if SELECTED_BOARD == CC_BOARD
	#define MY_ID		CC_ID
	#define MY_FLOOR	NO_FLOOR

// if floor 1 board
#elif SELECTED_BOARD == F1_BOARD
	#define MY_ID		F1_ID
	#define MY_FLOOR	FLOOR_1

// if floor 2 board
#elif	SELECTED_BOARD == F2_BOARD
	#define MY_ID		F2_ID
	#define MY_FLOOR	FLOOR_2

// if floor 3 board
#elif	SELECTED_BOARD == F3_BOARD
	#define MY_ID		F3_ID
	#define MY_FLOOR	FLOOR_3

// error check
#else
	#error "Invalid SELECTED_BOARD"

#endif

#define	NO_BUTTON_PRESSED	0		// default value for button status
#define BLUE_BUTTON_PRESS	1		// blue button will act as a 'homing' and send elv down to F1
#define	F1_BUTTON_PRESS		1		// req to go to floor 1 PB pressed
#define F2_BUTTON_PRESS		2		// req to go to floor 2 PB pressed
#define F3_BUTTON_PRESS		3		// req to go to floor 3 PB pressed

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
CAN_HandleTypeDef hcan;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
// ------------- NK UPDATE -------------:
CAN_TxHeaderTypeDef		TxHeader;						// variable of type CAN_TxHeaderTypeDef
CAN_RxHeaderTypeDef		RxHeader;						// variable of type CAN_RxHeaderTypeDef
uint8_t					TxData[8];						// 8 bytes of data per frame
uint8_t					RxData[8];						// 8 bytes of data per frame
uint32_t				TxMailbox;
volatile uint8_t		BUTTON = NO_BUTTON_PRESSED;		// initial value is that no button has been pressed

// receive initializations
volatile uint8_t CAN_MsgPending	= 0;		// flag for a pending message
volatile uint32_t rx_ID 	 	= 0;		// received message ID (only listen to EC's 0x101 ID)
volatile uint8_t rx_Byte0 		= 0;		// used to store numbers from first byte
uint8_t	 elevator_en 	= 0;				// elevator enabled flag (part of the message that EC sends)
uint8_t	 current_floor 	= 0;				// current floor value (part of the message that EC sends)
											// note: these are used in RxCallback function


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_CAN_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */
void CAN_ByteTransmit(uint32_t std_id, uint8_t data0);
void blinkLED(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, uint32_t delay_ms);
void CAN_ProcessReceive(void);
void PB_Process(uint8_t button);
void showFloor(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
// ------------- NK UPDATE -------------:

// make a new function for transmitting CAN message:
// note, this setup is almost the same as the MX_CAN_Init
void CAN_ByteTransmit(uint32_t std_id, uint8_t data0){
	// set the TxHeader's stdID to std_id that you can pass into the function (message type and message data)
	// config:
	TxHeader.StdId = std_id;				// same as MX_CAN
	TxHeader.IDE = CAN_ID_STD;				// same as MX_CAN
	TxHeader.RTR = CAN_RTR_DATA;			// same as MX_CAN
	TxHeader.DLC = 1;						// same as MX_CAN
	TxHeader.TransmitGlobalTime = DISABLE;	// same as MX_CAN

	TxData[0] = data0;						// in the first byte of TxData, put the data you want to send

	// handle error
    if (HAL_CAN_AddTxMessage(&hcan, &TxHeader, TxData, &TxMailbox) != HAL_OK)
    {
        Error_Handler();
    }


    // note, the goal is to use this function as:
    // CAN_ByteTransmit(MY_ID, FLOOR_X);
}

void blinkLED(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, uint32_t delay_ms){
	 // blink LED briefly
		HAL_GPIO_WritePin(GPIOx, GPIO_Pin, GPIO_PIN_SET);
		HAL_Delay(delay_ms);
		HAL_GPIO_WritePin(GPIOx, GPIO_Pin, GPIO_PIN_RESET);
}


// to show what floor elevator is on:
void showFloor(void)
{
    // turn all floor indicator LEDs off first
    HAL_GPIO_WritePin(F1_Indicator_LED_GPIO_Port, F1_Indicator_LED_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(F2_Indicator_LED_GPIO_Port, F2_Indicator_LED_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(F3_Indicator_LED_GPIO_Port, F3_Indicator_LED_Pin, GPIO_PIN_RESET);

    if(current_floor == FLOOR_1)
    {
        HAL_GPIO_WritePin(F1_Indicator_LED_GPIO_Port, F1_Indicator_LED_Pin, GPIO_PIN_SET);
    }
    else if(current_floor == FLOOR_2)
    {
        HAL_GPIO_WritePin(F2_Indicator_LED_GPIO_Port, F2_Indicator_LED_Pin, GPIO_PIN_SET);
    }
    else if(current_floor == FLOOR_3)
    {
        HAL_GPIO_WritePin(F3_Indicator_LED_GPIO_Port, F3_Indicator_LED_Pin, GPIO_PIN_SET);
    }
}


void CAN_ProcessReceive(void){
	// check if received ID is the ID of the EC
	if (rx_ID == EC_ID)
	{
		// shift the 3rd bit (the actual EC en bit; >> 2) down to bit 1 and keep that (& 0x01)
		elevator_en = (rx_Byte0 >> 2) & 0x01;

		// only read the first two bits
		current_floor = rx_Byte0 & 0x03;
	}
}


void PB_Process(uint8_t button){
// CC exclusive code; won't run unless CC is selected using SELECTED_BOARD
#if SELECTED_BOARD == CC_BOARD

	// PB1/2/3 means go to floor 1/2/3
	if(button == F1_BUTTON_PRESS)
	{
		CAN_ByteTransmit(MY_ID, FLOOR_1);
		// illuminate PB LED for visual confirmation
		blinkLED(PB1_LED_GPIO_Port, PB1_LED_Pin, 200);
		current_floor = 1;

	}
	else if(button == F2_BUTTON_PRESS)
	{
		CAN_ByteTransmit(MY_ID, FLOOR_2);
		// illuminate PB LED for visual confirmation
		blinkLED(PB2_LED_GPIO_Port, PB2_LED_Pin, 200);
		current_floor = 2;

	}
	else if(button == F3_BUTTON_PRESS)
	{
		CAN_ByteTransmit(MY_ID, FLOOR_3);
		// illuminate PB LED for visual confirmation
		blinkLED(PB3_LED_GPIO_Port, PB3_LED_Pin, 200);
		current_floor = 3;

	}

// process button input differently based on what STM32 is used (car vs floor controllers):
#elif SELECTED_BOARD == F1_BOARD
	if(button == F1_BUTTON_PRESS){
		CAN_ByteTransmit(MY_ID, 0x01);
		// illuminate PB LED for visual confirmation
		blinkLED(PB1_LED_GPIO_Port, PB1_LED_Pin, 200);
	}

#elif SELECTED_BOARD == F2_BOARD
	if(button == F2_BUTTON_PRESS){
		CAN_ByteTransmit(MY_ID, 0x01);
		// illuminate PB LED for visual confirmation
		blinkLED(PB2_LED_GPIO_Port, PB2_LED_Pin, 200);
	}

#elif SELECTED_BOARD == F3_BOARD
	if(button == F3_BUTTON_PRESS){
		CAN_ByteTransmit(MY_ID, 0x01);
		// illuminate PB LED for visual confirmation
		blinkLED(PB3_LED_GPIO_Port, PB3_LED_Pin, 200);
	}
#endif
}






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
  MX_CAN_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  	  // always display current floor on floor PCBs and car cab PCB
	  	  showFloor();
	  	  // Receive messages
	  	  // check if msgpending (if msg received)
	  	 if (CAN_MsgPending)
	  	 {
	  		 // flag msg as read by reseting MsgPending
	  		 CAN_MsgPending = 0;
	  		 // process msg; grab the two values from EC regarding position (floor #) and status (en or !en)
	  		 CAN_ProcessReceive();
	  	 }

	  	 // Transmit messages
	  	 // check if input present
	  	 if(BUTTON != NO_BUTTON_PRESSED)
	  	 {
	  		 // temporarily store before processing
	  		 // avoids weird interrupt flags (like if BUTTON is updated before msg read)
	  		 uint8_t pressed_button = BUTTON;
	  		 // reset BUTTON since it's input has been captured
	  		 BUTTON = NO_BUTTON_PRESSED;
	  		 // process button
	  		 PB_Process(pressed_button);
	  		 // blink green LED briefly for transmit successful
	  		 blinkLED(GPIOA, Green_LED_Pin, 500);

	  	 }
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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART2;
  PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief CAN Initialization Function
  * @param None
  * @retval None
  */
static void MX_CAN_Init(void)
{

  /* USER CODE BEGIN CAN_Init 0 */

  /* USER CODE END CAN_Init 0 */

  /* USER CODE BEGIN CAN_Init 1 */

  /* USER CODE END CAN_Init 1 */
  hcan.Instance = CAN;
  hcan.Init.Prescaler = 32;
  hcan.Init.Mode = CAN_MODE_NORMAL;
  hcan.Init.SyncJumpWidth = CAN_SJW_1TQ;
  hcan.Init.TimeSeg1 = CAN_BS1_4TQ;
  hcan.Init.TimeSeg2 = CAN_BS2_4TQ;
  hcan.Init.TimeTriggeredMode = DISABLE;
  hcan.Init.AutoBusOff = DISABLE;
  hcan.Init.AutoWakeUp = DISABLE;
  hcan.Init.AutoRetransmission = DISABLE;
  hcan.Init.ReceiveFifoLocked = DISABLE;
  hcan.Init.TransmitFifoPriority = DISABLE;
  if (HAL_CAN_Init(&hcan) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CAN_Init 2 */

  // ------------- NK UPDATE -------------:
  CAN_FilterTypeDef filter;		// this is one of the 13 filters; can create more filters but this will be #0

  // configure filter 0 to direct everything to FIFO 0
  filter.FilterBank = 0;				// this will filter number 0
  filter.FilterIdHigh = 0x0100 << 5;	// Set FilterIdHigh bits by choosing an ID and aligning the bits in the filter register with the receive register by shifting << 5
  filter.FilterIdLow = 0x0000;			// not using FilterIdLow bits (sets as dc/low)
  filter.FilterMaskIdHigh = 0xFFC << 5;	// Same as example in lecture (this gives a range of ID's that will be accept between 0x100 and 0x103). Must also align bits?
  filter.FilterMaskIdLow = 0x0000;		// not using FilterMaskIdLow bits (set as dc/low)
  filter.FilterFIFOAssignment = CAN_FILTER_FIFO0;
  filter.FilterMode = CAN_FILTERMODE_IDMASK;	// uses Mask Mode (so cans et range of IDs)
  filter.FilterScale = CAN_FILTERSCALE_32BIT;	//use 32 bit filters
  filter.FilterActivation = ENABLE;				// by default the filters are disabled so enable
  filter.SlaveStartFilterBank = 0;

  if(HAL_CAN_ConfigFilter(&hcan, &filter) != HAL_OK){ // set above values for filter 0?
	  Error_Handler();
  }

  // start CAN peripheral
  if(HAL_CAN_Start(&hcan) != HAL_OK){
	  Error_Handler();
  }

  // Activate CAN Rx notification interrupt
  if(HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK){
	  Error_Handler();
  }

  // prepare header fields for standard mode CAN transmission
  TxHeader.IDE = CAN_ID_STD; 			// using std mode. Note this CAN_ID_EXT for extended mode?
  TxHeader.ExtId = 0x00;				// extended id is not used?
  // ------------------------------------------------------------------------------------------
  TxHeader.StdId = MY_ID;				// standard mode ID is NODE specific. Currently using 0x200 for CC  controller on THIS board!
  // ------------------------------------------------------------------------------------------
  TxHeader.RTR = CAN_RTR_DATA;			// send a data frame not an RTR?
  TxHeader.DLC = 1;						// data length code = 1 (only send 1 byte)
  TxHeader.TransmitGlobalTime = DISABLE;



  /* USER CODE END CAN_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

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
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, PB1_LED_Pin|Green_LED_Pin|PB2_LED_Pin|PB3_LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(F1_Indicator_LED_GPIO_Port, F1_Indicator_LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, F2_Indicator_LED_Pin|F3_Indicator_LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : Blue_Button_Pin */
  GPIO_InitStruct.Pin = Blue_Button_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(Blue_Button_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PB1_LED_Pin Green_LED_Pin PB2_LED_Pin PB3_LED_Pin */
  GPIO_InitStruct.Pin = PB1_LED_Pin|Green_LED_Pin|PB2_LED_Pin|PB3_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : F1_Indicator_LED_Pin */
  GPIO_InitStruct.Pin = F1_Indicator_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(F1_Indicator_LED_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : F2_Indicator_LED_Pin F3_Indicator_LED_Pin */
  GPIO_InitStruct.Pin = F2_Indicator_LED_Pin|F3_Indicator_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PushButton_1_Pin PushButton_3_Pin PushButton_2_Pin */
  GPIO_InitStruct.Pin = PushButton_1_Pin|PushButton_3_Pin|PushButton_2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
// ------------- NK CHANGE -------------:
// Override the HAL_CAN_RxFifo0MsgPending_Callback function
// this is called when the interrupt for FIFO0 is triggered

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan){

	// get Rx Msg and store in RxData buffer
	if(HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &RxHeader, RxData) != HAL_OK)
	{
		// reception error
		Error_Handler();
	}

	rx_ID = RxHeader.StdId;		// grab the ID of the message and store it locally
	rx_Byte0 = RxData[0];		// read and store first byte locally
	CAN_MsgPending = 1;			// message received and set high for process in main

	// goal: store these read values in main and process them there
	// use-case: 'msg receive (msgPending) with rx_ID, of size 1 byte (first byte)'

}

// override the HAL_GPIO Callback
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
	//set the button flag to indicate which button was pressed
	// GPIO pin 13 is the 'blue' button, or BLUE_BUTTON_PRESS in my program
	if(GPIO_Pin == Blue_Button_Pin){
		// blue button pressed
		BUTTON = BLUE_BUTTON_PRESS;
	}

	// check to see if the other elevator PBs have been pressed and associate the physical pin
	// with the defined value
	// note, the naming scheme for these seems to be "[customNameInMX]_Pin"
	if(GPIO_Pin == PushButton_1_Pin){
		BUTTON = F1_BUTTON_PRESS;
	} else if(GPIO_Pin == PushButton_2_Pin){
		BUTTON = F2_BUTTON_PRESS;
	}else if(GPIO_Pin == PushButton_3_Pin){
		BUTTON = F3_BUTTON_PRESS;
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

#ifdef  USE_FULL_ASSERT
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
