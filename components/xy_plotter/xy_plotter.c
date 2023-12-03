#include "xy_plotter.h"

#ifdef XYP_JOYSTICK_TEST
#include <driver/adc.h>
#endif
#include <driver/gpio.h>
#include <driver/gptimer.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>
#include <string.h>
#include <math.h>

#include "electromagnet.h"

// GPIOs
#define X_DIR_GPIO              0
#define X_STEP_GPIO             35
#define Y_DIR_GPIO              36
#define Y_STEP_GPIO             37
#define LIMITX_GPIO             41
#define LIMITY_GPIO             42
#define X_ENBL_GPIO             2
#define Y_ENBL_GPIO             44
#ifdef XYP_JOYSTICK_TEST
#define X_JOYSTICK_GPIO         19
#define Y_JOYSTICK_GPIO         20
#endif

// Alarm/timer config
#define TIMER_RESOLUTION        1e6 // 1Mhz
#define ALARM_PERIOD            1e-3 // 1ms
#define ALARM_COUNT             TIMER_RESOLUTION * ALARM_PERIOD
#define CAL_ALARM_COUNT         ALARM_COUNT

// Stepper motors
#define STEPS_PER_INCH          126 // Found experimentally
#define INCHES_TO_STEPS(INCHES) ((STEPS_PER_INCH) * (INCHES))

// Chess board
#define SQUARE_SIZE             2.5
#define X_LIMIT_OFFSET_SQUARES  0.67
#define Y_LIMIT_OFFSET_SQUARES  0.17

// Event group bits
#define X_BIT                   BIT0
#define Y_BIT                   BIT1
#define CAL_BIT                 BIT2

#define HIGH                    1
#define LOW                     0

// direction gpio levels
#define LEFT_DIR    0
#define RIGHT_DIR   1

struct xyp_stepper {
    gptimer_handle_t timer;
    gpio_num_t dir_gpio;
    gpio_num_t step_gpio;
    gpio_num_t enbl_gpio;
    float current_board_pos; // in units of squares
    uint32_t steps_remaining;
    uint8_t step_state;
    uint8_t event_group_bit;
#ifdef XYP_JOYSTICK_TEST
    adc2_channel_t joystick_channel;
    int32_t current_steps;
#endif
};

typedef enum CAL_STEPPER_MOVE {
    TIMED_MOVE,
    LIMIT_MOVE,
} cal_stepper_move_t;

typedef enum CAL_AXIS {
    X_AXIS,
    Y_AXIS,
} cal_axis_t;

// Timers for step pulse generation
static struct xyp_stepper x_stepper;
static struct xyp_stepper y_stepper;

// Event group for stepper position reached
EventGroupHandle_t stepper_event_group;

// Event group for calibration
EventGroupHandle_t xy_calibration_event_group;

// Calibration globals
cal_stepper_move_t current_move;
cal_axis_t current_axis;
gpio_num_t current_switch;

bool move_timer_callback(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx) {
    struct xyp_stepper* stepper = NULL;
    if (timer == x_stepper.timer) {
        stepper = &x_stepper;
    }
    else if (timer == y_stepper.timer) {
        stepper = &y_stepper;
    }
    else {
        return false;
    }

    stepper->step_state = !stepper->step_state;
    gpio_set_level(stepper->step_gpio, stepper->step_state);

#ifdef XYP_JOYSTICK_TEST
    int joystick_raw;
    adc2_get_raw(stepper->joystick_channel, ADC_WIDTH_BIT_12, &joystick_raw);
    if (joystick_raw < 826) {
        gpio_set_level(stepper->dir_gpio, LEFT_DIR);
        gpio_set_level(stepper->enbl_gpio, LOW);

        if (stepper->step_state == LOW) {
            --stepper->current_steps;
        }
    }
    else if (joystick_raw > 2477) {
        gpio_set_level(stepper->dir_gpio, RIGHT_DIR);
        gpio_set_level(stepper->enbl_gpio, LOW);

        if (stepper->step_state == LOW) {
            ++stepper->current_steps;
        }
    }
    else {
        gpio_set_level(stepper->enbl_gpio, HIGH);
    }
#else
    if (stepper->step_state == LOW) {
        --stepper->steps_remaining;
        if (stepper->steps_remaining == 0) {
            gptimer_stop(stepper->timer);
            gpio_set_level(stepper->enbl_gpio, HIGH);
            xEventGroupSetBitsFromISR(stepper_event_group, stepper->event_group_bit, NULL);
        }
    }
#endif
    return false;
}

void stepper_init(struct xyp_stepper* stepper) {
    stepper->steps_remaining = 0;
    stepper->step_state = LOW;
    stepper->current_board_pos = 0;
    
    // Set GPIOs to output
    gpio_set_direction(stepper->dir_gpio, GPIO_MODE_OUTPUT);
    gpio_set_direction(stepper->step_gpio, GPIO_MODE_OUTPUT);
    gpio_set_direction(stepper->enbl_gpio, GPIO_MODE_OUTPUT);

    // Default off
    gpio_set_level(stepper->enbl_gpio, HIGH);

    // Create timer
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = TIMER_RESOLUTION,
    };
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &stepper->timer));

    // Add alarm
    gptimer_alarm_config_t alarm_config = {
        .reload_count = 0,
        .alarm_count = ALARM_COUNT,
        .flags.auto_reload_on_alarm = true,
    };
    ESP_ERROR_CHECK(gptimer_set_alarm_action(stepper->timer, &alarm_config));

    gptimer_event_callbacks_t alarm_cb = {
        .on_alarm = move_timer_callback,
    };
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(stepper->timer, &alarm_cb, NULL));
    ESP_ERROR_CHECK(gptimer_enable(stepper->timer));

#ifdef XYP_JOYSTICK_TEST
    adc2_config_channel_atten(stepper->joystick_channel, ADC_ATTEN_DB_11);
    stepper->current_steps = 0;
#endif
}

void stepper_set_board_pos(struct xyp_stepper* stepper, const float pos) {
    if (pos < stepper->current_board_pos) {
        gpio_set_level(stepper->enbl_gpio, LOW);
        gpio_set_level(stepper->dir_gpio, LEFT_DIR);
    }
    else if (pos > stepper->current_board_pos) {
        gpio_set_level(stepper->enbl_gpio, LOW);
        gpio_set_level(stepper->dir_gpio, RIGHT_DIR);
    }
    else {
        // Already at pos
        xEventGroupSetBits(stepper_event_group, stepper->event_group_bit);
        return;
    }

    float current_inches = stepper->current_board_pos * SQUARE_SIZE;
    float target_inches = pos * SQUARE_SIZE;
    uint32_t steps = (uint32_t)(INCHES_TO_STEPS((fabs(current_inches - target_inches))));
    stepper->steps_remaining = steps;
    stepper->current_board_pos = pos;
    if(steps == 0) {
        xEventGroupSetBits(stepper_event_group, stepper->event_group_bit);
        return;
    }
    gptimer_start(stepper->timer);
}

void xyp_init() {
    memset(&x_stepper, 0, sizeof(x_stepper));
    memset(&y_stepper, 0, sizeof(y_stepper));

    gpio_config_t xy_enable_gpio_config;
    xy_enable_gpio_config.pin_bit_mask = 1ULL << X_ENBL_GPIO | 1ULL << Y_ENBL_GPIO;
    xy_enable_gpio_config.mode = GPIO_MODE_OUTPUT;
    xy_enable_gpio_config.pull_up_en = GPIO_PULLUP_ENABLE;
    xy_enable_gpio_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    xy_enable_gpio_config.intr_type = GPIO_INTR_DISABLE;
    ESP_ERROR_CHECK(gpio_config(&xy_enable_gpio_config));

    x_stepper.dir_gpio = X_DIR_GPIO;
    x_stepper.step_gpio = X_STEP_GPIO;
    x_stepper.event_group_bit = X_BIT;
    x_stepper.enbl_gpio = X_ENBL_GPIO;
    y_stepper.dir_gpio = Y_DIR_GPIO;
    y_stepper.step_gpio = Y_STEP_GPIO;
    y_stepper.event_group_bit = Y_BIT;
    y_stepper.enbl_gpio = Y_ENBL_GPIO;

#ifdef XYP_JOYSTICK_TEST
    x_stepper.joystick_channel = ADC2_CHANNEL_8;
    y_stepper.joystick_channel = ADC2_CHANNEL_9;
#endif

    stepper_init(&x_stepper);
    stepper_init(&y_stepper);

    stepper_event_group = xEventGroupCreate();
    xy_calibration_event_group = xEventGroupCreate();
}

void xyp_set_board_pos(const float x_pos, const float y_pos) {
    stepper_set_board_pos(&x_stepper, x_pos);
    stepper_set_board_pos(&y_stepper, y_pos);
    xEventGroupWaitBits(stepper_event_group, x_stepper.event_group_bit | y_stepper.event_group_bit, pdTRUE, pdTRUE, portMAX_DELAY);
}

// calibration timer event callback
bool calibration_timer_callback(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx) {
    struct xyp_stepper* stepper = NULL;
    if (current_axis == X_AXIS) {
        stepper = &x_stepper;
    }
    else if (current_axis == Y_AXIS) {
        stepper = &y_stepper;
    }
    else {
        return false;
    }

    stepper->step_state = !stepper->step_state;
    gpio_set_level(stepper->step_gpio, stepper->step_state);
    if (stepper->step_state == LOW) {
        if(current_move == TIMED_MOVE) {
            --stepper->steps_remaining;
            if (stepper->steps_remaining == 0) {
                gptimer_stop(timer);
                xEventGroupSetBitsFromISR(xy_calibration_event_group, CAL_BIT, NULL);
            }
        }
        else if(current_move == LIMIT_MOVE){
            if(!gpio_get_level(current_switch)) {
                gptimer_stop(timer);
                xEventGroupSetBitsFromISR(xy_calibration_event_group, CAL_BIT, NULL);
            }
        }
    }

    return false;
}

void ensure_no_limit_switch(gptimer_handle_t timer) {
    if(!gpio_get_level(LIMITX_GPIO)) {
        current_axis = X_AXIS;
        gpio_set_level(x_stepper.dir_gpio, RIGHT_DIR);
        x_stepper.steps_remaining = INCHES_TO_STEPS(2);
        gpio_set_level(x_stepper.enbl_gpio, LOW);
        gptimer_start(timer);
        xEventGroupWaitBits(xy_calibration_event_group, CAL_BIT, pdTRUE, pdTRUE, portMAX_DELAY);
        gpio_set_level(x_stepper.enbl_gpio, HIGH);
    }
    if(!gpio_get_level(LIMITY_GPIO)) {
        current_axis = Y_AXIS;
        gpio_set_level(y_stepper.dir_gpio, LEFT_DIR);
        y_stepper.steps_remaining = INCHES_TO_STEPS(2);
        gpio_set_level(y_stepper.enbl_gpio, LOW);
        gptimer_start(timer);
        xEventGroupWaitBits(xy_calibration_event_group, CAL_BIT, pdTRUE, pdTRUE, portMAX_DELAY);
        gpio_set_level(y_stepper.enbl_gpio, HIGH);
    }
}

void xyp_calibrate(){ 
    gptimer_handle_t cal_timer;

    // create local timer for calibration speed
    gptimer_config_t cal_timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = TIMER_RESOLUTION,
        .flags.intr_shared = 0,
    };
    ESP_ERROR_CHECK(gptimer_new_timer(&cal_timer_config, &cal_timer));

    // add alarm
    gptimer_alarm_config_t cal_alarm_config = {
        .reload_count = 0,
        .alarm_count = CAL_ALARM_COUNT,
        .flags.auto_reload_on_alarm = true,
    };
    ESP_ERROR_CHECK(gptimer_set_alarm_action(cal_timer, &cal_alarm_config));

    // add alarm callback
    gptimer_event_callbacks_t cal_alarm_cb = {
        .on_alarm = calibration_timer_callback,
    };
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(cal_timer, &cal_alarm_cb, NULL));
    ESP_ERROR_CHECK(gptimer_enable(cal_timer));

    // first check limit switches and make sure none are pressed
    current_move = TIMED_MOVE;
    ensure_no_limit_switch(cal_timer);
    
    // move x to limit switch
    current_move = LIMIT_MOVE;
    current_axis = X_AXIS;
    current_switch = LIMITX_GPIO;
    gpio_set_level(x_stepper.dir_gpio, LEFT_DIR);
    gpio_set_level(x_stepper.enbl_gpio, LOW);
    gptimer_start(cal_timer);
    xEventGroupWaitBits(xy_calibration_event_group, CAL_BIT, pdTRUE, pdTRUE, portMAX_DELAY);
    gpio_set_level(x_stepper.enbl_gpio, HIGH);
    x_stepper.current_board_pos = X_LIMIT_OFFSET_SQUARES; // setting x position
    
    // move y to limit switch
    current_move = LIMIT_MOVE;
    current_axis = Y_AXIS;
    current_switch = LIMITY_GPIO;
    gpio_set_level(y_stepper.dir_gpio, RIGHT_DIR);
    gpio_set_level(y_stepper.enbl_gpio, LOW);
    gptimer_start(cal_timer);
    xEventGroupWaitBits(xy_calibration_event_group, CAL_BIT, pdTRUE, pdTRUE, portMAX_DELAY);
    gpio_set_level(y_stepper.enbl_gpio, HIGH);
    y_stepper.current_board_pos = 8 + Y_LIMIT_OFFSET_SQUARES; // setting y position


    xyp_return_home();

    // disable timer to free its resources
    ESP_ERROR_CHECK(gptimer_disable(cal_timer));

}

void xyp_return_home(){
    xyp_set_board_pos(4.5, 4.5);
}

#ifdef XYP_JOYSTICK_TEST
void xyp_joystick_control() {
    gptimer_start(x_stepper.timer);
    gptimer_start(y_stepper.timer);

    for (;; ) {
        printf("X: %ld, Y: %ld\n", x_stepper.current_steps, y_stepper.current_steps);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
#endif
piece_color_t get_piece_color(int piece) {
    if(piece == 0) {
        return NONE;
    } else if(piece < 7) {
        return WHITE;
    } else {
        return BLACK;
    }
}

float get_half_y_step(float prev_y_cord) {
    if(prev_y_cord < 4.5) {
        return prev_y_cord + 0.5;
    } else {
        return prev_y_cord - 0.5;
    }
}

int map_x_to_board_state(int cur_x, int cur_y) {
    cur_x -= 1;
    return abs(cur_y - 8);
}

int map_y_to_board_state(int cur_x, int cur_y) {
    cur_y -= 1;
    return cur_x - 1;
}

void add_target_square(struct move_sequence * sequence, float target_x, float target_y, piece_color_t color) {
    sequence->squares_to_move[sequence->num_moves].target_x_cord = target_x;
    sequence->squares_to_move[sequence->num_moves].target_y_cord = target_y;
    sequence->squares_to_move[sequence->num_moves].target_emag_status = color;

    sequence->num_moves++;
}

void xyp_generate_moves(struct move_sequence * sequence, board_state_t* board_state, const move_type_t move_type, char * move_to_make) {
    sequence->num_moves = 0;

    float prev_x_cord = (float)(move_to_make[0] - 'a' + 1);
    float prev_y_cord = (float)(move_to_make[1] - '0');
    float goal_x_cord = (float)(move_to_make[2] - 'a' + 1);
    float goal_y_cord = (float)(move_to_make[3] - '0');

    printf("%f %f | %f %f\n", prev_x_cord, prev_y_cord, goal_x_cord, goal_y_cord);
    int dest_x = map_x_to_board_state(goal_x_cord, goal_y_cord);
    int dest_y = map_y_to_board_state(goal_x_cord, goal_y_cord);
    int goal_square_status = board_state_get_piece_on_square(board_state, dest_x, dest_y);
    board_state_print();
    printf("DEST: %d %d\n", dest_x, dest_y);
    // if there is soemthing on target square, campture has taken place
    if(goal_square_status != NP) {  
        printf("PIECE CAPTURE\n");
        piece_color_t capture_piece_color = get_piece_color(goal_square_status);

        // move to captured piece
        add_target_square(sequence, goal_x_cord, goal_y_cord, NONE);

        // grab captured piece and move to between squares
        add_target_square(sequence, goal_x_cord, get_half_y_step(goal_y_cord), capture_piece_color);
        
        // move captured piece to side of board
        if(goal_x_cord < 4.5) {
            add_target_square(sequence, 0.25, get_half_y_step(goal_y_cord), capture_piece_color);
        } else {
            add_target_square(sequence, 8.75, get_half_y_step(goal_y_cord), capture_piece_color);
        }
    }

    // move to piece that was played
    add_target_square(sequence, prev_x_cord, prev_y_cord, NONE);

    dest_x = map_x_to_board_state(prev_x_cord, prev_y_cord);
    dest_y = map_y_to_board_state(prev_x_cord, prev_y_cord);
    int played_piece = board_state_get_piece_on_square(board_state, dest_x, dest_y);
    piece_color_t played_piece_color = get_piece_color(played_piece);

    if(move_type == EN_PASSANT) {
                // move played pawn
        printf("EN PASSANT\n");
        add_target_square(sequence, goal_x_cord, goal_y_cord, played_piece_color);

        // move to captured pawn
        add_target_square(sequence, goal_x_cord, prev_y_cord, NONE);

        // grab captured pawn and move to between squares
        piece_color_t captured_pawn_color;
        if(played_piece_color == WHITE) {
            captured_pawn_color = BLACK;
        } else {
            captured_pawn_color = WHITE;
        }
        add_target_square(sequence, goal_x_cord, get_half_y_step(prev_y_cord), captured_pawn_color);

        // move captured pawn off to side of board
        if(goal_x_cord < 4.5) {
            add_target_square(sequence, 0.25, get_half_y_step(prev_y_cord), captured_pawn_color);
        } else {
            add_target_square(sequence, 8.75, get_half_y_step(prev_y_cord), captured_pawn_color);
        }

        return;
    }

    if(move_type == CASTLE) {
        // move king
        printf("CASTLE\n");
        add_target_square(sequence, goal_x_cord, goal_y_cord, played_piece_color);

        // move to rook
        float rook_x;
        float rook_goal;
        if(goal_x_cord < prev_x_cord) {
            // king moving left, rook is on left of king, will end up right of king
            rook_x = goal_x_cord - 2;
            rook_goal = goal_x_cord + 1;
        } else {
            rook_x = goal_x_cord + 1;
            rook_goal = goal_x_cord - 1;
        }
        add_target_square(sequence, rook_x, goal_y_cord, NONE);

        // grab rook and move to between squares
        add_target_square(sequence, rook_x, get_half_y_step(goal_y_cord), played_piece_color);

        // move rook to other side of king
        add_target_square(sequence, rook_goal, get_half_y_step(goal_y_cord), played_piece_color);
        add_target_square(sequence, rook_goal, goal_y_cord, played_piece_color);

        return;
    }

    // knights can jump...
    if(played_piece == WN || played_piece == BN) {
        printf("KNIGHT MOVE\n");
        float half_step;
        if(fabs(goal_x_cord - prev_x_cord) == 1) {
            if(prev_x_cord > goal_x_cord) {
                half_step = prev_x_cord - 0.5;
            } else {
                half_step = prev_x_cord + 0.5;
            }
            add_target_square(sequence, half_step, prev_y_cord, played_piece_color);
            add_target_square(sequence, half_step, goal_y_cord, played_piece_color);
        } else {
            if(prev_y_cord > goal_y_cord) {
                half_step = prev_y_cord - 0.5;
            } else {
                half_step = prev_y_cord + 0.5;
            }
            add_target_square(sequence, prev_x_cord, half_step, played_piece_color);
            add_target_square(sequence, goal_x_cord, half_step, played_piece_color);
        }
    } 

    // move to final position
    add_target_square(sequence, goal_x_cord, goal_y_cord, played_piece_color);
    
    return;
}

void xyp_play_move(struct move_sequence* sequence) {
    printf("Moving sequence:\n");
    for (uint8_t i = 0; i < sequence->num_moves; ++i) {
        printf("Moving to X: %f, Y: %f, EM: %d\n", sequence->squares_to_move[i].target_x_cord, sequence->squares_to_move[i].target_y_cord, sequence->squares_to_move[i].target_emag_status);
        electromagnet_set(sequence->squares_to_move[i].target_emag_status);
        xyp_set_board_pos(sequence->squares_to_move[i].target_x_cord, sequence->squares_to_move[i].target_y_cord);
    }

    electromagnet_off();
    xyp_return_home();
}
