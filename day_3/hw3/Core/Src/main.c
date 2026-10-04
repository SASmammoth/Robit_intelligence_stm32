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
#include "can.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define MOTOR_ID   1       // 확인할 모터 ID
#define HOST_ID    0xFD

#define RS_ENABLE    3
#define RS_STOP      4
#define RS_WRITE     18

#define IDX_RUN_MODE 0x7005   // 0 MIT, 1 위치, 2 속도, 3 전류
#define IDX_SPD_REF  0x700A   // 목표 속도 [rad/s]
#define IDX_LIM_CUR  0x7018   // 전류 제한 [A]
#define IDX_ACC_RAD  0x7022   // 가속도 [rad/s^2]

#define SPEED        3.0f     // 목표 속도 크기
#define PERIOD_MS    10000     // 방향 전환 주기

#define P_MIN  (-12.566371f)   // -4π
#define P_MAX  ( 12.566371f)   //  4π
#define V_MIN  (-33.0f)
#define V_MAX  ( 33.0f)
#define T_MIN  (-14.0f)
#define T_MAX  ( 14.0f)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
volatile uint8_t  result = 0;      // 1 Private, 2 MIT, 3 CANopen, 4 응답 없음
volatile uint32_t rx_count = 0;    // 받은 프레임 수
volatile uint32_t rx_id = 0;       // 마지막으로 받은 ID
volatile uint8_t  rx_ext = 0;      // 1: 확장 ID, 0: 표준 ID
volatile uint8_t  rx_data[8];      // 마지막으로 받은 데이터
volatile uint32_t esr = 0;         // CAN 에러 레지스터

volatile uint32_t start_st = 0xFF;   // HAL_CAN_Start 결과 (0이면 OK)
volatile uint32_t tx_st = 0xFF;      // 마지막 송신 결과 (0이면 OK)
volatile uint32_t can_err = 0;       // HAL 에러 코드
volatile uint8_t  found_id = 0;      // ID 스캔 결과


volatile float spd_cmd = 0.0f;   // 현재 지령 속도 (Live Expressions로 확인)


volatile float    enc_pos = 0.0f;    // 현재 각도 [rad]
volatile float    enc_vel = 0.0f;    // 현재 속도 [rad/s]
volatile float    enc_tor = 0.0f;    // 현재 토크 [N·m]
volatile float    enc_temp = 0.0f;   // 온도 [℃]
volatile uint8_t  motor_mode = 0;    // 0 리셋, 1 캘리브레이션, 2 동작 중
volatile uint8_t  motor_fault = 0;   // 폴트 비트 (0이면 정상)
volatile uint32_t fb_count = 0;      // 받은 피드백 수

volatile float enc_pos_total = 0.0f;   // 보정된 누적 각도 [rad]
volatile int32_t wrap_count = 0;       // 경계를 넘은 횟수 (+면 overflow, -면 underflow)
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
// 0~65535 정수를 실제 값으로 변환
static float uint_to_float(uint16_t x, float xmin, float xmax)
{
  return xmin + (xmax - xmin) * (float)x / 65535.0f;
}

// 수신 인터럽트: 프레임이 들어오면 자동으로 호출됨
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
  CAN_RxHeaderTypeDef h;
  uint8_t d[8];
  if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &h, d) == HAL_OK) {
    rx_ext = (h.IDE == CAN_ID_EXT);
    rx_id  = rx_ext ? h.ExtId : h.StdId;
    for (int i = 0; i < 8; i++) rx_data[i] = d[i];
    rx_count++;

    // 통신 타입 2: 피드백 프레임 해석
    if (rx_ext && ((rx_id >> 24) & 0x1F) == 2) {
          enc_pos  = uint_to_float((d[0] << 8) | d[1], P_MIN, P_MAX);
          enc_vel  = uint_to_float((d[2] << 8) | d[3], V_MIN, V_MAX);
          enc_tor  = uint_to_float((d[4] << 8) | d[5], T_MIN, T_MAX);
          enc_temp = ((d[6] << 8) | d[7]) / 10.0f;

          // overflow / underflow 보정
          static float prev_pos = 0.0f;
          static uint8_t first = 1;
          float diff = enc_pos - prev_pos;

          if (!first) {
            if (diff < -12.566371f) wrap_count++;   // +4π → -4π : overflow
            if (diff >  12.566371f) wrap_count--;   // -4π → +4π : underflow
          }
          first = 0;
          prev_pos = enc_pos;
          enc_pos_total = enc_pos + wrap_count * 25.132741f;   // 8π

          motor_mode  = (rx_id >> 22) & 0x03;
          motor_fault = (rx_id >> 16) & 0x3F;
          fb_count++;
        }
  }
}

// 프레임 하나 보내기 (ext=1이면 확장 ID)
void can_send(uint32_t id, uint8_t ext, uint8_t *d)
{
  CAN_TxHeaderTypeDef h = {0};
  uint32_t mailbox;
  if (ext) { h.IDE = CAN_ID_EXT; h.ExtId = id; }
  else     { h.IDE = CAN_ID_STD; h.StdId = id; }
  h.RTR = CAN_RTR_DATA;
  h.DLC = 8;
  tx_st = HAL_CAN_AddTxMessage(&hcan1, &h, d, &mailbox);
}

// 29비트 ID 만들기
static uint32_t rs_id(uint8_t type)
{
  return ((uint32_t)type << 24) | ((uint32_t)HOST_ID << 8) | MOTOR_ID;
}

void rs_enable(void)
{
  uint8_t d[8] = {0};
  can_send(rs_id(RS_ENABLE), 1, d);
}

void rs_stop(void)
{
  uint8_t d[8] = {0};
  can_send(rs_id(RS_STOP), 1, d);
}

// float 파라미터 쓰기
void rs_write_float(uint16_t index, float value)
{
  uint8_t d[8] = {0};
  d[0] = index & 0xFF;
  d[1] = index >> 8;
  memcpy(&d[4], &value, 4);      // STM32는 리틀 엔디안이라 그대로 복사
  can_send(rs_id(RS_WRITE), 1, d);
}

// 1바이트 파라미터 쓰기 (run_mode용)
void rs_write_u8(uint16_t index, uint8_t value)
{
  uint8_t d[8] = {0};
  d[0] = index & 0xFF;
  d[1] = index >> 8;
  d[4] = value;
  can_send(rs_id(RS_WRITE), 1, d);
}

// 보내고 50ms 동안 응답이 오는지 확인
uint8_t send_and_wait(uint32_t id, uint8_t ext, uint8_t *d)
{
  uint32_t before = rx_count;
  can_send(id, ext, d);
  HAL_Delay(50);
  esr = hcan1.Instance->ESR;
  HAL_CAN_AbortTxRequest(&hcan1, CAN_TX_MAILBOX0 | CAN_TX_MAILBOX1 | CAN_TX_MAILBOX2);
  return rx_count != before;    // 새 프레임이 왔으면 1
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
  MX_CAN1_Init();
  /* USER CODE BEGIN 2 */
  // 모든 ID 수신하도록 필터 설정
  CAN_FilterTypeDef f = {0};
  f.FilterBank = 0;
  f.FilterMode = CAN_FILTERMODE_IDMASK;
  f.FilterScale = CAN_FILTERSCALE_32BIT;
  f.FilterFIFOAssignment = CAN_RX_FIFO0;
  f.FilterActivation = ENABLE;
  f.SlaveStartFilterBank = 14;
  HAL_CAN_ConfigFilter(&hcan1, &f);

  start_st = HAL_CAN_Start(&hcan1);
  can_err = hcan1.ErrorCode;
  HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING);
  HAL_Delay(100);

  uint8_t d[8];

  // 1) Private: Device ID 요청 (확장 ID)
  memset(d, 0, 8);
  if (send_and_wait((0u << 24) | (HOST_ID << 8) | MOTOR_ID, 1, d) && rx_ext)
    result = 1;

  // 2) CANopen: SDO로 0x1000 읽기 요청
  if (result == 0) {
    uint8_t sdo[8] = {0x40, 0x00, 0x10, 0x00, 0, 0, 0, 0};
    if (send_and_wait(0x600 + MOTOR_ID, 0, sdo) && rx_id == 0x580 + MOTOR_ID)
      result = 3;
  }

  // 3) MIT: Disable 명령 (모터 출력 끄는 명령이라 안전)
  if (result == 0) {
    uint8_t mit[8] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFD};
    if (send_and_wait(MOTOR_ID, 0, mit) && !rx_ext)
      result = 2;
  }

  if (result == 0)
    result = 4;    // 아무 응답 없음
  if (result == 4) {
    for (uint8_t id = 1; id <= 127 && found_id == 0; id++) {
      memset(d, 0, 8);
      if (send_and_wait((HOST_ID << 8) | id, 1, d)) { found_id = id; result = 1; break; }

      uint8_t mit[8] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFD};
      if (send_and_wait(id, 0, mit)) { found_id = id; result = 2; break; }
    }
  }

  HAL_Delay(500);                     // 모터 부팅 대기

    rs_stop();                          // 모드 변경은 정지 상태에서
    HAL_Delay(10);
    rs_write_u8(IDX_RUN_MODE, 2);       // 속도 모드
    HAL_Delay(10);
    rs_enable();
    HAL_Delay(10);
    rs_write_float(IDX_LIM_CUR, 5.0f);  // 전류 제한 5A (정격 8A보다 낮게)
    HAL_Delay(10);
    rs_write_float(IDX_ACC_RAD, 10.0f); // 가속도 (방향 전환 충격 줄이기)
    HAL_Delay(10);

    spd_cmd = SPEED;
    uint32_t last_toggle = HAL_GetTick();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  if (HAL_GetTick() - last_toggle >= PERIOD_MS) {
	        spd_cmd = -spd_cmd;               // 방향 반전
	        last_toggle = HAL_GetTick();
	      }

	      rs_write_float(IDX_SPD_REF, spd_cmd);
	      HAL_Delay(20);                       // 50Hz로 지령 갱신
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
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
