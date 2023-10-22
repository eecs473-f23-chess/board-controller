#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "esp_adc/adc_oneshot.h"
#include "hal/adc_types.h"
#include "esp_adc/adc_cali_scheme.h"
#include <stdio.h>

extern int Get_Magnetic(adc_oneshot_unit_handle_t adc_handler);

adc_oneshot_unit_handle_t ADC_setup();