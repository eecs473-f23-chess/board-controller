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
#include "electromagnet.h"
#include "board_state.h"

bool our_turn; // Track whose move it is, should be switched every turn
Board chess_board[8][8];

void app_main(void)
{
    nvs_flash_init();
    xyp_init();
    electromag_init();

    //xyp_calibrate();
    vTaskDelay(pdMS_TO_TICKS(2000));

    xyp_set_board_pos(2,2);
    electromagnet_on(BLACK);
    vTaskDelay(pdMS_TO_TICKS(5000));
    xyp_set_board_pos(2, 7);
    vTaskDelay(pdMS_TO_TICKS(2000));
    electromagnet_off();
    xyp_set_board_pos(5,5);
}
