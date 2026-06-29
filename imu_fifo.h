#ifndef IMU_FIFO_H
#define IMU_FIFO_H

#include "lsm6dsv16x_reg.h"

typedef struct {
    float lux, luy, luz;    // Акселерометр (g)
    float lgx, lgy, lgz;    // Гироскоп (dps)
    float pitch, roll;      // Углы SFLP
    float q[4];             // Кватернион SFLP
    uint16_t fifo_level;    // Текущая заполненность
    uint8_t last_tag;       // Последний обработанный тег
} imu_data_t;

extern imu_data_t imu_data; // Сделаем данные доступными для main.c

// Прототип функции обслуживания FIFO
void imu_fifo_service(stmdev_ctx_t *ctx);

#endif /* IMU_FIFO_H */
