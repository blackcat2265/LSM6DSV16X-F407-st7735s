#include "main.h"
#include "display.h"
#include "st7735.h"
#include <stdio.h>
#include "imu_fifo.h"
#include "imu.h" // Нужно для доступа к dev_ctx

void display_update(uint32_t irq_cnt) {
    char buf;
    // 1. Вывод уровня FIFO
       sprintf(buf, "FIFO: %03d  ", imu_data.fifo_level);
       ST7735_WriteString(10, 50, buf, Font_7x10, ST7735_WHITE, ST7735_BLACK);

       // 2. Вывод последнего Тега (должен стать 13)
       sprintf(buf, "TAG:  %02X   ", imu_data.last_tag);
       ST7735_WriteString(10, 60, buf, Font_7x10, ST7735_YELLOW, ST7735_BLACK);

       // 3. Счётчик прерываний
       sprintf(buf, "IRQ: %lu   ", irq_cnt);
       ST7735_WriteString(10, 30, buf, Font_7x10, ST7735_MAGENTA, ST7735_BLACK);

       // 4. Проверка связи (ID датчика)
       uint8_t id = 0;
       lsm6dsv16x_device_id_get(&dev_ctx, &id);
       sprintf(buf, "ID:   %02X   ", id);
       ST7735_WriteString(10, 80, buf, Font_7x10, ST7735_CYAN, ST7735_BLACK);
   }
