#include <stdio.h>
#include "clock_display.h"

void app_main(void)
{
    gpio_set_direction((gpio_num_t)RES, GPIO_MODE_OUTPUT); // configure RES as output
    gpio_set_direction((gpio_num_t)CS, GPIO_MODE_OUTPUT);  // configure CS as output
    gpio_set_direction((gpio_num_t)RS, GPIO_MODE_OUTPUT);  // configure RS as output
    gpio_set_direction((gpio_num_t)SC, GPIO_MODE_OUTPUT);  // configure SC as output
    gpio_set_direction((gpio_num_t)SI, GPIO_MODE_OUTPUT);  // configure SI as output
    gpio_set_level((gpio_num_t)RES, LOW);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level((gpio_num_t)RES, HIGH);
    vTaskDelay(pdMS_TO_TICKS(100));
    GraphicLCD_init_LCD();

    vTaskDelay(pdMS_TO_TICKS(1000));

    GraphicLCD_DispNHDPic();
}
