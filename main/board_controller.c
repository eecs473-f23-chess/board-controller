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

#include "lichess_api.h"
#include "mobile_app_ble.h"
#include "clock_display.h"
#include "score_display.h"
#include "xy_plotter.h"

int P1_time; //Tracks our time
int P2_time; //Tracks opponents time
bool our_turn; // Track whose move it is, should be switched every turn


void app_main(void)
{
    nvs_flash_init();
    xyp_init();

    xyp_set_board_pos(0, 1);
    vTaskDelay(pdMS_TO_TICKS(1000));
    xyp_set_board_pos(0, 2);
    vTaskDelay(pdMS_TO_TICKS(1000));
    xyp_set_board_pos(0, 3);
    vTaskDelay(pdMS_TO_TICKS(1000));
    xyp_set_board_pos(0, 4);
    vTaskDelay(pdMS_TO_TICKS(1000));
    xyp_set_board_pos(0, 6);
    vTaskDelay(pdMS_TO_TICKS(1000));
    xyp_set_board_pos(0, 8);
    vTaskDelay(pdMS_TO_TICKS(1000));
    xyp_set_board_pos(0, 4);
    vTaskDelay(pdMS_TO_TICKS(1000));
    xyp_set_board_pos(0, 2.5);
    vTaskDelay(pdMS_TO_TICKS(1000));
    xyp_set_board_pos(0, 1.5);
    vTaskDelay(pdMS_TO_TICKS(1000));
    xyp_set_board_pos(0, 0.5);
    vTaskDelay(pdMS_TO_TICKS(1000));
    xyp_set_board_pos(0, 0);
    vTaskDelay(pdMS_TO_TICKS(1000));
}
