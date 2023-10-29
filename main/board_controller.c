#include <stdio.h>
#include "Hall_Effect.h"

char board[8][8];

void app_main(void)
{
    // adc_oneshot_unit_handle_t hall_effect;
    // adc_oneshot_unit_init_cfg_t init_config = {
    //     .unit_id = ADC_UNIT_1,
    //     .clk_src = 0,
    //     .ulp_mode = ADC_ULP_MODE_DISABLE,
    // };
    // adc_oneshot_new_unit(&init_config, &hall_effect);
    // adc_oneshot_chan_cfg_t config = {
    //     .bitwidth = ADC_BITWIDTH_DEFAULT,
    //     .atten = ADC_ATTEN_DB_11,
    // };
    // adc_oneshot_config_channel(hall_effect, ADC_CHANNEL_0, &config);
    // adc_cali_curve_fitting_config_t curve_config = {
    //     .unit_id = ADC_UNIT_1,
    //     .chan = ADC_CHANNEL_0,
    //     .atten = ADC_ATTEN_DB_11,
    //     .bitwidth = ADC_BITWIDTH_DEFAULT,
    // };
    // adc_cali_handle_t cali;
    // adc_cali_create_scheme_curve_fitting(&curve_config, &cali);
    // // int gpio;
    // // adc_continuous_channel_to_io(ADC_UNIT_1, ADC_CHANNEL_0, &gpio);
    // // printf("Pin: %d", gpio);
    ADC_setup();

    // poll_board(board);

    // for(int i = 0; i < 8; i++){
    //     for(int j = 0; j < 8; j++){
    //         printf("%d", board[i][j]);
    //     }
    //     printf("\n");
    // }

}
