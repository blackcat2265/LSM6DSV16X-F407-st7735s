#include "imu_fifo.h"

uint8_t imu_last_tag = 0;
uint16_t imu_fifo_level = 0;

void imu_fifo_service(stmdev_ctx_t *ctx)
{
    lsm6dsv16x_fifo_status_t fifo;
    lsm6dsv16x_fifo_out_raw_t raw;

    lsm6dsv16x_fifo_status_get(ctx, &fifo);

    imu_fifo_level = fifo.fifo_level;

    if (imu_fifo_level == 0)
        return;

    lsm6dsv16x_fifo_out_raw_get(ctx, &raw);

    imu_last_tag = raw.tag;
}

#include "imu_fifo.h"

volatile uint8_t  fifo_irq = 0;
volatile uint8_t  imu_last_tag = 0;
volatile uint16_t imu_fifo_level = 0;

void imu_fifo_service(stmdev_ctx_t *ctx)
{
    lsm6dsv16x_fifo_status_t fifo_status;

    lsm6dsv16x_fifo_status_get(ctx, &fifo_status);

    imu_fifo_level = fifo_status.fifo_level;

    while (fifo_status.fifo_level)
    {
        lsm6dsv16x_fifo_out_raw_t fifo_raw;

        lsm6dsv16x_fifo_out_raw_get(ctx, &fifo_raw);

        imu_last_tag = fifo_raw.tag;

        fifo_status.fifo_level--;
    }
}


