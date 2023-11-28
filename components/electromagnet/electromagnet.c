#include "electromagnet.h"

#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <stdio.h>

#define LOW 0
#define HIGH 1

#define ELECMAG_SELA_GPIO   16
#define ELECMAG_SELB_GPIO   17

#ifdef ELECMAG_BUTTON_TEST
#define ELECMAG_WHITE_GPIO  18
#define ELECMAG_BLACK_GPIO  3
#define ELECMAG_OFF_GPIO    8
#endif

#ifdef ELECMAG_BUTTON_TEST
void isr_white_handler(void* args) {
    electromagnet_on(WHITE);
}
void isr_black_handler(void* args) {
    electromagnet_on(BLACK);
}
void isr_off_handler(void* args) {
    electromagnet_off();
}
#endif

void electromag_init() {
    gpio_set_direction((gpio_num_t)ELECMAG_SELA_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction((gpio_num_t)ELECMAG_SELB_GPIO, GPIO_MODE_OUTPUT); 

    electromagnet_off();

#ifdef ELECMAG_BUTTON_TEST
    gpio_config_t button_conf = {
        .pin_bit_mask = (1ULL << ELECMAG_WHITE_GPIO) | (1ULL << ELECMAG_BLACK_GPIO) | (1ULL << ELECMAG_OFF_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };
    gpio_config(&button_conf);
    gpio_install_isr_service(ESP_INTR_FLAG_LEVEL3);
    gpio_isr_handler_add(ELECMAG_WHITE_GPIO, isr_white_handler, NULL);
    gpio_isr_handler_add(ELECMAG_BLACK_GPIO, isr_black_handler, NULL);
    gpio_isr_handler_add(ELECMAG_OFF_GPIO, isr_off_handler, NULL);
#endif
}

void electromagnet_on(piece_color_t color) {
    if(color == WHITE) {
        gpio_set_level((gpio_num_t)ELECMAG_SELA_GPIO, HIGH);
        gpio_set_level((gpio_num_t)ELECMAG_SELB_GPIO, LOW);
    } 
    else if (color == BLACK) {
        gpio_set_level((gpio_num_t)ELECMAG_SELA_GPIO, LOW);
        gpio_set_level((gpio_num_t)ELECMAG_SELB_GPIO, HIGH);
    }
}

void electromagnet_off() {
    gpio_set_level((gpio_num_t)ELECMAG_SELA_GPIO, LOW);
    gpio_set_level((gpio_num_t)ELECMAG_SELB_GPIO, LOW);
}
