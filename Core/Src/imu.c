#include "main.h"
#include "imu.h"
#include "spi.h"
#include <string.h>
#include "main.h" // Для доступа к hspi2
extern SPI_HandleTypeDef hspi2; // Говорим файлу, что SPI объявлен в другом месте (в main.c)

stmdev_ctx_t dev_ctx; // Реальное создание переменной здесь

static int32_t platform_write(void *handle, uint8_t reg, const uint8_t *bufp, uint16_t len)
{
    uint8_t tx_buf[33]; // Создаем массив на 33 байта (индекс 33 в скобках)
    tx_buf[0] = reg; // 1. В самую первую ячейку (индекс 0) кладем только адрес регистра
   // if (len > 1) tx_buf[0] |= 0x40; // IF_INC
   // memcpy(&tx_buf[1], bufp, len);
    memcpy(&tx_buf[1], bufp, len); // 2. Копируем данные из bufp в массив, начиная со ВТОРОЙ ячейки (индекс 1)
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET); // CS в 0
    HAL_StatusTypeDef status = HAL_SPI_Transmit(handle, tx_buf, len + 1, 100); // 3. Передаем всё за один раз: адрес (1 байт) + данные (len байт)
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET); // CS в 1
    return (status == HAL_OK) ? 0 : -1;
}

static int32_t platform_read(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len) {
    uint8_t tx_buf = reg | 0x80;
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
    HAL_SPI_Transmit(handle, &tx_buf, 1, 100);
    HAL_StatusTypeDef status = HAL_SPI_Receive(handle, bufp, len, 100);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
    return (status == HAL_OK) ? 0 : -1;
}

void imu_init(void) {
    dev_ctx.write_reg = platform_write;
    dev_ctx.read_reg = platform_read;
    dev_ctx.handle = &hspi2;

    lsm6dsv16x_xl_full_scale_set(&dev_ctx, LSM6DSV16X_2g);
    lsm6dsv16x_xl_data_rate_set(&dev_ctx, LSM6DSV16X_ODR_AT_120Hz);
    lsm6dsv16x_gy_data_rate_set(&dev_ctx, LSM6DSV16X_ODR_AT_120Hz);
}
