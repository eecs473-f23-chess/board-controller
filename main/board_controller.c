#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <nvs_flash.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <wifi.h>

#include "types.h"
#include "lichess_api.h"
#include "mobile_app_ble.h"
#include "clock_display.h"
#include "score_display.h"
#include "xy_plotter.h"
#include "electromagnet.h"

int P1_time; //Tracks our time
int P2_time; //Tracks opponents time
bool our_turn; // Track whose move it is, should be switched every turn


void app_main(void)
{
    nvs_flash_init();
    xyp_init();
    printf("finished xpy init\n");
    vTaskDelay(pdMS_TO_TICKS(500));

    printf("starting calibration\n");
    xyp_calibrate();
    printf("finished calibration\n");

    vTaskDelay(pdMS_TO_TICKS(5000));

    printf("setting board pos to 2,2\n");
    xyp_set_board_pos(2,2);
    vTaskDelay(pdMS_TO_TICKS(5000));
    printf("setting board pos to 5,5\n");
    xyp_set_board_pos(5,5);
}
