#ifndef IMU_FIFO_H
#define IMU_FIFO_H

#include "lsm6dsv16x_reg.h"
#include "imu.h"  // Для типа imu_data_t


extern volatile uint32_t imu_irq_cnt;
extern volatile uint8_t fifo_irq;
extern volatile uint8_t fifo_event;




// Прототип функции обслуживания FIFO
void imu_fifo_init(stmdev_ctx_t *ctx);
void imu_fifo_service(stmdev_ctx_t *ctx);
void imu_fifo_display_status(stmdev_ctx_t *ctx);
void imu_fifo_get_registers(stmdev_ctx_t *ctx, uint8_t *reg4b, uint8_t *reg0a);


#endif /* IMU_FIFO_H */
