#include "imu.h"
#include <string.h>
#include "main.h" // Для доступа к hspi2

stmdev_ctx_t dev_ctx;

static int32_t platform_write(void *handle, uint8_t reg, const uint8_t *bufp, uint16_t len) {
    uint8_t tx_buf;
    tx_buf = reg;
    if (len > 1) tx_buf |= 0x40; // IF_INC [5]
    memcpy(&tx_buf[6], bufp, len);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
    HAL_StatusTypeDef status = HAL_SPI_Transmit(handle, tx_buf, len + 1, 100);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
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

    lsm6dsv16x_xl_full_scale_set(&dev_ctx, LSM6DSV16X_2g); [4]
    lsm6dsv16x_xl_data_rate_set(&dev_ctx, LSM6DSV16X_ODR_AT_120Hz); [4]
    lsm6dsv16x_gy_data_rate_set(&dev_ctx, LSM6DSV16X_ODR_AT_120Hz); [1]
}
