#include "Buttons.h"
#include "lichess_api.h"

bool game_created = false;
bool make_move = false;
void buttons_init(){
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    //bit mask of the pins, use GPIO4/5 here
    io_conf.pin_bit_mask = 18;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = 1;
    io_conf.pin_bit_mask = 1835272;
    gpio_config(&io_conf);
    // io_conf.pin_bit_mask = 19;
    // gpio_config(&io_conf);
    // io_conf.pin_bit_mask = 20;
    // gpio_config(&io_conf);
    // io_conf.pin_bit_mask = 3;
    // gpio_config(&io_conf);   
    // gpio_install_isr_service(ESP_INTR_FLAG_LEVEL3);
    // gpio_isr_handler_add(8, resign_button, (void*) 0);
    // gpio_isr_handler_add(18, draw_button, (void*) 0);
    // gpio_isr_handler_add(19, make_game_button, (void*) 0);
    // // 1 is making game as black
    // gpio_isr_handler_add(20, make_game_button, (void*) 1);
    // gpio_isr_handler_add(3, clock_button, (void*) 0);
    printf("Button init finished\n");
}

void draw_button(void * arg){
    xSemaphoreGiveFromISR(xSemaphore_Draw, NULL);
}

void resign_button(void * arg){
    xSemaphoreGiveFromISR(xSemaphore_Resign, NULL);
}

void make_game_button(void * arg){
    xSemaphoreGiveFromISR(xSemaphore, NULL);
}

void clock_button(void * arg){
    xSemaphoreGiveFromISR(xSemaphore_MakeMove, NULL);    
}
