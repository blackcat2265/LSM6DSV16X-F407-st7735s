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
volatile uint32_t qvar_irq_cnt = 0;
volatile uint32_t drdy_cnt = 0;
volatile uint8_t accel_ready = 0;
volatile uint32_t accel_ready_cnt = 0;
volatile uint8_t fifo_event = 0;
volatile uint8_t imu_data_ready = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);

/* USER CODE BEGIN PFP */

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_13) // Это ваш пин PE13 (INT1)
    {
        imu_irq_cnt++;       // Ваш старый счетчик прерываний
        fifo_event = 1;      // Ваш старый флаг

        imu_data_ready = 1;  // НОВЫЙ ФЛАГ для запуска обработки SFLP в while(1)
    }
}

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// Глобальные структуры данных
stmdev_ctx_t dev_ctx;

typedef struct {
    float roll;
    float pitch;
    float yaw;
} EulerAngles_t;

static void platform_delay(uint32_t ms)
{
    HAL_Delay(ms);
}

static int32_t platform_write(void *handle, uint8_t reg, const uint8_t *bufp, uint16_t len)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
    // Для LSM6DSV16X бит 0x40 не нужен, автоинкремент работает аппаратно
    HAL_SPI_Transmit(handle, &reg, 1, 1000);
    HAL_SPI_Transmit(handle, (uint8_t*)bufp, len, 1000);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
    return 0;
}

static int32_t platform_read(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len)
{
    uint8_t reg_read = reg | 0x80; // Бит чтения SPI

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
    HAL_SPI_Transmit(handle, &reg_read, 1, 1000);
    // Прямой прием в буфер драйвера без ограничений в 32 байта
    HAL_SPI_Receive(handle, bufp, len, 1000);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);

    return 0;
}

// Функция декодирования кватерниона SFLP в углы
EulerAngles_t LSM6DSV16X_Decode_SFLP(float sflp_x, float sflp_y, float sflp_z)
{
    EulerAngles_t angles;

    // Векторная часть кватерниона
    float qx = sflp_x;
    float qy = sflp_y;
    float qz = sflp_z;

    // Восстанавливаем скалярную часть W
    float t = 1.0f - (qx * qx + qy * qy + qz * qz);
    float qw = (t > 0.0f) ? sqrtf(t) : 0.0f;

    // Расчет Roll (вокруг оси X крен)
    float sinr_cosp = 2.0f * (qw * qx + qy * qz);
    float cosr_cosp = 1.0f - 2.0f * (qx * qx + qy * qy);
    angles.roll = atan2f(sinr_cosp, cosr_cosp) * (180.0f / M_PI);

    // Расчет Pitch (вокруг оси Y тангаж)
    float sinp = 2.0f * (qw * qy - qz * qx);
    if (fabsf(sinp) >= 1.0f) {
        angles.pitch = copysignf(M_PI / 2.0f, sinp) * (180.0f / M_PI);
    } else {
        angles.pitch = asinf(sinp) * (180.0f / M_PI);
    }

    // Расчет Yaw (вокруг оси Z рысканье)
    float siny_cosp = 2.0f * (qw * qz + qx * qy);
    float cosy_cosp = 1.0f - 2.0f * (qy * qy + qz * qz);
    angles.yaw = atan2f(siny_cosp, cosy_cosp) * (180.0f / M_PI);

    return angles;
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
    HAL_Delay(10);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
    HAL_Delay(10);

    // Инициализация контекста драйвера ST
    dev_ctx.write_reg = platform_write;
    dev_ctx.read_reg = platform_read;
    dev_ctx.mdelay = platform_delay;
    dev_ctx.handle = &hspi2;

    // Проверяем связь с чипом через WHO_AM_I
    uint8_t whoamI = 0;
    lsm6dsv16x_device_id_get(&dev_ctx, &whoamI);

    // Инициализация дисплея и ШИМ подсветки
    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_4);
    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_4, 500);
    ST7735_Init();
    ST7735_FillScreen(ST7735_BLACK);

    if (whoamI == LSM6DSV16X_ID) {
        ST7735_WriteString(5, 0, "LSM6DSV16X OK", Font_7x10, ST7735_GREEN, ST7735_BLACK);

        // 1. Программный сброс (SW_RESET)
        uint8_t rst_reg = 0x01;
        lsm6dsv16x_write_reg(&dev_ctx, LSM6DSV16X_CTRL3, &rst_reg, 1);
        do {
            lsm6dsv16x_read_reg(&dev_ctx, LSM6DSV16X_CTRL3, &rst_reg, 1);
        } while ((rst_reg & 0x01) != 0);

        // 2. Включаем Акселерометр и Гироскоп строго на 30 Гц (SFLP жестко завязан на 15 или 30 Гц)
        lsm6dsv16x_xl_full_scale_set(&dev_ctx, LSM6DSV16X_2g);
        lsm6dsv16x_xl_data_rate_set(&dev_ctx, LSM6DSV16X_ODR_AT_30Hz);
        lsm6dsv16x_xl_mode_set(&dev_ctx, LSM6DSV16X_XL_HIGH_PERFORMANCE_MD);

        lsm6dsv16x_gy_full_scale_set(&dev_ctx, LSM6DSV16X_2000dps);
        lsm6dsv16x_gy_data_rate_set(&dev_ctx, LSM6DSV16X_ODR_AT_30Hz);

        // 3. АКТИВАЦИЯ SFLP (Sensor Fusion Low-Power)
        // Для вашей версии драйвера включаем ODR для SFLP и активируем игровой вектор вращения
        lsm6dsv16x_sflp_data_rate_set(&dev_ctx, LSM6DSV16X_SFLP_30Hz);

        // Прямая команда на включение Game Rotation Vector (слияние Акселерометр + Гироскоп)
        lsm6dsv16x_sflp_game_rotation_set(&dev_ctx, 1);

        // 4. Очистка и конфигурация буфера FIFO
        lsm6dsv16x_fifo_mode_set(&dev_ctx, LSM6DSV16X_BYPASS_MODE);
        HAL_Delay(10);

        // Настройка записи SFLP Game Vector в буфер FIFO через структуру конфигурации
        lsm6dsv16x_fifo_sflp_raw_t fifo_sflp_cfg;
        memset(&fifo_sflp_cfg, 0, sizeof(fifo_sflp_cfg));
        fifo_sflp_cfg.game_rotation = 1; // Записывать Game Vector в FIFO
        lsm6dsv16x_fifo_sflp_batch_set(&dev_ctx, fifo_sflp_cfg);

        // Переводим FIFO в режим циклического накопления потока (Stream Mode)
        lsm6dsv16x_fifo_mode_set(&dev_ctx, LSM6DSV16X_STREAM_MODE);

        // 5. Настройка генерации прерывания по FIFO на пин INT1 (PE13)
        lsm6dsv16x_pin_int_route_t int1_route;
        memset(&int1_route, 0, sizeof(int1_route));
        int1_route.fifo_th = 1; // Сигнал на ножку, если в буфере есть данные
        lsm6dsv16x_pin_int1_route_set(&dev_ctx, &int1_route);

    } else {
        char err_buf[32];
        sprintf(err_buf, "IMU ERR ID:%02X", whoamI);
        ST7735_WriteString(5, 0, err_buf, Font_7x10, ST7735_RED, ST7735_BLACK);
    }
    // Принудительно включаем обработку прерываний для пинов 10-15 порта EXTI
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  /* USER CODE END 2 */


  /* Infinite loop */

    /* USER CODE BEGIN WHILE */
    while (1)
    {
        // Обработка запускается строго по аппаратному прерыванию от PE13
        if (imu_data_ready)
        {
            imu_data_ready = 0; // Сбрасываем флаг

            uint8_t fifo_status[2] = {0};
            uint16_t num_samples = 0;

            // Проверяем сколько данных накопилось в FIFO
            if (lsm6dsv16x_read_reg(&dev_ctx, LSM6DSV16X_FIFO_STATUS1, fifo_status, 2) == 0)
            {
                num_samples = fifo_status[0] | ((fifo_status[1] & 0x03) << 8);
            }

            // Вычитываем все пакеты, пока FIFO не опустеет
            while (num_samples > 0)
            {
                lsm6dsv16x_fifo_out_raw_t f_data;

                if (lsm6dsv16x_fifo_out_raw_get(&dev_ctx, &f_data) != 0) {
                    break;
                }

                // 0x13 — это тег SFLP Game Rotation Vector
                if (f_data.tag == 0x13)
                {
                    // 1. Интерпретируем сырые байты из FIFO пакета как массив 16-битных целых чисел
                    int16_t *raw_axis = (int16_t *)f_data.data;

                    // 2. В вашей библиотеке ST масштабирует кватернионы из FIFO через деление на 24576.0f
                    // Извлекаем правильные нормализованные float значения осей X, Y, Z
                    float sflp_x = (float)raw_axis[0] / 24576.0f;
                    float sflp_y = (float)raw_axis[1] / 24576.0f;
                    float sflp_z = (float)raw_axis[2] / 24576.0f;

                    // 3. Конвертируем нормализованный кватернион в углы Эйлера
                    EulerAngles_t orientation = LSM6DSV16X_Decode_SFLP(sflp_x, sflp_y, sflp_z);

                    // 4. Вывод углов на экран ST7735
                    char scr_buf[32];

                    snprintf(scr_buf, sizeof(scr_buf), "Roll:  %6.1f", orientation.roll);
                    ST7735_WriteString(5, 20, scr_buf, Font_7x10, ST7735_WHITE, ST7735_BLACK);

                    snprintf(scr_buf, sizeof(scr_buf), "Pitch: %6.1f", orientation.pitch);
                    ST7735_WriteString(5, 35, scr_buf, Font_7x10, ST7735_WHITE, ST7735_BLACK);

                    snprintf(scr_buf, sizeof(scr_buf), "Yaw:   %6.1f", orientation.yaw);
                    ST7735_WriteString(5, 50, scr_buf, Font_7x10, ST7735_WHITE, ST7735_BLACK);
                }
                num_samples--;
            }
        }

        // Небольшая задержка, чтобы микроконтроллер успевал выполнять другие системные задачи,
        // когда прерывания от датчика неактивны
        HAL_Delay(1);
      /* USER CODE END WHILE */


    }
      /* USER CODE BEGIN 3 */
    }
    /* USER CODE END 3 */

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
