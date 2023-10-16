#include <nvs_flash.h>
#include <stddef.h>
#include "lichess_api.h"
#include "mobile_app_ble.h"
#include "wifi.h"
#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

void app_main(void)
{
    nvs_flash_init();
    wifi_init();
    mobile_app_ble_init();
    connect_wifi();
    lichess_api_init_client(); 
    lichess_api_create_game(true, 5, 3);
    printf("%s\n", getColor());
    char* color = getColor();
    if(strcmp(color, "white") == 0){
        lichess_api_make_move("e2e4");
    }
    else{
        lichess_api_make_move("e7e5");
    }
}
