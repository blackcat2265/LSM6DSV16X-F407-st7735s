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
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "lsm6dsv16x_reg.h"
#include <stdio.h>
#include "st7735.h"
#include "fonts.h"
#include <string.h>
#include <math.h>
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

/* USER CODE BEGIN PV */
stmdev_ctx_t dev_ctx;
volatile uint32_t imu_irq_cnt = 0;
volatile uint32_t drdy_cnt = 0;
volatile uint8_t accel_ready = 0;
volatile uint32_t accel_ready_cnt = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static int32_t platform_write(void *handle, uint8_t reg, const uint8_t *bufp, uint16_t len)
{
    uint8_t tx_buf[33];

    tx_buf[0] = reg; // ← УБРАЛИ |= 0x40. LSM6DSV16X использует только бит в CTRL3 (0x12)

    memcpy(&tx_buf[1], bufp, len);

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
    HAL_StatusTypeDef status = HAL_SPI_Transmit(handle, tx_buf, len + 1, 100);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);

    return (status == HAL_OK) ? 0 : -1;
}

static int32_t platform_read(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len)
{
    uint8_t tx_buf[33];
    uint8_t rx_buf[33];

    tx_buf[0] = reg | 0x80; // Только бит чтения, без 0x40!
    memset(&tx_buf[1], 0x00, len);

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
    HAL_StatusTypeDef status = HAL_SPI_TransmitReceive(handle, tx_buf, rx_buf, len + 1, 100);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);

    if (status != HAL_OK) {
        return -1;
    }

    memcpy(bufp, &rx_buf[1], len);
    return 0;
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
  MX_USART1_UART_Init();
  MX_SPI3_Init();
  MX_TIM4_Init();
  MX_SPI2_Init();

  /* USER CODE BEGIN 2 */

  // === 1. Включаем прерывания EXTI ===
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  HAL_Delay(1);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
  HAL_Delay(1);

  // === 2. Инициализация контекста датчика ===
  dev_ctx.write_reg = platform_write;
  dev_ctx.read_reg = platform_read;
  dev_ctx.handle = &hspi2;

  // === 3. Проверка WHO_AM_I ===
  uint8_t who_am_i = 0;
  lsm6dsv16x_read_reg(&dev_ctx, 0x0F, &who_am_i, 1);
  if (who_am_i != 0x70) {
      // Ошибка: датчик не отвечает
      while(1) { HAL_Delay(100); }
  }

  // === 4. Включаем автоинкремент адреса (IF_INC) ===
  uint8_t reg;
  lsm6dsv16x_read_reg(&dev_ctx, 0x12, &reg, 1);
  reg |= 0x04;  // IF_INC = 1
  lsm6dsv16x_write_reg(&dev_ctx, 0x12, &reg, 1);

  HAL_Delay(1);

  // === 5. Настройка диапазонов (ПЕРЕД FIFO!) ===
  lsm6dsv16x_xl_full_scale_set(&dev_ctx, LSM6DSV16X_2g);
  lsm6dsv16x_gy_full_scale_set(&dev_ctx, LSM6DSV16X_250dps);

  // === 6. НАСТРОЙКА FIFO (СТРОГО ПО АЛГОРИТМУ ИЗ lsm6dsv16x_fifo_irq.c) ===

  // 6.1 Watermark = 10 пакетов (70 байт)
  uint8_t wtm_low = 0x0A;   // FIFO_CTRL1 (0x07)
  uint8_t wtm_high = 0x00;  // FIFO_CTRL2 (0x08)
  lsm6dsv16x_write_reg(&dev_ctx, 0x07, &wtm_low, 1);
  lsm6dsv16x_write_reg(&dev_ctx, 0x08, &wtm_high, 1);

  // 6.2 BDR_XL = 6 (120 Hz), BDR_GY = 6 (120 Hz)
  uint8_t fifo_ctrl3 = 0x66;  // FIFO_CTRL3 (0x09)
  lsm6dsv16x_write_reg(&dev_ctx, 0x09, &fifo_ctrl3, 1);

  // 6.3 FIFO_MODE = 6 (Continuous Mode)
  uint8_t fifo_ctrl4 = 0x06;  // FIFO_CTRL4 (0x0A)
  lsm6dsv16x_write_reg(&dev_ctx, 0x0A, &fifo_ctrl4, 1);

  // 6.4 Маршрутизация прерывания: int1_fifo_th = 1
  uint8_t int1_ctrl = 0x08;  // INT1_CTRL (0x0D), бит 3
  lsm6dsv16x_write_reg(&dev_ctx, 0x0D, &int1_ctrl, 1);

  // Включаем алгоритм SFLP (Sensor Fusion)
  uint8_t sflp_ctrl = 0x04; // Включаем SFLP Game Rotation Vector
  lsm6dsv16x_write_reg(&dev_ctx, 0x5E, &sflp_ctrl, 1); // EMB_FUNC_SRC_BDR

  // Направляем SFLP данные в FIFO (Batching Data Rate)
  uint8_t fifo_sflp_ctrl = 0x06; // Частота та же, что и у датчиков (120 Hz)
  lsm6dsv16x_write_reg(&dev_ctx, 0x0C, &fifo_sflp_ctrl, 1); // FIFO_CTRL6

  // === 7. Включение ODR (ТОЛЬКО ПОСЛЕ настройки FIFO!) ===
  lsm6dsv16x_xl_data_rate_set(&dev_ctx, LSM6DSV16X_ODR_AT_120Hz);
  lsm6dsv16x_xl_mode_set(&dev_ctx, LSM6DSV16X_XL_HIGH_PERFORMANCE_MD);
  lsm6dsv16x_gy_data_rate_set(&dev_ctx, LSM6DSV16X_ODR_AT_120Hz);

  HAL_Delay(10);

  // ========================================================
  // === ЖЕЛЕЗОБЕТОННАЯ ИНИЦИАЛИЗАЦИЯ SFLP (SMART PAGE 1) ===
  // ========================================================

  // 1. Сначала жестко переводим гироскоп и акселерометр в High-Performance режим
  // Без этого встроенный ИИ-сопроцессор просто обесточен!
  uint8_t ctrl1_val = 0x60; // Акселерометр: 120Hz, High-Performance
  lsm6dsv16x_write_reg(&dev_ctx, 0x10, &ctrl1_val, 1);
  uint8_t ctrl2_val = 0x60; // Гироскоп: 120Hz, High-Performance
  lsm6dsv16x_write_reg(&dev_ctx, 0x11, &ctrl2_val, 1);

  // 2. Открываем доступ к скрытой странице встроенных функций (Page 1)
  uint8_t page_access = 0x80;
  lsm6dsv16x_write_reg(&dev_ctx, 0x01, &page_access, 1); // FUNC_CFG_ACCESS

  // 3. Активируем Sensor Fusion Low Power (Регистр EMB_FUNC_EN_A = 0x44)
  uint8_t emb_func_a = 0x04; // sflp_game_en = 1
  lsm6dsv16x_write_reg(&dev_ctx, 0x44, &emb_func_a, 1);

  // 4. Задаем частоту работы самого алгоритма SFLP (Регистр SFLP_ODR = 0x5E)
  uint8_t sflp_odr_reg = 0x04; // 120Hz (совпадает с акселерометром)
  lsm6dsv16x_write_reg(&dev_ctx, 0x5E, &sflp_odr_reg, 1);

  // 5. Запускаем сброс и инициализацию ядра SFLP (Регистр EMB_FUNC_INIT_A = 0x66)
  uint8_t emb_init_a = 0x04; // sflp_game_init = 1
  lsm6dsv16x_write_reg(&dev_ctx, 0x66, &emb_init_a, 1);

  // 6. Закрываем доступ к скрытой странице, возвращаемся в основную (ОБЯЗАТЕЛЬНО!)
  page_access = 0x00;
  lsm6dsv16x_write_reg(&dev_ctx, 0x01, &page_access, 1);

  // 7. Настраиваем буфер FIFO отправлять кватернионы наружу (Регистр FIFO_CTRL6 = 0x0C)
  uint8_t fifo_ctrl6 = 0x04; // sflp_game_fifo_en = 1
  lsm6dsv16x_write_reg(&dev_ctx, 0x0C, &fifo_ctrl6, 1);

  HAL_Delay(100); // Даем время ИИ-ядру откалиброваться и рассчитать первые углы



  // === 8. Диагностика: читаем регистры обратно ===
  uint8_t diag[5];
  lsm6dsv16x_read_reg(&dev_ctx, 0x07, &diag[0], 1);  // FIFO_CTRL1
  lsm6dsv16x_read_reg(&dev_ctx, 0x09, &diag[1], 1);  // FIFO_CTRL3
  lsm6dsv16x_read_reg(&dev_ctx, 0x0A, &diag[2], 1);  // FIFO_CTRL4
  lsm6dsv16x_read_reg(&dev_ctx, 0x0D, &diag[3], 1);  // INT1_CTRL

  // === 9. Инициализация дисплея ===
  HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_4);
  __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_4, 500);

  HAL_Delay(1);
  ST7735_Init();
  HAL_Delay(1);

  ST7735_FillScreen(ST7735_BLACK);
  ST7735_WriteString(10, 0, "FIFO Test", Font_7x10, ST7735_GREEN, ST7735_BLACK);

  char buf[32];
  sprintf(buf, "F3:%02X F4:%02X", diag[1], diag[2]);
  ST7735_WriteString(10, 20, buf, Font_7x10, ST7735_CYAN, ST7735_BLACK);

  sprintf(buf, "INT:%02X WTM:%02X", diag[3], diag[0]);
  ST7735_WriteString(10, 30, buf, Font_7x10, ST7735_YELLOW, ST7735_BLACK);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  uint32_t t_scr = 0;

  int16_t last_x = 0, last_y = 0, last_z = 0;
  int16_t last_gx = 0, last_gy = 0, last_gz = 0;

  // Переменные для кватернионов SFLP
  int16_t sflp_q[3] = {0};
  float pitch = 0.0f;

  while (1)
  {
      // 1. Быстро вычитываем FIFO пакетами
      uint8_t fifo_status[2];
      lsm6dsv16x_read_reg(&dev_ctx, 0x1B, fifo_status, 2);

      // diff — это КОЛИЧЕСТВО 7-БАЙТОВЫХ ПАКЕТОВ в буфере
      uint16_t diff = ((fifo_status[1] & 0x0F) << 8) | fifo_status[0];

      // Защита от слишком долгого цикла
      if (diff > 30) diff = 30;

      while (diff > 0)
      {
          uint8_t packet[7];
          // Читаем КАЖДЫЙ пакет ОТДЕЛЬНОЙ транзакцией SPI из регистра 0x78
          if (lsm6dsv16x_read_reg(&dev_ctx, 0x78, packet, 7) != 0) {
              break;
          }

          uint8_t tag = packet[0] >> 3; // Старшие 5 бит — это TAG

          if (tag == 0x02) // TAG акселерометра
          {
              last_x = (int16_t)((packet[2] << 8) | packet[1]);
              last_y = (int16_t)((packet[4] << 8) | packet[3]);
              last_z = (int16_t)((packet[6] << 8) | packet[5]);
          }
          else if (tag == 0x01) // TAG гироскопа
          {
              last_gx = (int16_t)((packet[2] << 8) | packet[1]);
              last_gy = (int16_t)((packet[4] << 8) | packet[3]);
              last_gz = (int16_t)((packet[6] << 8) | packet[5]);
          }
          else if (tag == 0x13) // TAG для SFLP (Game Rotation Vector)
          {
              sflp_q[0] = (int16_t)((packet[2] << 8) | packet[1]);
              sflp_q[1] = (int16_t)((packet[4] << 8) | packet[3]);
              sflp_q[2] = (int16_t)((packet[6] << 8) | packet[5]);

              // Перевод кватернионов в float (-1.0 до 1.0)
              float x = (float)sflp_q[0] / 32768.0f;
              float y = (float)sflp_q[1] / 32768.0f;
              float z = (float)sflp_q[2] / 32768.0f;

              // Восстанавливаем скалярную часть q0
              float q0_sq = 1.0f - (x*x + y*y + z*z);
              float q0 = (q0_sq > 0.0f) ? sqrtf(q0_sq) : 0.0f;

              // Математический расчет угла Pitch (Наклон)
              float sin_pitch = 2.0f * (q0 * y - z * x);

              if (sin_pitch > 1.0f)  sin_pitch = 1.0f;
              if (sin_pitch < -1.0f) sin_pitch = -1.0f;

              pitch = asinf(sin_pitch) * (180.0f / 3.14159265f);
          }

          diff--;
      }

      // Сбрасываем флаг прерывания EXTI
      if (accel_ready) {
          accel_ready = 0;
      }

      // 2. Обновляем экран ST7735S строго 10 раз в секунду
      if (HAL_GetTick() - t_scr >= 100)
      {
          t_scr = HAL_GetTick();

          char display_buf[32];
          // Выводим угол с точностью до 0.01 градуса
          sprintf(display_buf, "Angle: %0.2f deg  ", pitch);

          ST7735_WriteString(10, 60, display_buf, Font_7x10, ST7735_WHITE, ST7735_BLACK);
      }
  }
  /* USER CODE END WHILE */


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
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    // 1. Считаем ВСЕ прерывания EXTI, чтобы проверить, доходит ли сигнал до CPU вообще
    imu_irq_cnt++;

    // 2. Если это наш пин, ставим флаг
    if (GPIO_Pin == INIT1_Pin)
    {
        accel_ready = 1;
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
