#include "main.h"
#include "imu_sflp.h"
#include "imu.h"

void imu_sflp_init(void)
{
	// 1. Включаем сам алгоритм SFLP (GBIAS + Кватернион)
	// Пишем 0x0A в регистр 0x4B
	uint8_t sflp_en = 0x0A;
//	uint8_t sflp_ctrl = 0x0A;
	lsm6dsv16x_write_reg(&dev_ctx, 0x4B, &sflp_en, 1); // Включить расчет
//	lsm6dsv16x_write_reg(&dev_ctx, 0x4B, &sflp_ctrl, 1);

	uint8_t sflp_fifo = 0x08;
	lsm6dsv16x_write_reg(&dev_ctx, 0x0A, &sflp_fifo, 1); // Включить запись в FIFO

}
/*
    // Частота SFLP 120 Гц
    lsm6dsv16x_sflp_data_rate_set(&dev_ctx, LSM6DSV16X_SFLP_120Hz);

    // 3. САМОЕ ВАЖНОЕ ДЛЯ TAG 13: Включаем запись SFLP в FIFO
        // Регистр 0x0A (FIFO_CTRL4), бит 3 отвечает за батчинг SFLP
        uint8_t fifo_batch = 0x08;
        lsm6dsv16x_write_reg(&dev_ctx, 0x0A, &fifo_batch, 1);
}
*/
