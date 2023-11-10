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

int P1_time; //Tracks our time
int P2_time; //Tracks opponents time
bool our_turn; // Track whose move it is, should be switched every turn


void app_main(void)
{
    nvs_flash_init();
    wifi_init();
    mobile_app_ble_init();
    lichess_api_init_client();

    // TODO, uncomment
    // while (!wifi_is_connected() || !lichess_api_is_logged_in()) {
    //     vTaskDelay(pdMS_TO_TICKS(250));
    // }

    GraphicLCD_init_LCD();
    scoreboard_init();
    // //Need app input here for create game parameterss
    // //printf("%s\n", getColor());


    //Whenever we get the functions to do so, call this
    // scoreboard_Chess_Setup(name1, name2, country1, country2, rating1, rating2)

    //Should be called whenver we find out the color
   
    // TODO, uncomment
   
    // if(strcmp(color, "white") == 0){
    //     lichess_api_make_move("e2e4");
    // }
    // else{
    //     lichess_api_make_move("e7e5");
    // }
    wifi_connect();
    const char token_fake = "fake";
    lichess_api_login(token_fake, 10);
    lichess_api_create_game(true, 15, 3);
    xTaskCreate(&lichess_api_stream_move_of_game, "get opponent move", 8192, NULL, 4, NULL);
    if(strcmp(getColor(), "white")){
        our_turn = true;
    }
    else if(strcmp(getColor(), "black")){
        our_turn = false;
    }
    xTaskCreate(&decrement_time, "Clock", 2048, NULL, 1, NULL);
}
