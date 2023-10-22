#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "esp_adc/adc_oneshot.h"
#include "hal/adc_types.h"
#include "esp_adc/adc_cali_scheme.h"
#include <stdio.h>
#include "freertos/timers.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_continuous.h"

int Get_Magnetic();

void ADC_setup();

void set_Square_Mux(int i);

void set_Board_Mux(int i);

void select_xy_sensor(int x, int y);