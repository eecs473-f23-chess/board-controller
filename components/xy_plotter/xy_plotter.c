#include "xy_plotter.h"

#include <driver/gpio.h>
#include <driver/gptimer.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>
#include <string.h>
#include <esp32s3/rom/ets_sys.h>

// GPIOs
#define X_DIR_GPIO              0
#define X_STEP_GPIO             35
#define Y_DIR_GPIO              36
#define Y_STEP_GPIO             37
#define LIMITL_GPIO             41 
#define LIMITR_GPIO             42 

// Alarm/timer config
#define TIMER_RESOLUTION        1e6 // 1Mhz
#define ALARM_PERIOD            1e-3 // 1ms 
#define ALARM_COUNT             TIMER_RESOLUTION * ALARM_PERIOD
#define CAL_ALARM_COUNT         ALARM_COUNT 

// Stepper motors
#define STEPS_PER_REVOLUTION    200
#define PULLEY_DIAMETER         0.4811 // inches
#define INCHES_PER_REVOLUTION   PULLEY_DIAMETER * 3.14159
#define INCHES_TO_STEPS(INCHES) (((INCHES) / (INCHES_PER_REVOLUTION)) * (STEPS_PER_REVOLUTION))

// Chess board
#define SQUARE_SIZE             2.5 // inches
#define INCHES_LENGTH_X         23.5    // actual length of x side in inches
#define INCHES_LENGTH_Y         22.5    // actual length of y side in inches
#define X_OFFSET_INCHES         3
#define Y_OFFSET_INCHES         1.25
#define X_OFFSET_STEPS          INCHES_TO_STEPS(X_OFFSET_INCHES)    
#define Y_OFFSET_STEPS          INCHES_TO_STEPS(Y_OFFSET_INCHES)
uint32_t STEPS_LENGTH_X;
uint32_t STEPS_LENGTH_Y;

// Event group bits
#define X_BIT                   BIT0
#define Y_BIT                   BIT1
#define CAL_BIT                 BIT2

#define HIGH                    1
#define LOW                     0

// direction gpio levels
#define LEFT_DIR    1
#define RIGHT_DIR   0

struct xyp_stepper {
    gptimer_handle_t timer;
    gpio_num_t dir_gpio;
    gpio_num_t step_gpio;
    float current_board_pos; // in units of squares
    uint32_t steps_remaining;
    uint8_t step_state;
    uint8_t event_group_bit;
    uint8_t steps_per_inch;
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


bool on_alarm_event(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx) {
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
    if (stepper->step_state == LOW) {
        --stepper->steps_remaining;
        if (stepper->steps_remaining == 0) {
            gptimer_stop(stepper->timer);
            xEventGroupSetBitsFromISR(stepper_event_group, stepper->event_group_bit, NULL);
        }
    }

    return true;
}

void stepper_init(struct xyp_stepper* stepper) {
    stepper->steps_remaining = 0;
    stepper->step_state = LOW;
    
    // Set GPIOs to output
    gpio_set_direction(stepper->dir_gpio, GPIO_MODE_OUTPUT);
    gpio_set_direction(stepper->step_gpio, GPIO_MODE_OUTPUT);

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
        .on_alarm = on_alarm_event,
    };
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(stepper->timer, &alarm_cb, NULL));
    ESP_ERROR_CHECK(gptimer_enable(stepper->timer));
}

void stepper_set_board_pos(struct xyp_stepper* stepper, const float pos) {
    if (pos < stepper->current_board_pos) {
        gpio_set_level(stepper->dir_gpio, LEFT_DIR);
    }
    else if (pos > stepper->current_board_pos) {
        gpio_set_level(stepper->dir_gpio, RIGHT_DIR);
    }
    else {
        // Already at pos
        printf("already at pos...\n");
        xEventGroupSetBits(stepper_event_group, stepper->event_group_bit);
        return;
    }

    float current_inches = stepper->current_board_pos * SQUARE_SIZE;
    float target_inches = pos * SQUARE_SIZE;
    uint32_t steps = abs(current_inches - target_inches)*stepper->steps_per_inch;
    stepper->steps_remaining = steps;
    stepper->current_board_pos = pos;
    gptimer_start(stepper->timer);
}

void xyp_init() {
    memset(&x_stepper, 0, sizeof(x_stepper));
    memset(&y_stepper, 0, sizeof(y_stepper));

    x_stepper.dir_gpio = X_DIR_GPIO;
    x_stepper.step_gpio = X_STEP_GPIO;
    x_stepper.event_group_bit = X_BIT;
    y_stepper.dir_gpio = Y_DIR_GPIO;
    y_stepper.step_gpio = Y_STEP_GPIO;
    y_stepper.event_group_bit = Y_BIT;

    stepper_init(&x_stepper);
    stepper_init(&y_stepper);

    stepper_event_group = xEventGroupCreate();
    xy_calibration_event_group = xEventGroupCreate();
}

void xyp_set_board_pos(const float x_pos, const float y_pos) {
    stepper_set_board_pos(&x_stepper, x_pos);
    stepper_set_board_pos(&y_stepper, y_pos);
    xEventGroupWaitBits(stepper_event_group, y_stepper.event_group_bit, pdTRUE, pdTRUE, portMAX_DELAY);
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
            ++stepper->steps_remaining;
            if(!gpio_get_level(current_switch)) {
                ets_printf("limit switch hit, stopping timer\n");
                gptimer_stop(timer);
                ets_printf("returning from isr\n");
                xEventGroupSetBitsFromISR(xy_calibration_event_group, CAL_BIT, NULL);
            }
        }
    }

    return true;
}

void ensure_no_limit_switch(gptimer_handle_t timer) {
    
}

/**
 * Changes XY plotter speed to slower calibration speed
 * Ensures XY plotter is not hitting any limit switches
 * Calibrates axes by hitting right limit switch then left to adjust ticks for full working area
 * Sets XY plotter home position/ (0,0) as bottom left corner (square A1)
*/
void xyp_calibrate(){ 
    gptimer_handle_t cal_timer;

    // Create timer
    gptimer_config_t cal_timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = TIMER_RESOLUTION,
        .flags.intr_shared = 0,

    };
    ESP_ERROR_CHECK(gptimer_new_timer(&cal_timer_config, &cal_timer));

    // Add alarm
    gptimer_alarm_config_t cal_alarm_config = {
        .reload_count = 0,
        .alarm_count = CAL_ALARM_COUNT,
        .flags.auto_reload_on_alarm = true,
    };
    ESP_ERROR_CHECK(gptimer_set_alarm_action(cal_timer, &cal_alarm_config));

    gptimer_event_callbacks_t alarm_cb = {
        .on_alarm = calibration_timer_callback,
    };
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(cal_timer, &alarm_cb, NULL));
    ESP_ERROR_CHECK(gptimer_enable(cal_timer));

    printf("finished cal 'init', timer/event group made\n");

    // first check limit switches and make sure none are pressed
    current_move = TIMED_MOVE;
    ensure_no_limit_switch(cal_timer);
    printf("no limit switches hitting!\n");
    
    // reset x axis
    printf("resetting x axis to left limit switch\n");
    current_move = LIMIT_MOVE;
    current_axis = X_AXIS;
    current_switch = LIMITL_GPIO;
    gpio_set_level(x_stepper.dir_gpio, LEFT_DIR);
    gptimer_start(cal_timer);
    xEventGroupWaitBits(xy_calibration_event_group, CAL_BIT, pdTRUE, pdTRUE, portMAX_DELAY);
    printf("finished x axis reset\n");

    // calibrate x axis
    printf("calibrating x axis with right limit switch\n");
    x_stepper.steps_remaining = 0;
    current_switch = LIMITR_GPIO;
    gpio_set_level(x_stepper.dir_gpio, RIGHT_DIR);
    gptimer_start(cal_timer);
    xEventGroupWaitBits(xy_calibration_event_group, CAL_BIT, pdTRUE, pdTRUE, portMAX_DELAY);
    printf("finished calibration\n");
    STEPS_LENGTH_X = x_stepper.steps_remaining; // fill in global
    x_stepper.steps_per_inch = STEPS_LENGTH_X / INCHES_LENGTH_X; // fill in x stepper struct
    
    // move x to center
    printf("move x to center\n");
    x_stepper.current_board_pos = 9.2;
    stepper_set_board_pos(&x_stepper, 4.5);
    printf("finished moving x to center\n");
    
    // reset y axis
    printf("resetting y axis to left limit switch\n");
    current_move = LIMIT_MOVE;
    current_axis = Y_AXIS;
    current_switch = LIMITL_GPIO;
    gpio_set_level(y_stepper.dir_gpio, LEFT_DIR);
    gptimer_start(cal_timer);
    xEventGroupWaitBits(xy_calibration_event_group, CAL_BIT, pdTRUE, pdTRUE, portMAX_DELAY);
    printf("finished y axis reset\n");

    // calibrate y axis
    printf("calibrating y axis with right limit switch\n");
    y_stepper.steps_remaining = 0;
    current_switch = LIMITR_GPIO;
    gpio_set_level(y_stepper.dir_gpio, RIGHT_DIR);
    gptimer_start(cal_timer);
    xEventGroupWaitBits(xy_calibration_event_group, CAL_BIT, pdTRUE, pdTRUE, portMAX_DELAY);
    printf("finished calibrating\n");
    STEPS_LENGTH_Y = y_stepper.steps_remaining; // fill in global
    y_stepper.steps_per_inch = STEPS_LENGTH_Y / INCHES_LENGTH_Y; // fill in y stepper struct

    // move y to center
    printf("moving y to center\n");
    y_stepper.current_board_pos = 9;
    stepper_set_board_pos(&y_stepper, 4.5);
    printf("finished move y to center\n");

    // disable timer
    ESP_ERROR_CHECK(gptimer_disable(cal_timer));
}

void move_center(){

}
