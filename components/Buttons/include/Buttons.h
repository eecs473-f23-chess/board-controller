#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_intr_alloc.h"

void buttons_init();

void IRAM_ATTR draw_button(void * arg);

void IRAM_ATTR resign_button(void * arg);

void IRAM_ATTR make_game_button(void * arg);

void IRAM_ATTR clock_button(void * arg);
