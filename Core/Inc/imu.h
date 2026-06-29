#ifndef IMU_H
#define IMU_H
#include "lsm6dsv16x_reg.h"

extern stmdev_ctx_t dev_ctx; // Теперь все, кто инклудит imu.h, видят эту переменную
void imu_init(void);
#endif
