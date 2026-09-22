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
#include "adc.h"
#include "dma.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define RX_BUFFER_SIZE 256 // USART 수신 버퍼 크기
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint8_t PCrxBuffer[RX_BUFFER_SIZE];

uint16_t rb_head = 0;

uint16_t _500ms_count = 0; // 5ms타이머를 이용해서 0.5s를 만듦

uint8_t is_len_on = 0;

// 어제자 과제에서 가져옴
uint8_t SW_DATA; // sw 데이터. LSB(오른쪽)부터 PB12,13,14,15

// CRC만 AI에게 맞긴 패킷
const uint8_t torq_on[] = { 0xFF, 0xFF, 0xFD, 0x00, 0x0E, 0x06, 0x00, 0x03,
		0x40, 0x00, 0x01, 0x2B, 0x69 };
const uint8_t pos_center[] = { 0xFF, 0xFF, 0xFD, 0x00, 0x0E, 0x09, 0x00, 0x03,
		0x74, 0x00, 0x00, 0x04, 0x00, 0x00, 0x71, 0x29 };
const uint8_t pos_p90[] = { 0xFF, 0xFF, 0xFD, 0x00, 0x0E, 0x09, 0x00, 0x03,
		0x74, 0x00, 0x00, 0x08, 0x00, 0x00, 0x81, 0x29 };
const uint8_t pos_m90[] = { 0xFF, 0xFF, 0xFD, 0x00, 0x0E, 0x09, 0x00, 0x03,
		0x74, 0x00, 0x00, 0x00, 0x00, 0x00, 0x22, 0xA9 };
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

// 스위치 데이터를 세팅하는 함수
void sw_data_set();

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

//패킷 송신
void pkt_send(const uint8_t *pkt, uint8_t len) {
	for (uint8_t i = 0; i < len; i++) {
		while (!LL_USART_IsActiveFlag_TXE(USART3))
			;
		LL_USART_TransmitData8(USART3, pkt[i]);
	}
	while (!LL_USART_IsActiveFlag_TC(USART3))
		;
}
/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {

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
	MX_ADC1_Init();
	MX_TIM8_Init();
	MX_TIM6_Init();
	MX_USART3_UART_Init();
	/* USER CODE BEGIN 2 */

	//timer6, 5ms period
	HAL_TIM_Base_Start_IT(&htim6);
	LL_DMA_SetMemoryAddress(DMA1, LL_DMA_STREAM_1, (uint32_t) PCrxBuffer); // 수신 메모리 주소 설정(DMA를 이용해서 USART -> 메모리)
	LL_DMA_SetPeriphAddress(DMA1, LL_DMA_STREAM_1, (uint32_t) &USART3->DR);
	LL_DMA_SetDataLength(DMA1, LL_DMA_STREAM_1, RX_BUFFER_SIZE);
	LL_DMA_EnableStream(DMA1, LL_DMA_STREAM_1);
	LL_USART_EnableDMAReq_RX(USART3);
	LL_USART_EnableIT_IDLE(USART3);

	pkt_send(torq_on, sizeof(torq_on));
	HAL_Delay(10);

	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1) {
		/* USER CODE END WHILE */
		HAL_Delay(100);
		sw_data_set(); // 스위치 값 읽어옴

		if (SW_DATA & 0x01)
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
		else {
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
			pkt_send(pos_center, sizeof(pos_center));
			continue;
		}
		if (SW_DATA & 0x02)
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_SET);
		else {
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET);
			pkt_send(pos_p90, sizeof(pos_p90));
			continue;
		}
		if (SW_DATA & 0x04)
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_SET);
		else {
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_RESET);
			pkt_send(pos_m90, sizeof(pos_m90));
			continue;
		}

	}
	/* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

	/** Configure the main internal regulator output voltage
	 */
	__HAL_RCC_PWR_CLK_ENABLE();
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLM = 8;
	RCC_OscInitStruct.PLL.PLLN = 180;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
	RCC_OscInitStruct.PLL.PLLQ = 2;
	RCC_OscInitStruct.PLL.PLLR = 2;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}

	/** Activate the Over-Drive mode
	 */
	if (HAL_PWREx_EnableOverDrive() != HAL_OK) {
		Error_Handler();
	}

	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) {
		Error_Handler();
	}
}

/* USER CODE BEGIN 4 */
void writePacket(uint8_t packet[], int length) {
	for (int i = 0; i < length; i++) {
		while (!LL_USART_IsActiveFlag_TXE(USART3))
			;
		LL_USART_TransmitData8(USART3, packet[i]);
	}
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
	if (htim->Instance == TIM6) {

	}
}

// 스위치 세팅
void sw_data_set(void) {
	if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_12))
		SW_DATA |= 0x01;
	else
		SW_DATA &= ~(0x01);

	if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_13))
		SW_DATA |= 0x02;
	else
		SW_DATA &= ~(0x02);

	if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_14))
		SW_DATA |= 0x04;
	else
		SW_DATA &= ~(0x04);

	if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_15))
		SW_DATA |= 0x08;
	else
		SW_DATA &= ~(0x08);
}

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1) {
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
