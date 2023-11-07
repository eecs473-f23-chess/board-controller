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

#define POSITIVE 1150
#define NEGATIVE 850

int Get_Magnetic();

void ADC_setup();

void set_Square_Mux(int i);

void set_Board_Mux(int i);

void select_xy_sensor(int x, int y);

void poll_board();

void compare(char board_after [8][8], char* move);

void map_array_coordinate_to_chess_square(int x, int y, char* move);

void update_board_based_off_opponent_move(char* move);