#include "Hall_Effect.h"

int Get_Magnetic(adc_oneshot_unit_handle_t adc_handler){
    int reading;
    adc_cali_scheme_t cali = adc_cali_create_scheme_x();
    adc_oneshot_get_calibrated_result(adc_handler, cali, ADC_CHANNEL_0, reading);
    return reading;
}

adc_oneshot_unit_handle_t ADC_setup(){
    adc_oneshot_unit_handle_t hall_effect;
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    adc_oneshot_new_unit(init_config, hall_effect);
    adc_oneshot_chan_cfg_t config{
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_11,
    };
    adc_oneshot_config_channel(hall_effect, ADC_CHANNEL_0, config);
    return hall_effect;
}