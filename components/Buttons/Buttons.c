#include "Buttons.h"

extern int test;

void buttons_init(){
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_POSEDGE;
    //bit mask of the pins, use GPIO4/5 here
    io_conf.pin_bit_mask = 18;
    //set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    //enable pull-up mode
    io_conf.pull_up_en = 1;
    // gpio_config(&io_conf);
    io_conf.pin_bit_mask = 256;
    gpio_config(&io_conf);
    // io_conf.pin_bit_mask = 19;
    // gpio_config(&io_conf);
    // io_conf.pin_bit_mask = 20;
    // gpio_config(&io_conf);
    // io_conf.pin_bit_mask = 3;
    // gpio_config(&io_conf);
    gpio_install_isr_service(ESP_INTR_FLAG_LEVEL3);
    gpio_isr_handler_add(8, resign_button, (void*) 0);
}

void IRAM_ATTR draw_button(void * arg){

}

void IRAM_ATTR resign_button(void * arg){
    test++;
}

void IRAM_ATTR make_game_button(void * arg){

}

void IRAM_ATTR clock_button(void * arg){

}