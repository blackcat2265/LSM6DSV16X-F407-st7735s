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
#include "imu_fifo.h"
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
volatile uint8_t fifo_event = 0;
volatile uint8_t fifo_irq = 0;
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
/*
    lsm6dsv16x_reset_set(&dev_ctx, 1); // програмный сброс датчика
    uint8_t rst;
    do
    {
        lsm6dsv16x_reset_get(&dev_ctx, &rst);
    }
    while(rst);
*/
/*Для пакетного чтения по SPI необходимо включить параметр IF_INC*/
    /*Включить IF_INC (автоматическое увеличение регистра)
* Требуется для пакетного чтения: * 0x28 -> 0x29 -> 0x2A ->*/
    uint8_t reg;
    /* IF_INC = 1 */
    lsm6dsv16x_read_reg(&dev_ctx, 0x12, &reg, 1);
    reg |= 0x04;
    lsm6dsv16x_write_reg(&dev_ctx, 0x12, &reg, 1);

    HAL_Delay(1);
    //uint8_t reg = 0;
    uint8_t ctrl1 = 0;
    uint8_t ctrl2 = 0;
    uint8_t status = 0;

    lsm6dsv16x_xl_full_scale_set(&dev_ctx, LSM6DSV16X_2g);
    lsm6dsv16x_xl_data_rate_set(&dev_ctx, LSM6DSV16X_ODR_AT_120Hz);
   // lsm6dsv16x_xl_data_rate_set(&dev_ctx, LSM6DSV16X_ODR_AT_7680Hz);
    lsm6dsv16x_xl_mode_set(&dev_ctx, LSM6DSV16X_XL_HIGH_PERFORMANCE_MD);

    lsm6dsv16x_gy_full_scale_set(&dev_ctx, LSM6DSV16X_250dps);
   // lsm6dsv16x_gy_data_rate_set(&dev_ctx, LSM6DSV16X_ODR_AT_7680Hz);
    lsm6dsv16x_gy_data_rate_set(&dev_ctx, LSM6DSV16X_ODR_AT_120Hz);

    lsm6dsv16x_fifo_watermark_set(&dev_ctx, 32);

    lsm6dsv16x_fifo_xl_batch_set(
        &dev_ctx,
        LSM6DSV16X_XL_BATCHED_AT_120Hz);

    lsm6dsv16x_fifo_gy_batch_set(
        &dev_ctx,
        LSM6DSV16X_GY_BATCHED_AT_120Hz);

    lsm6dsv16x_fifo_mode_set(
        &dev_ctx,
        LSM6DSV16X_STREAM_MODE);

    lsm6dsv16x_pin_int_route_t int1_route;
    memset(&int1_route,0,sizeof(int1_route));

    int1_route.fifo_th = 1;

    lsm6dsv16x_pin_int1_route_set(&dev_ctx,&int1_route);

    uint8_t int1_ctrl = 0;
    lsm6dsv16x_read_reg(&dev_ctx, 0x0D, &int1_ctrl, 1);

    lsm6dsv16x_pin_int_route_t chk;
    memset(&chk, 0, sizeof(chk));
    lsm6dsv16x_pin_int1_route_get(&dev_ctx, &chk);

    HAL_Delay(1);
    lsm6dsv16x_read_reg(&dev_ctx, 0x10, &ctrl1, 1);
    lsm6dsv16x_read_reg(&dev_ctx, 0x11, &ctrl2, 1);
    lsm6dsv16x_read_reg(&dev_ctx, 0x1E, &status, 1);

    // Отладка: читаем WHO_AM_I напрямую
    uint8_t who_am_i = 0;
    lsm6dsv16x_read_reg(&dev_ctx, 0x0F, &who_am_i, 1);

    char debug_buf[64];
    sprintf(debug_buf, "WHO:%02X CS:%d", who_am_i, HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_12));
    HAL_UART_Transmit(&huart1, (uint8_t*)debug_buf, strlen(debug_buf), 100);
    HAL_UART_Transmit(&huart1, (uint8_t*)"\r\n", 2, 100);

    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_4);
    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_4, 500);

    HAL_Delay(1);
    ST7735_Init();
    HAL_Delay(1);

    ST7735_FillScreen(ST7735_RED);
    HAL_Delay(10);
    ST7735_FillScreen(ST7735_GREEN);
    HAL_Delay(10);
    ST7735_FillScreen(ST7735_BLUE);
    HAL_Delay(10);
    ST7735_FillScreen(ST7735_BLACK);

    ST7735_WriteString(10, 0, "STM32F407 OK", Font_7x10, ST7735_WHITE, ST7735_BLACK);

    uint8_t txbuf[2];
    uint8_t rxbuf[2];

    txbuf[0] = 0x8F;
    txbuf[1] = 0x00;

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
    HAL_SPI_TransmitReceive(&hspi2, txbuf, rxbuf, 2, 100);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);

    char buf[32];

    sprintf(buf, "C1:%02X C2:%02X", ctrl1, ctrl2);
    ST7735_WriteString(10, 10, buf, Font_7x10, ST7735_GREEN, ST7735_BLACK);

  /* USER CODE END 2 */

  /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    uint32_t t_scr = 0;
    char lcd_buf; // Используем отдельный буфер для экрана

    while (1)
    {
      // 1. Обработка прерывания FIFO
      if (fifo_irq)
      {
        fifo_irq = 0;
        imu_fifo_service(&dev_ctx);
      }

      // 2. Обновление экрана раз в 100 мс
      if (HAL_GetTick() - t_scr >= 100)
      {
        t_scr = HAL_GetTick();

        // Вывод заполненности FIFO (из модуля imu_fifo)
        sprintf(lcd_buf, "FIFO: %03d  ", imu_data.fifo_level);
        ST7735_WriteString(10, 50, lcd_buf, Font_7x10, ST7735_WHITE, ST7735_BLACK);

        // Вывод последнего тега (02-XL, 01-GY, 13-SFLP)
        sprintf(lcd_buf, "TAG:  %02X   ", imu_data.last_tag);
        ST7735_WriteString(10, 60, lcd_buf, Font_7x10, ST7735_YELLOW, ST7735_BLACK);

        // Счётчик прерываний для проверки связи
        sprintf(lcd_buf, "IRQ: %lu   ", imu_irq_cnt);
        ST7735_WriteString(10, 30, lcd_buf, Font_7x10, ST7735_MAGENTA, ST7735_BLACK);

        // Здесь будут Pitch/Roll, как только включим SFLP
      }
      /* USER CODE END WHILE */
      }

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
    if (GPIO_Pin == GPIO_PIN_13)  // PE13 = INT1 от LSM6DSV16X
    {
 //       accel_ready = 1;          // Флаг: новые данные готовы
    	fifo_irq = 1;
    	imu_irq_cnt++;            // Счётчик прерываний
    }
    if (GPIO_Pin == GPIO_PIN_14)      // INT2 (пока не используется)
    {
 //       qvar_irq_cnt++;
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
