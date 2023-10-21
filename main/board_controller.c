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

void app_main(void)
{
    nvs_flash_init();

    wifi_init();
    mobile_app_ble_init();
    lichess_api_init_client();

    while (!wifi_is_connected() || !lichess_api_is_logged_in()) {
        vTaskDelay(pdMS_TO_TICKS(250));
    }

    lichess_api_create_game(false, 5, 3);
    printf("%s\n", getColor());
    char* color = getColor();
    if(strcmp(color, "white") == 0){
        lichess_api_make_move("e2e4");
    }
    else{
        lichess_api_make_move("e7e5");
    }
}
