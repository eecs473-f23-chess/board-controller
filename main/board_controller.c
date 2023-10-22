#include <stdio.h>
#include "Hall_Effect.h"

void app_main(void)
{
    adc_oneshot_unit_handle_t hall_effect;
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    adc_oneshot_new_unit(&init_config, &hall_effect);
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_11,
    };
    adc_oneshot_config_channel(hall_effect, ADC_CHANNEL_0, &config);
    // ADC_setup();
    while(1){
        int reading;
        // adc_cali_scheme_t cali = {
        //     .unit_id = ADC_UNIT_1,
        //     .atten = ADC_ATTEN_DB_11,
        //     .bitwidth = ADC_BITWIDTH_DEFAULT,
        // }
        esp_adc_cali_new_scheme();
        adc_oneshot_get_calibrated_result(hall_effect, cali, ADC_CHANNEL_0, &reading);
        printf("Volts: %d", reading);
    }
}
