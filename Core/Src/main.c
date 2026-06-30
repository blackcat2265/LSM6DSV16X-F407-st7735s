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
#include "imu.h"
#include "imu_fifo.h"
#include "imu_sflp.h"
#include "display.h"
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

    // Запуск ШИМ для подсветки (обычно TIM_CHANNEL_1 или 2)
      HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_4);

      // Установка яркости на 100% (если период таймера равен 100 или 1000)
      __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_4, 400);


    HAL_Delay(1);
    ST7735_Init();
    HAL_Delay(1);

    ST7735_FillScreen(ST7735_BLACK);

    imu_init();      // Настройка SPI и базовых параметров (ODR, FS)
    imu_sflp_init(); // Включение кватернионов и GBIAS

    // 1. Настройка маршрутизации прерывания на ножку INT1
  /*    lsm6dsv16x_pin_int_route_t pin_route = {0}; // Используем имя типа без цифры '1'
      // Мы говорим датчику: "Когда FIFO заполнится до Watermark, подай сигнал на INT1"
      pin_route.fifo_th = 1; // Прерывание по достижению Watermark
      lsm6dsv16x_pin_int1_route_set(&dev_ctx, &pin_route);

      // 2. Настройка того, ЧТО записывать в FIFO
      // Включаем запись данных акселерометра в FIFO (чтобы увидеть тег 02)
      lsm6dsv16x_fifo_xl_batch_set(&dev_ctx, LSM6DSV16X_XL_BATCHED_AT_120Hz);

      // --- ДОБАВЬТЕ ЭТИ СТРОКИ ДЛЯ TAG 13 ---
        uint8_t fifo_ctrl4 = 0x08; // Бит 3 отвечает за SFLP batching
        lsm6dsv16x_write_reg(&dev_ctx, 0x0A, &fifo_ctrl4, 1);
        // --------------------------------------

      // 3. Настройка FIFO (Watermark = 32, запуск режима STREAM)
      lsm6dsv16x_fifo_watermark_set(&dev_ctx, 32);
      lsm6dsv16x_fifo_mode_set(&dev_ctx, LSM6DSV16X_STREAM_MODE);
*/
    imu_fifo_init(&dev_ctx);

  /* USER CODE END 2 */
  /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    uint32_t t_scr = 0;
    char d_buf;

    while (1)
    {
      if (fifo_irq) // 1. Сервис ТЕПЕРЬ СТРОГО ВНУТРИ
      {
        fifo_irq = 0;
        imu_fifo_service(&dev_ctx);
      }

      if (HAL_GetTick() - t_scr >= 100) // 2. Отрисовка ТЕПЕРЬ СТРОГО ТУТ
      {
        t_scr = HAL_GetTick();

        // Принудительно включаем алгоритм (чтобы R4B стал 0A)
        uint8_t sflp_on = 0x0A;
        lsm6dsv16x_write_reg(&dev_ctx, 0x4B, &sflp_on, 1);

        display_update(imu_irq_cnt);

        uint8_t r4b = 0, r0a = 0;
        lsm6dsv16x_read_reg(&dev_ctx, 0x4B, &r4b, 1);
        lsm6dsv16x_read_reg(&dev_ctx, 0x0A, &r0a, 1);

        sprintf(d_buf, "R4B:%02X R0A:%02X", r4b, r0a);
        ST7735_WriteString(10, 110, d_buf, Font_7x10, ST7735_YELLOW, ST7735_BLACK);
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
