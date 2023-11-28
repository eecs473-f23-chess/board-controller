
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

void app_main(void)
{
    nvs_flash_init();

#ifdef XYP_JOYSTICK_TEST
    xyp_init();
    electromag_init();
    // xyp_calibrate();
    xyp_joystick_control();
#else
    xyp_init();
    electromag_init();

    xyp_calibrate();
    printf("finished calibration\n");

    xyp_set_board_pos(7.0, 1.0);
    electromagnet_on(WHITE);
    xyp_set_board_pos(6.5, 1);
    xyp_set_board_pos(6.5, 3);
    xyp_set_board_pos(6, 3);
    vTaskDelay(pdMS_TO_TICKS(10000));
    electromagnet_off();
    xyp_return_home();

#endif
}
