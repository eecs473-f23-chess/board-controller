#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_intr_alloc.h"
#include "lichess_api.h"
extern SemaphoreHandle_t xSemaphore;
extern SemaphoreHandle_t xSemaphore_Resign;
extern SemaphoreHandle_t xSemaphore_Draw;
extern SemaphoreHandle_t xSemaphore_DataTransfer;
extern SemaphoreHandle_t xSemaphore_MakeMove;


void buttons_init();
void IRAM_ATTR draw_button(void * arg);
void IRAM_ATTR resign_button(void * arg);
void IRAM_ATTR make_game_button(void * arg);
void IRAM_ATTR clock_button(void * arg);
