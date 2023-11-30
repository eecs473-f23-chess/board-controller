#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "esp_adc/adc_oneshot.h"
#include "hal/adc_types.h"
#include "esp_adc/adc_cali_scheme.h"
#include <stdio.h>
#include "freertos/timers.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_continuous.h"
#include "string.h"
#include "types.h"
#include "board_state.h"

#define POSITIVE 1050
#define NEGATIVE 950

int Get_Magnetic();

void ADC_setup();

void set_Square_Mux(int i);

void set_Board_Mux(int i);

void select_xy_sensor(int x, int y);

bool poll_board(board_state_t* board_state, char * move_made);

void compare(char board_after [8][8], char* move, int index);

void map_array_coordinate_to_chess_square(int x, int y, char* move);