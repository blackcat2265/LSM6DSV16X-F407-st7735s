#ifndef IMU_H
#define IMU_H
#include "lsm6dsv16x_reg.h"

// --- Определение типа (ДО использования) ---
typedef struct {
    float lux, luy, luz;     // Акселерометр (g)
    float lgx, lgy, lgz;     // Гироскоп (dps)
    float pitch, roll;       // Углы SFLP
    float q[4];              // Кватернион SFLP
    uint16_t fifo_level;    // Уровень FIFO
    uint8_t last_tag;        // Последний тег
} imu_data_t;

// --- Декларации переменных ---
extern imu_data_t imu_data;  // Глобальные данные
extern stmdev_ctx_t dev_ctx; // Контекст датчика

// --- Прототипы функций ---
void imu_init(void);

#endif
