#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_intr_alloc.h"
<<<<<<< HEAD
#include "lichess_api.h"
=======
extern int var;
extern SemaphoreHandle_t xSemaphore;
extern SemaphoreHandle_t xSemaphore_Resign;
extern SemaphoreHandle_t xSemaphore_Draw;
extern SemaphoreHandle_t xSemaphore_DataTransfer;

>>>>>>> 4247ee6 (Make game seems to work, others in progress)

void buttons_init();
void IRAM_ATTR draw_button(void * arg);
void IRAM_ATTR resign_button(void * arg);
void IRAM_ATTR make_game_button(void * arg);
void IRAM_ATTR clock_button(void * arg);
