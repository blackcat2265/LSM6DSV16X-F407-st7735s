#include "main.h"
#include "imu_sflp.h"
#include "imu.h"

void imu_sflp_init(void) {
    // Пишем напрямую в регистр FIFO_SFLP_RAW_CTRL (0x4B)
    // 0x0A включает компенсацию гироскопа и кватернион
    uint8_t data = 0x0A;
    lsm6dsv16x_write_reg(&dev_ctx, 0x4B, &data, 1);

    // Частота SFLP 120 Гц
    lsm6dsv16x_sflp_data_rate_set(&dev_ctx, LSM6DSV16X_SFLP_120Hz);
}
