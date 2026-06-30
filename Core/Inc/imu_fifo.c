#include "imu_fifo.h"
#include "imu.h"  // Для типа imu_data_t
#include <string.h>
#include "lsm6dsv16x_reg.h"

volatile uint32_t imu_irq_cnt = 0;
volatile uint8_t fifo_irq = 0;
volatile uint8_t fifo_event = 0;
// Глобальная структура данных (объявлена в .h)


void imu_fifo_init(stmdev_ctx_t *ctx)
{
    // 1. Настройка маршрутизации прерывания на ножку INT1
    lsm6dsv16x_pin_int_route_t pin_route = {0};
    pin_route.fifo_th = 1;
    lsm6dsv16x_pin_int1_route_set(ctx, &pin_route);

    // 2. Настройка того, ЧТО записывать в FIFO
    lsm6dsv16x_fifo_xl_batch_set(ctx, LSM6DSV16X_XL_BATCHED_AT_120Hz);

    // Включение SFLP batching (TAG 13)
    uint8_t fifo_ctrl4 = 0x08;
    lsm6dsv16x_write_reg(ctx, 0x0A, &fifo_ctrl4, 1);

    // 3. Настройка FIFO (Watermark = 32, запуск режима STREAM)
    lsm6dsv16x_fifo_watermark_set(ctx, 32);
    lsm6dsv16x_fifo_mode_set(ctx, LSM6DSV16X_STREAM_MODE);
}

void imu_fifo_get_registers(stmdev_ctx_t *ctx, uint8_t *reg4b, uint8_t *reg0a)
{
    lsm6dsv16x_read_reg(ctx, 0x4B, reg4b, 1);
    lsm6dsv16x_read_reg(ctx, 0x0A, reg0a, 1);
}

void imu_fifo_service(stmdev_ctx_t *ctx)
{
  lsm6dsv16x_fifo_status_t fifo_status;
  lsm6dsv16x_fifo_out_raw_t f_data; // Используем системный тип драйвера
  uint16_t samples = 0;

  // 1. Проверяем статус FIFO
  if (lsm6dsv16x_fifo_status_get(ctx, &fifo_status) != 0)
  {
    return;
  }

  samples = fifo_status.fifo_level;
  imu_data.fifo_level = samples;

  // 2. Вычитываем данные пакетно
  while (samples--)
{
    // В официальном драйвере функция принимает только 2 аргумента
    if (lsm6dsv16x_fifo_out_raw_get(ctx, &f_data) != 0)
    {
      break;
    }

    imu_data.last_tag = f_data.tag;

    switch (f_data.tag)
    {
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
