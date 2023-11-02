#include "xy_plotter.h"

#include <driver/gpio.h>
#include <driver/gptimer.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>
#include <string.h>

// GPIOs
#define X_DIR_GPIO              0
#define X_STEP_GPIO             35
#define Y_DIR_GPIO              36
#define Y_STEP_GPIO             37

// Alarm/timer config
#define TIMER_RESOLUTION        1e6 // 1Mhz
#define ALARM_PERIOD            1e-3 // 1ms
#define ALARM_COUNT             TIMER_RESOLUTION * ALARM_PERIOD

// Stepper motors
#define STEPS_PER_REVOLUTION    200
#define PULLEY_DIAMETER         0.4811 // inches
#define INCHES_PER_REVOLUTION   PULLEY_DIAMETER * 3.14159
#define INCHES_TO_STEPS(INCHES) (((INCHES) / (INCHES_PER_REVOLUTION)) * (STEPS_PER_REVOLUTION))

// Chess board
#define SQUARE_SIZE             2.5 // inches

// Event group bits
#define X_BIT                   BIT0
#define Y_BIT                   BIT1

#define HIGH                    1
#define LOW                     0

struct xyp_stepper {
    gptimer_handle_t timer;
    gpio_num_t dir_gpio;
    gpio_num_t step_gpio;
    float current_board_pos;
    uint32_t steps_remaining;
    uint8_t step_state;
    uint8_t event_group_bit;
};

// Timers for step pulse generation
static struct xyp_stepper x_stepper;
static struct xyp_stepper y_stepper;

// Event group for stepper position reached
EventGroupHandle_t stepper_event_group;

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
        gpio_set_level(stepper->dir_gpio, LOW);
    }
    else if (pos > stepper->current_board_pos) {
        gpio_set_level(stepper->dir_gpio, HIGH);
    }
    else {
        // Already at pos
        xEventGroupSetBits(stepper_event_group, stepper->event_group_bit);
        return;
    }

    uint16_t current_inches = stepper->current_board_pos * SQUARE_SIZE;
    uint16_t target_inches = pos * SQUARE_SIZE;
    uint16_t steps = INCHES_TO_STEPS(abs(current_inches - target_inches));
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
}

void xyp_set_board_pos(const float x_pos, const float y_pos) {
    stepper_set_board_pos(&x_stepper, x_pos);
    stepper_set_board_pos(&y_stepper, y_pos);
    xEventGroupWaitBits(stepper_event_group, y_stepper.event_group_bit, pdTRUE, pdTRUE, portMAX_DELAY);
}
