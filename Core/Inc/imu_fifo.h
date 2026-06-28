#ifndef IMU_FIFO_H
#define IMU_FIFO_H

#include "lsm6dsv16x_reg.h"

void imu_fifo_service(stmdev_ctx_t *ctx);

extern uint8_t imu_last_tag;
extern uint16_t imu_fifo_level;

#endif

#pragma once

#include "lsm6dsv16x_reg.h"

extern volatile uint8_t  fifo_irq;
extern volatile uint8_t  imu_last_tag;
extern volatile uint16_t imu_fifo_level;

void imu_fifo_service(stmdev_ctx_t *ctx);
