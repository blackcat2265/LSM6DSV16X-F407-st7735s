#ifndef IMU_H
#define IMU_H
#include "lsm6dsv16x_reg.h"

extern stmdev_ctx_t dev_ctx; // Теперь все, кто инклудит imu.h, видят эту переменную
void imu_init(void);
extern imu_data_t imu_data;  // Декларация глобальной переменной

typedef struct {
    uint16_t fifo_level;   // Уровень FIFO
    uint8_t last_tag;      // Последний тег (TAG)
    // Добавьте другие поля при необходимости (например, quaternion)
} imu_data_t;

extern imu_data_t imu_data;  // Декларация для всех .c файлов

#endif
