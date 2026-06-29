#include "imu_sflp.h"
#include "imu.h"

void imu_sflp_init(void) {
    // Включение SFLP напрямую через регистр, чтобы избежать ошибок структуры [7, 8]
    uint8_t sflp_ctrl = 0x0A; // GBIAS + QUAT
    lsm6dsv16x_write_reg(&dev_ctx, 0x4B, &sflp_ctrl, 1);
    lsm6dsv16x_sflp_data_rate_set(&dev_ctx, LSM6DSV16X_SFLP_120Hz); [8]

    // Настройка FIFO для SFLP
    lsm6dsv16x_fifo_sflp_batch_set(&dev_ctx, (lsm6dsv16x_fifo_sflp_raw_t){.sflp_quat = 1});
}
