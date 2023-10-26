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
#include "score_display.h"
#include "clock_display.h"

int P1_time; //Tracks our time
int P2_time; //Tracks opponents time
bool our_turn; // Track whose move it is, should be switched every turn

void app_main(void)
{
    nvs_flash_init();

    wifi_init();
    mobile_app_ble_init();
    lichess_api_init_client();

    while (!wifi_is_connected() || !lichess_api_is_logged_in()) {
        vTaskDelay(pdMS_TO_TICKS(250));
    }

    GraphicLCD_init_LCD();
    scoreboard_init();
    // //Need app input here for create game parameters
    lichess_api_create_game(false, 5, 3);
    // //printf("%s\n", getColor());


    //Whenever we get the functions to do so, call this
    scoreboard_Chess_Setup(name1, name2, country1, country2, rating1, rating2)

    //Should be called whenver we find out the color
    if(strcmp(getColor(), "white")){
        our_turn = true;
    }
    else if(strcmp(getColor(), "black")){
        our_turn = false;
    }


    //Whenever we first get the time these two lines should be called
    GraphicLCD_DispClock(P1_time, true);
    GraphicLCD_DispClock(P2_time, false);
    

    xTaskCreate(&decrement_time, "Clock", 2048, NULL, 1, NULL); //Creates the clock task
}
