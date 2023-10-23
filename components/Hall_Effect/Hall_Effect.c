#include "Hall_Effect.h"
//0 - 46, 1 - 9, 2 - 10

adc_oneshot_unit_handle_t hall_effect; //ADC setup stuff
adc_cali_handle_t cali;
char board[8][8]; //State of the board

struct coordinate{
    int x;
    int y;
    char original;
};

struct coordinate changes[4]; //Will store the changes in the board, should only be 4 max
                              //4 for castling, 2 for normal move

int Get_Magnetic(adc_oneshot_unit_handle_t adc_handler){ // Will read ADC pin
    int reading;
    adc_oneshot_get_calibrated_result(hall_effect, cali, ADC_CHANNEL_0, &reading);
    return reading;
}

void ADC_setup(){ //Setups up MUX and ADC pins
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

void set_Board_Mux(int i){ //Selects which channel the first mux is on
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

void set_Square_Mux(int i){ //Selects which channel the second mux will look at
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

void select_xy_sensor(int x, int y){ //Selects a specific hall effect sensor to look at
    set_Board_Mux(x);
    set_Square_Mux(y);
}

void poll_board(){ // Polls the entire board, reading each hall effect sensor and recording the state of the board, as well as which
                   // coordinates have changed, and from what
    int index = 0;
    int reading;
    for(int i = 0; i < 8; ++i){
        for(int j = 0; j < 8; ++j){
            select_xy_sensor(i, j);
            reading = Get_Magnetic(hall_effect);
            if(reading > 1500){
                if(board[i][j] != 'b'){
                    struct coordinate change;
                    change.x = i;
                    change.y = j;
                    change.original = board[i][j];
                    changes[index] = change;
                    ++index;
                }
                board[i][j] = 'b';
            }
            else if(reading < 500){
                if(board[i][j] != 'w'){
                    struct coordinate change;
                    change.x = i;
                    change.y = j;
                    change.original = board[i][j];
                    changes[index] = change;
                    ++index;
                }
                board[i][j] = 'w';
            }
            else{
                if(board[i][j] != 'n'){
                    struct coordinate change;
                    change.x = i;
                    change.y = j;
                    change.original = board[i][j];
                    changes[index] = change;
                    ++index;
                }
                board[i][j] = 'n';
            }
        }
    }
}