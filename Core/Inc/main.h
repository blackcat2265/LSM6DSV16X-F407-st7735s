/* Заголовок НАЧАЛА ПОЛЬЗОВАТЕЛЬСКОГО КОДА */
/**
 *******************************************************
 * @файл: main.h
 * @brief : Заголок файла main.c.
 * Этот файл содержит общие определения приложения.
 *******************************************************
 * @внимание
  *
 * Авторские права (c) 2026 STMicroelectronics.
 * Все права защищены.
  *
 * ЭтѾ проѳЀаммное обспѵчение лицензируется на условиях, кторые можно найти в файле ЛИЦЕНЗИЯ
 * в корневом каталоге этого программного компонента.
 * Если в комплект посстЂавки данного программного обспѵчения не входит файл ЛИЦЕНЗИЯ, он предѾссттавляѵтся «КЕК ЕСТЬ».
  *
 *******************************************************
 */
/* КОНЕЧНЫЙ заголовок ПОЛЬЗОВАТЕЛЬСКОГО КОДА */

/* Определите, чтобы предотвратить рекурсивное включение -------------------------------*/
#ifndef __ОСНОВНОЙ_H
#define __ОСНОВНОЙ_H

#ifdef __cplusplus
внешний "С" {
#endif

/* Включает ------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Частное включает в себя ---------------------------------------------------*/
/* НАЧАЛО КОДА ПОЛЬЗОВАТЕЛЯ Включает */

/* КОНЕЦ КОДА ПОЛЬЗОВАТЕЛЯ Включает */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define INT1_Pin GPIO_PIN_13
#define INT1_GPIO_Port GPIOE
#define INT1_EXTI_IRQn EXTI15_10_IRQn
#define INT2_Pin GPIO_PIN_14
#define INT2_GPIO_Port GPIOE
#define INT2_EXTI_IRQn EXTI15_10_IRQn
#define IMU_CS_Pin GPIO_PIN_12
#define IMU_CS_GPIO_Port GPIOB
#define SPI3_CS_Pin GPIO_PIN_6
#define SPI3_CS_GPIO_Port GPIOB
#define SPI3_DC_Pin GPIO_PIN_7
#define SPI3_DC_GPIO_Port GPIOB
#define SPI3_RES_Pin GPIO_PIN_8
#define SPI3_RES_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
