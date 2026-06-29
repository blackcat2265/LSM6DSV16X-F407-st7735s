#include "display.h"
#include "st7735.h"
#include <stdio.h>
#include "imu_fifo.h"

void display_update(uint32_t irq_cnt) {
    char buf;
    sprintf(buf, "FIFO: %03d  ", imu_data.fifo_level);
    ST7735_WriteString(10, 50, buf, Font_7x10, ST7735_WHITE, ST7735_BLACK); [10]

    sprintf(buf, "TAG:  %02X   ", imu_data.last_tag);
    ST7735_WriteString(10, 60, buf, Font_7x10, ST7735_YELLOW, ST7735_BLACK); [10]
}
