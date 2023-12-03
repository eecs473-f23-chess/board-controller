#include "Hall_Effect.h"
//0 - 46, 1 - 9, 2 - 10

adc_oneshot_unit_handle_t hall_effect; //ADC setup stuff
adc_cali_handle_t cali;
 //State of the board



struct coordinate{
    int x;
    int y;
};

struct coordinate changes[99]; //Will store the changes in the board, should only be 4 max
                              //4 for castling, 2 for normal move, 3 for en passant

int Get_Magnetic(){ // Will read ADC pin
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
    if(y == 0){
        set_Square_Mux(2);
    }
    else if(y == 1){
        set_Square_Mux(4);
    }
    else if(y == 2){
        set_Square_Mux(1);
    }
    else if(y == 3){
        set_Square_Mux(6);
    }
    else if(y == 4){
        set_Square_Mux(0);
    }
    else if(y == 5){
        set_Square_Mux(7);
    }
    else if(y == 6){
        set_Square_Mux(3);
    }
    else if(y == 7){
        set_Square_Mux(5);
    }
    set_Board_Mux(x);
}

bool poll_board(board_state_t* board_state, char * move_made){ // Polls the entire board, reading each hall effect sensor and recording the state of the board, as well as which
                   // coordinates have changed, and from what
    char cur_board[8][8];
    int index = 0;
    int reading;
    printf("--------------------------------------------\n");
    for(int i = 0; i < 8; ++i){
        for(int j = 0; j < 8; ++j){
            select_xy_sensor(i, j);
            vTaskDelay(pdMS_TO_TICKS(50));
            reading = Get_Magnetic();
            printf("%d     ", reading);
            if(reading < NEGATIVE){ // If Black
                if(board_state->board[i][j] == WK || board_state->board[i][j] == WQ || board_state->board[i][j] == WN ||
                    board_state->board[i][j] == WB || board_state->board[i][j] == WR || board_state->board[i][j] == WP || board_state->board[i][j] == NP){
                    printf("deteched black piece change\n");
                    struct coordinate change;
                    change.x = i;
                    change.y = j;
                    changes[index] = change;
                    ++index;
                }
                cur_board[i][j] = 'B';
            }
            else if(reading > POSITIVE){ //If white
                if(board_state->board[i][j] == BK || board_state->board[i][j] == BQ || board_state->board[i][j] == BN ||
                    board_state->board[i][j] == BB || board_state->board[i][j] == BRK || board_state->board[i][j] == BP || board_state->board[i][j] == NP){
                    printf("detected white piece change\n");
                    struct coordinate change;
                    change.x = i;
                    change.y = j;
                    changes[index] = change;
                    ++index;
                }
                cur_board[i][j] = 'W';
            }
            else{
                if(board_state->board[i][j] != NP){
                    printf("change from no piece\n");
                    struct coordinate change;
                    change.x = i;
                    change.y = j;
                    changes[index] = change;
                    ++index;
                }
                cur_board[i][j] = '-';
            }
        }
        printf("\n");
    }

    printf("Found changes on the following squares:\n");
    for (int i = 0; i < index; ++i) {
        printf("\tX: %d, Y: %d\n", changes[i].x, changes[i].y);
    }
    if (index >= 5) {
        printf("Found more changes than possible, exiting\n");
        return false;
    }
    compare(cur_board, move_made, index);
    return true;
}

void map_array_coordinate_to_chess_square(int x, int y, char* move){
    int rank = 8 - x;
    char rank_as_char = (char)(rank + '0');
    char file = (char)(y + 'a');
    char coordinate[3] = {};
    coordinate[0] = file;
    coordinate[1] = rank_as_char;
    coordinate[2] = 0;
    strcpy(move, coordinate);
}

void compare(char board_after [8][8], char* move, int index){
    printf("index is %d\n", index);
    if(index == 0 || index > 4) {
        return;
    }
    if(index == 3){
        struct coordinate src;
        struct coordinate dest;
        dest.x = -1;
        dest.y = -1;
        for(int i = 0; i < 3; i++){
            struct coordinate curr = changes[i];
            if(dest.x != -1){
                break;
            }
            if(board_after[curr.x][curr.y] != '-'){
                dest.x = curr.x;
                dest.y = curr.y;
            }
        }
        for(int i = 0; i < 3; i++){
            struct coordinate curr = changes[i];
            if(abs(curr.x - dest.x) == 1 && abs(curr.y - dest.y) == 1){
                src.x = curr.x;
                src.y = curr.y;
                break;
            }
        }
        char source_move[5] = {};
        char dest_move [5] = {};
        map_array_coordinate_to_chess_square(src.x, src.y, source_move);
        map_array_coordinate_to_chess_square(dest.x, dest.y, dest_move);
        strcat(move, source_move);
        strcat(move, dest_move);
        return;
    }
    // Castling happened
    if(index == 4){
        if(changes[0].x == 0){
            if(changes[0].y == 0){
                strcpy(move, "e8c8");
                return;
            }
            else{
                if(changes[3].y == 7){
                    // Black kingside castled
                    strcpy(move, "e8g8");
                    return;
                }
            }
        }
        else if(changes[0].x == 7){
            if(changes[0].y == 0){
                strcpy(move, "e1c1");
                return;
            }
            else if(changes[3].y == 7){
                strcpy(move, "e1g1");
                return;
            }
        }
    }
    char src[5] = {};
    char dest[5] = {};
    for(int i = 0; i < index; i++){
        struct coordinate c = changes[i];
        printf("Changes[i] = %d %d\n", c.x, c.y);
        if(board_after[c.x][c.y] == '-'){
            map_array_coordinate_to_chess_square(c.x, c.y, src);
            printf("In compare, source square is %s\n", src);
        }   
        else{
            map_array_coordinate_to_chess_square(c.x, c.y, dest);
            printf("In compare, destination square is %s\n", dest);
        }
    }
    strcat(move, src);
    strcat(move, dest);
    printf("End of compare, move is %s\n", move);
}