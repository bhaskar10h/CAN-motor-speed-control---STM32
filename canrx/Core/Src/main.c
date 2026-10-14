#include "main.h"
TIM_HandleTypeDef htim2;
CAN_HandleTypeDef hcan;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_CAN_Init(void);
static void MX_TIM2_Init(void);

/* USER CODE BEGIN 0 */
CAN_FilterTypeDef sFilterConfig;
CAN_RxHeaderTypeDef RxHeader;
uint8_t RxData[8];

void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
  HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO1, &RxHeader, RxData);

  int8_t receivedSpeed = (int8_t)RxData[7];

  uint32_t pulseWidth = 1500 + (receivedSpeed * 5);

  if(pulseWidth > 2000) pulseWidth = 2000;
  if(pulseWidth < 1000) pulseWidth = 1000;

  __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, pulseWidth);
}

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_CAN_Init();
  MX_TIM2_Init(); // 3. Initialize the Timer

  /* USER CODE BEGIN 2 */
  HAL_CAN_Start(&hcan);

  // 4. START THE PWM SIGNAL
  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);

   // Configure the filter (Same as before)
   sFilterConfig.FilterActivation = CAN_FILTER_ENABLE;
   sFilterConfig.FilterFIFOAssignment = CAN_FILTER_FIFO1;
   sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
   sFilterConfig.FilterIdHigh = 0x6A5<<5;
   sFilterConfig.FilterIdLow = 0;
   sFilterConfig.FilterMaskIdHigh = 0x7FF<<5;
   sFilterConfig.FilterMaskIdLow = 0;
   sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
   HAL_CAN_ConfigFilter(&hcan, &sFilterConfig);

   HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO1_MSG_PENDING);
  /* USER CODE END 2 */

  while (1)
  {
      // Main loop is empty; everything happens in the Callback!
  }
}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9; // 72MHz Clock
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

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
}

// --- 5. TIMER 2 INITIALIZATION (The New Part) ---
static void MX_TIM2_Init(void)
{
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  htim2.Instance = TIM2;

  htim2.Init.Prescaler = 71;

  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 19999;
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
  sConfigOC.Pulse = 1500; // Start at 1500 (Stop/Neutral)
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }

  __HAL_RCC_GPIOA_CLK_ENABLE();

}

static void MX_CAN_Init(void)
{
  hcan.Instance = CAN1;
  hcan.Init.Prescaler = 72;
  hcan.Init.Mode = CAN_MODE_NORMAL;
  hcan.Init.SyncJumpWidth = CAN_SJW_1TQ;
  hcan.Init.TimeSeg1 = CAN_BS1_2TQ;
  hcan.Init.TimeSeg2 = CAN_BS2_1TQ;
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
}

static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  // Configure PA0 as Alternate Function for TIM2 (PWM Output)
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP; // Alternate Function Push Pull
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}
