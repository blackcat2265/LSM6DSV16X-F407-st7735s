#include "imu_fifo.h"
#include <string.h>
imu_data_t imu_data; // Глобальная структура для хранения последних данных
void imu_fifo_service(stmdev_ctx_t *ctx)
{
  lsm6dsv16x_fifo_status_t fifo_status;
  uint8_t tag;
  uint8_t data[1];
  uint16_t samples = 0;

  // 1. Проверяем, сколько данных накопилось в FIFO
  if (lsm6dsv16x_fifo_status_get(ctx, &fifo_status) != 0) {
    return;
  }

  samples = fifo_status.fifo_level;

  // 2. Вычитываем FIFO пакет за пакетом
  while (samples--) {
    // Чтение тега и 6 байт данных (автоматически сдвигает указатель FIFO в датчике)
    if (lsm6dsv16x_fifo_out_raw_get(ctx, &tag, data) != 0) {
      break;
    }

    // Обработка данных в зависимости от тега (подготовка под SFLP)
    switch (tag) {
      case LSM6DSV16X_XL_NC_TAG:
        // Здесь будет обработка акселерометра из FIFO
        break;
      case LSM6DSV16X_GY_NC_TAG:
        // Здесь будет обработка гироскопа из FIFO
        break;
      case LSM6DSV16X_SFLP_GY_QUAT_TAG:
        // Здесь будет обработка кватерниона SFLP
        break;
      default:
        // Прочие теги игнорируем
        break;
    }
  }
}
