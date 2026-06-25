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

    tx_buf[0] = reg;
    if (len > 1) {
        tx_buf[0] |= 0x40;
    }
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

    tx_buf[0] = reg | 0x80;
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

    HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
    HAL_Delay(1);

    dev_ctx.write_reg = platform_write;
    dev_ctx.read_reg = platform_read;
    dev_ctx.handle = &hspi2;

    uint8_t reg;
    lsm6dsv16x_read_reg(&dev_ctx, 0x12, &reg, 1);
    reg |= 0x04;
    lsm6dsv16x_write_reg(&dev_ctx, 0x12, &reg, 1);

    HAL_Delay(1);

    lsm6dsv16x_xl_full_scale_set(&dev_ctx, LSM6DSV16X_2g);
    lsm6dsv16x_xl_data_rate_set(&dev_ctx, LSM6DSV16X_ODR_AT_120Hz);
    lsm6dsv16x_xl_mode_set(&dev_ctx, LSM6DSV16X_XL_HIGH_PERFORMANCE_MD);

    lsm6dsv16x_gy_full_scale_set(&dev_ctx, LSM6DSV16X_250dps);
    lsm6dsv16x_gy_data_rate_set(&dev_ctx, LSM6DSV16X_ODR_AT_120Hz);

    uint8_t who_am_i = 0;
    lsm6dsv16x_read_reg(&dev_ctx, 0x0F, &who_am_i, 1);

    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_4);
    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_4, 500);
    HAL_Delay(1);
    ST7735_Init();
    HAL_Delay(1);
    ST7735_FillScreen(ST7735_BLACK);

    char buf[32];
    sprintf(buf, "WHO:%02X", who_am_i);
    ST7735_WriteString(10, 0, buf, Font_7x10, ST7735_CYAN, ST7735_BLACK);

    if (who_am_i != 0x70) {
        ST7735_WriteString(10, 10, "BAD ID!", Font_7x10, ST7735_RED, ST7735_BLACK);
        while(1) { HAL_Delay(100); }
    }

    // FIFO настройка (watermark = 2 пакета = 14 байт)
    uint8_t wtm_low = 0x02;  // УМЕНЬШИЛИ watermark!
    uint8_t wtm_high = 0x00;
    lsm6dsv16x_write_reg(&dev_ctx, 0x07, &wtm_low, 1);
    lsm6dsv16x_write_reg(&dev_ctx, 0x08, &wtm_high, 1);

    uint8_t fifo_ctrl3 = 0x66;
    lsm6dsv16x_write_reg(&dev_ctx, 0x09, &fifo_ctrl3, 1);

    uint8_t fifo_ctrl4 = 0x06;
    lsm6dsv16x_write_reg(&dev_ctx, 0x0A, &fifo_ctrl4, 1);

    uint8_t int1_ctrl = 0x08;
    lsm6dsv16x_write_reg(&dev_ctx, 0x0D, &int1_ctrl, 1);

    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

    // БЕЗ HAL_Delay! Сразу переходим в while(1)

  /* USER CODE END 2 */

  /* Infinite loop */
    /* USER CODE BEGIN WHILE */
      uint32_t t_scr = 0;

      static int16_t last_x = 0, last_y = 0, last_z = 0;
      static int16_t last_gx = 0, last_gy = 0, last_gz = 0;
      static float temp_c = 25.0f;
      static bool new_accel = false, new_gyro = false;

      static float filter_roll = 0.0f;
      static float filter_pitch = 0.0f;

      static uint32_t cnt_gyro = 0;
      static uint32_t cnt_accel = 0;
      static uint32_t cnt_temp = 0;

      while (1)
      {
          // ЧИТАЕМ FIFO ВСЕГДА (не только по прерыванию!)
          uint8_t fifo_status[2];
          lsm6dsv16x_read_reg(&dev_ctx, 0x1B, fifo_status, 2);
          uint16_t fifo_level = ((fifo_status[1] & 0x0F) << 8) | fifo_status[0];
          uint8_t fifo_full = (fifo_status[1] >> 7) & 0x01;

          // Защита от переполнения
          if (fifo_full) {
              // Сбрасываем FIFO
              uint8_t fifo_reset = 0x00;
              lsm6dsv16x_write_reg(&dev_ctx, 0x0A, &fifo_reset, 1);
              HAL_Delay(1);
              uint8_t fifo_ctrl4 = 0x06;
              lsm6dsv16x_write_reg(&dev_ctx, 0x0A, &fifo_ctrl4, 1);
          }

          if (fifo_level >= 7)
          {
              if (fifo_level > 200) fifo_level = 200;

              uint8_t fifo_buf[210];
              lsm6dsv16x_read_reg(&dev_ctx, 0x78, fifo_buf, fifo_level);

              for (uint16_t i = 0; i + 6 < fifo_level; i += 7)
              {
                  uint8_t tag = fifo_buf[i] >> 3;

                  if (tag == 0x02) // Акселерометр
                  {
                      last_x = (int16_t)((fifo_buf[i+2] << 8) | fifo_buf[i+1]);
                      last_y = (int16_t)((fifo_buf[i+4] << 8) | fifo_buf[i+3]);
                      last_z = (int16_t)((fifo_buf[i+6] << 8) | fifo_buf[i+5]);
                      new_accel = true;
                      cnt_accel++;
                  }
                  else if (tag == 0x01) // Гироскоп
                  {
                      last_gx = (int16_t)((fifo_buf[i+2] << 8) | fifo_buf[i+1]);
                      last_gy = (int16_t)((fifo_buf[i+4] << 8) | fifo_buf[i+3]);
                      last_gz = (int16_t)((fifo_buf[i+6] << 8) | fifo_buf[i+5]);
                      new_gyro = true;
                      cnt_gyro++;
                  }
                  else if (tag == 0x03) // Температура
                  {
                      int16_t t_raw = (int16_t)((fifo_buf[i+2] << 8) | fifo_buf[i+1]);
                      temp_c = (t_raw / 256.0f) + 25.0f;
                      cnt_temp++;
                  }
              }
              drdy_cnt++;
          }

          if (new_accel && new_gyro)
          {
              new_accel = false;
              new_gyro = false;

              float x_g = (float)last_x / 16384.0f;
              float y_g = (float)last_y / 16384.0f;
              float z_g = (float)last_z / 16384.0f;

              float gyro_x_deg = last_gx * 0.00875f;
              float gyro_y_deg = last_gy * 0.00875f;

              float accel_roll  = atan2f(y_g, z_g) * 57.2958f;
              float accel_pitch = atan2f(-x_g, z_g) * 57.2958f;

              float dt = 0.01f;
              filter_roll  = 0.98f * (filter_roll + gyro_x_deg * dt) + 0.02f * accel_roll;
              filter_pitch = 0.98f * (filter_pitch + gyro_y_deg * dt) + 0.02f * accel_pitch;
          }

          if(HAL_GetTick() - t_scr >= 100)
          {
              t_scr = HAL_GetTick();

              sprintf(buf, "G:%lu A:%lu T:%lu", cnt_gyro, cnt_accel, cnt_temp);
              ST7735_WriteString(10, 70, buf, Font_7x10, ST7735_CYAN, ST7735_BLACK);

              sprintf(buf, "Roll :%7.3f", filter_roll);
              ST7735_WriteString(10, 80, buf, Font_7x10, ST7735_GREEN, ST7735_BLACK);

              sprintf(buf, "Pitch:%7.3f", filter_pitch);
              ST7735_WriteString(10, 90, buf, Font_7x10, ST7735_GREEN, ST7735_BLACK);

              sprintf(buf, "Temp:%5.1f", temp_c);
              ST7735_WriteString(10, 100, buf, Font_7x10, ST7735_YELLOW, ST7735_BLACK);
          }

          HAL_Delay(1);
      }
    /* USER CODE END WHILE */

  /* USER CODE BEGIN 3 */
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
