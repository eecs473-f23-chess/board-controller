
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
#include "Buttons.h"
#include "freertos/semphr.h"

bool our_turn; // Track whose move it is, should be switched every turn
int var = 0;
SemaphoreHandle_t xSemaphore;
SemaphoreHandle_t xSemaphore_Resign;
SemaphoreHandle_t xSemaphore_Draw;
bool game_created = false;

void lichess_api_create_game_helper(){
    for(;;){
        xSemaphoreTake(xSemaphore, portMAX_DELAY);
        lichess_api_create_game(true, 15, 5);
    }    
}

void lichess_api_resign_game_helper(){
    for(;;){
        xSemaphoreTake(xSemaphore_Resign, portMAX_DELAY);
        lichess_api_resign_game();
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void lichess_api_handle_draw_helper(){
    for(;;){
        xSemaphoreTake(xSemaphore_Draw, portMAX_DELAY);
        lichess_api_handle_draw();
    }
}

// TODO, not used as of right now
void lichess_stream_game_moves_helper(){
    for(;;){
        if(game_created){
            printf("Game created. Entering now\n");
            lichess_api_stream_move_of_game();
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
}

void app_main(void)
{

    xSemaphore = xSemaphoreCreateBinary();
    xSemaphore_Resign = xSemaphoreCreateBinary();
    xSemaphore_Draw = xSemaphoreCreateBinary();
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
    wifi_init();
    GraphicLCD_init_LCD();
    scoreboard_init();
    buttons_init();

    wifi_connect();
    lichess_api_init_client();
    const char token_fake = "fake";
    lichess_api_login(token_fake, 10);
    xTaskCreate(&lichess_api_create_game_helper, "Create a lichess game", 8192, NULL, 4, NULL);
    xTaskCreate(&lichess_api_resign_game_helper, "Resign the current lichess game", 4096, NULL, 5, NULL);  
    // xTaskCreate(&decrement_time, "Clock", 2048, NULL, 1, NULL);
    
    
    // mobile_app_ble_init();
    
    
    // TODO, uncomment
    // while (!wifi_is_connected() || !lichess_api_is_logged_in()) {
    //     vTaskDelay(pdMS_TO_TICKS(250));
    // }

    
    
    // //Need app input here for create game parameterss
    // //printf("%s\n", getColor());

    xyp_set_board_pos(7.0, 1.0);
    electromagnet_on(WHITE);
    xyp_set_board_pos(6.5, 1);
    xyp_set_board_pos(6.5, 3);
    xyp_set_board_pos(6, 3);
    vTaskDelay(pdMS_TO_TICKS(10000));
    electromagnet_off();
    xyp_return_home();

#endif
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
    // const char token_fake = "fake";
    // lichess_api_login(token_fake, 10);
    // lichess_api_create_game(true, 15, 3);
    // 
    // if(strcmp(getColor(), "white")){
    //     our_turn = true;
    // }
    // else if(strcmp(getColor(), "black")){
    //     our_turn = false;
    // }
    // 
}
