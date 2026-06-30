#include "main.h"
#include "imu_sflp.h"
#include "imu.h"

/*
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

	// 1. Включаем SFLP-движок
	lsm6dsv16x_sflp_engine_enable_set(&dev_ctx, LSM6DSV16X_ENABLE);

	// 2. Включаем вывод SFLP в FIFO
	lsm6dsv16x_sflp_output_to_fifo_set(&dev_ctx, LSM6DSV16X_ENABLE);

	// 3. Включаем кватернионы в FIFO (КРИТИЧНО для TAG:0x13!)
	lsm6dsv16x_sflp_quaternion_to_fifo_set(&dev_ctx, LSM6DSV16X_ENABLE);

	// 4. Настраиваем частоту SFLP (опционально, но рекомендуется)
	lsm6dsv16x_sflp_output_data_rate_set(&dev_ctx, LSM6DSV16X_ODR_AT_120Hz);
}
*/
void imu_sflp_init(void)
{
    // 1. Включаем SFLP-движок + ODR=XL + вывод в FIFO (регистр 0x19)
    uint8_t sflp_ctrl = 0x0B;  // Бит 3 (Engine) + Бит 1 (ODR=XL) + Бит 0 (Output to FIFO)
    lsm6dsv16x_write_reg(&dev_ctx, 0x19, &sflp_ctrl, 1);
    HAL_Delay(50);  // Ждём инициализации SFLP (КРИТИЧНО!)

    // 2. Настраиваем вывод КВАТЕРНИОНОВ (регистр 0x1A, бит 2)
    uint8_t sflp_ctrl2 = 0x04;  // Quaternions to FIFO
    lsm6dsv16x_write_reg(&dev_ctx, 0x1A, &sflp_ctrl2, 1);
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
