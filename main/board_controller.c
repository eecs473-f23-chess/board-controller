#include <stdio.h>
#include "Buttons.h"

int test = 0;



static void gpio_task_example(void* arg)
{
    while(true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        printf("%d\n", test);
    }
}

void app_main(void)
{
    buttons_init();
   xTaskCreate(gpio_task_example, "gpio_task_example", 2048, NULL, 0, NULL);
   while(true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        printf("%d\n", test);
    }
}
