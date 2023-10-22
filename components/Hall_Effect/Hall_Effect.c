#include "Hall_Effect.h"
//0 - 46, 1 - 9, 2 - 10

adc_oneshot_unit_handle_t hall_effect;
adc_cali_handle_t cali;

int Get_Magnetic(adc_oneshot_unit_handle_t adc_handler){
    int reading;
    adc_oneshot_get_calibrated_result(hall_effect, cali, ADC_CHANNEL_0, &reading);
    return reading;
}

void ADC_setup(){
    gpio_set_direction(46, GPIO_MODE_OUTPUT);
    gpio_set_direction(10, GPIO_MODE_OUTPUT);
    gpio_set_direction(9, GPIO_MODE_OUTPUT);
    esp_rom_gpio_pad_select_gpio(39);
    esp_rom_gpio_pad_select_gpio(40);
    gpio_set_direction(39, GPIO_MODE_OUTPUT);
    gpio_set_direction(40, GPIO_MODE_OUTPUT);
    gpio_set_direction(38, GPIO_MODE_OUTPUT);
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
        .clk_src = 0,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    adc_oneshot_new_unit(&init_config, &hall_effect);
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_11,
    };
    adc_oneshot_config_channel(hall_effect, ADC_CHANNEL_0, &config);
    adc_cali_curve_fitting_config_t curve_config = {
        .unit_id = ADC_UNIT_1,
        .chan = ADC_CHANNEL_0,
        .atten = ADC_ATTEN_DB_11,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    adc_cali_create_scheme_curve_fitting(&curve_config, &cali);
}

void set_Board_Mux(int i){
    if(i == 7){
        gpio_set_level(46, 1);
        gpio_set_level(9, 1);
        gpio_set_level(10, 1);
    }
    else if(i == 6){
        gpio_set_level(46, 0);
        gpio_set_level(9, 1);
        gpio_set_level(10, 1);
    }
    else if(i == 5){
        gpio_set_level(46, 1);
        gpio_set_level(9, 0);
        gpio_set_level(10, 1);
    }
    else if(i == 4){
        gpio_set_level(46, 0);
        gpio_set_level(9, 0);
        gpio_set_level(10, 1);
    }
    else if(i == 3){
        gpio_set_level(46, 1);
        gpio_set_level(9, 1);
        gpio_set_level(10, 0);
    }
    else if(i == 2){
        gpio_set_level(46, 0);
        gpio_set_level(9, 1);
        gpio_set_level(10, 0);
    }
    else if(i == 1){
        gpio_set_level(46, 1);
        gpio_set_level(9, 0);
        gpio_set_level(10, 0);
    }
    else if(i == 0){
        gpio_set_level(46, 0);
        gpio_set_level(9, 0);
        gpio_set_level(10, 0);
    }

}

void set_Square_Mux(int i){
    if(i == 7){
        gpio_set_level(38, 1);
        gpio_set_level(39, 1);
        gpio_set_level(40, 1);
    }
    else if(i == 6){
        gpio_set_level(38, 0);
        gpio_set_level(39, 1);
        gpio_set_level(40, 1);
    }
    else if(i == 5){
        gpio_set_level(38, 1);
        gpio_set_level(39, 0);
        gpio_set_level(40, 1);
    }
    else if(i == 4){
        gpio_set_level(38, 0);
        gpio_set_level(39, 0);
        gpio_set_level(40, 1);
    }
    else if(i == 3){
        gpio_set_level(38, 1);
        gpio_set_level(39, 1);
        gpio_set_level(40, 0);
    }
    else if(i == 2){
        gpio_set_level(38, 0);
        gpio_set_level(39, 1);
        gpio_set_level(40, 0);
    }
    else if(i == 1){
        gpio_set_level(38, 1);
        gpio_set_level(39, 0);
        gpio_set_level(40, 0);
    }
    else if(i == 0){
        gpio_set_level(38, 0);
        gpio_set_level(39, 0);
        gpio_set_level(40, 0);
    }

}

void select_xy_sensor(int x, int y){
    set_Board_Mux(x);
    set_Square_Mux(y);
}