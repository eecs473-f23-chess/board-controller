#include "electromagnet.h"

#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <stdio.h>

#define LOW 0
#define HIGH 1

void electromag_init() {
    gpio_set_direction((gpio_num_t)ELECMAG_SELA, GPIO_MODE_OUTPUT);
    gpio_set_direction((gpio_num_t)ELECMAG_SELB, GPIO_MODE_OUTPUT); 

    electromagnet_off();
}

void electromagnet_on(piece_color_t color) {
    if(color == WHITE) {
        gpio_set_level((gpio_num_t)ELECMAG_SELA, HIGH);
        gpio_set_level((gpio_num_t)ELECMAG_SELB, LOW);
    } 
    else if (color == BLACK) {
        gpio_set_level((gpio_num_t)ELECMAG_SELA, LOW);
        gpio_set_level((gpio_num_t)ELECMAG_SELB, HIGH);
    }
}

void electromagnet_off() {
    gpio_set_level((gpio_num_t)ELECMAG_SELA, LOW);
    gpio_set_level((gpio_num_t)ELECMAG_SELB, LOW);
}
