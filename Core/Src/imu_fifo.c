#include "imu_fifo.h"
#include <string.h>

// Глобальная структура данных (объявлена в .h)
imu_data_t imu_data;

void imu_fifo_service(stmdev_ctx_t *ctx)
{
  lsm6dsv16x_fifo_status_t fifo_status;
  lsm6dsv16x_fifo_out_raw_t f_data; // Используем системный тип драйвера
  uint16_t samples = 0;

  // 1. Проверяем статус FIFO
  if (lsm6dsv16x_fifo_status_get(ctx, &fifo_status) != 0) {
    return;
  }

  samples = fifo_status.fifo_level;
  imu_data.fifo_level = samples;

  // 2. Вычитываем данные пакетно
  while (samples--) {
    // В официальном драйвере функция принимает только 2 аргумента
    if (lsm6dsv16x_fifo_out_raw_get(ctx, &f_data) != 0) {
      break;
    }

    imu_data.last_tag = f_data.tag;

    switch (f_data.tag) {
      case 0x02: // LSM6DSV16X_XL_NC_TAG (Акселерометр)
        // Временно просто сохраняем тег, парсинг добавим следующим шагом
        break;
      case 0x01: // LSM6DSV16X_GY_NC_TAG (Гироскоп)
        break;
      case 0x13: // LSM6DSV16X_SFLP_GY_QUAT_TAG (Кватернион SFLP)
        break;
      default:
        break;
    }
  }
}
