/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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
#include "random.h"
#include "ws2812.h"
#include "stdlib.h"
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define TIMER_PERIOD_MS			20	// Период вызова функции обновления состояния
#define TIME_MIN_TICK			(TIME_MIN_MSEC / TIMER_PERIOD_MS)
#define TIME_MAX_TICK			(TIME_MAX_MSEC / TIMER_PERIOD_MS)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

CRC_HandleTypeDef hcrc;

SPI_HandleTypeDef hspi1;
DMA_HandleTypeDef hdma_spi1_tx;

TIM_HandleTypeDef htim1;

/* USER CODE BEGIN PV */
volatile uint8_t timFlag = 0;
volatile uint8_t dmaFlag = 1;
uint32_t rgbArr[LED_NUM]; // кольцевой массив
const uint32_t rgbConst[8]={0x000000, 0x00007F, 0x007F00, 0x007F7F, 0x7F0000, 0x7F007F, 0x7F7F00, 0x7F7F7F};
volatile uint32_t timerCounter;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_SPI1_Init(void);
static void MX_ADC1_Init(void);
static void MX_CRC_Init(void);
static void MX_TIM1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
	static uint32_t count = 0;
	count++;
	uint32_t adc = HAL_ADC_GetValue(hadc);
	uint8_t ans = Random_AddBit(adc);
	if (ans != 0)
		HAL_ADC_Start_IT(&hadc1);
	else
		__NOP();
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	timFlag = 1;
	timerCounter++;
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
	dmaFlag = 1 ;
}

// type  0 - linear; 1 - V образный
// при 0 - возврат 32 бит, при 1 - только 8
uint32_t GetRandom(uint8_t type)
{
	uint32_t rnd;
	static uint8_t isFirst = 1;
	if (isFirst)
	{
		isFirst = 0;
		rnd = Random_GetRnd();
		HAL_ADC_Start_IT(&hadc1);
		while (!rnd)
		{
			rnd = Random_GetRnd();
		};
		srand(rnd);
	}

	rnd = rand();
	if (type == 0)
		return rnd;

	// V
	return Random_GetV(rnd);
}

// Бегущие огни
void Prog_0()
{
	static uint32_t timer = 0;
	static uint32_t curr = 0;
	static uint32_t dest = 0;

	uint8_t* currP = (uint8_t*)&curr;
	uint8_t* destP = (uint8_t*)&dest;
	// Задаем цвет нулевому диоду
	if (!timer)
	{
		timer = 0xFF;
		curr = dest;
		do
		{
			dest = rgbConst[GetRandom(0)&7];
		}while (curr == dest);

	}
	else
	{
		timer--;
		for(int i=0; i<3; i++)
			if (destP[i])
			{
				if (currP[i] < 0xFF)
					currP[i]++;
			}
			else
			{
				if (currP[i] > 0)
					currP[i]--;
			}
	}

	// Передаем в массив
	for (int i = LED_NUM-1; i>0; i--)
		rgbArr[i] = rgbArr[i-1];

	rgbArr[0] = curr;

	for(int i=0; i<LED_NUM; i++)
		ws2813_AddRGB(rgbArr[i], i);
}

// Случайная вспышка
void Prog_1()
{
	for (int i=0; i<10; i++)
	{
		uint32_t rnd = rgbConst[GetRandom(0)&7];
		uint16_t pos = GetRandom(0) % LED_NUM;
		rgbArr[pos] = rnd;
	}
	for (int i=0; i<LED_NUM; i++)
		ws2813_AddRGB(rgbArr[i], i);
}


// Бегущие огни 2
void Prog_2()
{

	static uint32_t timer = 0;
	static uint32_t curr = 0;
	static uint16_t arrPos = 0; // указатель на начало массива

	// Задаем цвет нулевому диоду
	if (!timer)
	{
		timer = 25;//(TIME_MIN_TICK + GetRandom(0)) % TIME_MAX_TICK;
		curr = rgbConst[GetRandom(0)&7];

	}
	else
	{
		timer--;
	}

	rgbArr[arrPos] = curr;

	// Передаем в массив
	int pos = LED_NUM - 1;
	for (int i=arrPos; i<LED_NUM; i++)
		ws2813_AddRGB(rgbArr[i], pos--);
	for(int i=0; i<arrPos; i++)
		ws2813_AddRGB(rgbArr[i], pos--);

	arrPos++;
	if (arrPos == LED_NUM)
		arrPos = 0;

}

// Бегущие огни
void Prog_3()
{
	#define MAX_NUM 16
	static int16_t pos[MAX_NUM];
	static uint32_t rgb[MAX_NUM];

	static uint8_t isFirst = 1;
	static int delay = 0;
	if (isFirst)
	{
		isFirst = 0;
		for (int i=0; i<MAX_NUM; i++)
		{
			pos[i] = LED_NUM * i / MAX_NUM;
			rgb[i] = rgbConst[GetRandom(0)&7];
		}
	}

	if ((delay++ & 0x1F) == 0)
	{
		for (int i=0; i<MAX_NUM; i++)
			if (++pos[i]==LED_NUM)
			{
				rgb[i] = rgbConst[GetRandom(0)&7];
				pos[i] = 0;
			}

		// Передаем в массив
		memset(rgbArr, 0, LED_NUM*4);
		for (int i=0; i<MAX_NUM; i++)
			rgbArr[pos[i]] = rgb[i];
	}
	for(int i=0; i<LED_NUM; i++)
		ws2813_AddRGB(rgbArr[i], i);

}

void Prog_4()
{
	#define DIVIDER (50*5) // 5сек
	static uint32_t tim = 0;
	if (!tim)
	{
		tim = DIVIDER;
		for( int i=0; i<LED_NUM; i++)
			rgbArr[i] = rgbConst[GetRandom(0)&7];
	}
	tim--;

	for(int i=0; i<LED_NUM; i++)
		ws2813_AddRGB(rgbArr[i], i);
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
  MX_DMA_Init();
  MX_SPI1_Init();
  MX_ADC1_Init();
  MX_CRC_Init();
  MX_TIM1_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_Base_Start_IT(&htim1);
  //HAL_TIM_Base_Start_IT(&htim1);
  HAL_ADC_Start_IT(&hadc1);
  ws2813_FillConvertBuffer();
//  HAL_TIM_Base_Start_IT(&htim1);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  static uint8_t currentProg = 3;
  while (1)
  {
		if (timerCounter == 50*60*3) // 3 минуты
			{
				timerCounter = 0;
				currentProg++;
			}

			switch(currentProg)
			{
				case 0:
					Prog_0();
					break;
				case 1:
					Prog_1();
					break;
				case 2:
					Prog_2();
					break;
				case 3:
					Prog_3();
					break;
				case 4:
					Prog_4();
					break;
				default:
					currentProg = 0;
					break;
			}

			while(!dmaFlag);
			dmaFlag = 0;

			while(!timFlag);
			timFlag = 0;

			uint8_t *cdmaBuff = ws2813_GetDMAbuff();
			HAL_SPI_Transmit_DMA(&hspi1, cdmaBuff, DMA_BUFF_SIZE);
			ws2813_ChangeCurrentBuff();
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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
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
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
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

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_TEMPSENSOR;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief CRC Initialization Function
  * @param None
  * @retval None
  */
static void MX_CRC_Init(void)
{

  /* USER CODE BEGIN CRC_Init 0 */

  /* USER CODE END CRC_Init 0 */

  /* USER CODE BEGIN CRC_Init 1 */

  /* USER CODE END CRC_Init 1 */
  hcrc.Instance = CRC;
  if (HAL_CRC_Init(&hcrc) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CRC_Init 2 */

  /* USER CODE END CRC_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_2EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

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

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 71;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 20000;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
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
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel3_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

}

/* USER CODE BEGIN 4 */

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
