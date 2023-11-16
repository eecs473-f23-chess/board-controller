#include "Buttons.h"
#include "lichess_api.h"

bool making_game = false;
<<<<<<< HEAD

<<<<<<< HEAD

extern int test;
extern bool make_game = false;

void buttons_init(){
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_POSEDGE;
=======

=======
>>>>>>> 6f44d07 (Create game and resign button work in progress)
void buttons_init(){
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    //bit mask of the pins, use GPIO4/5 here
>>>>>>> 4247ee6 (Make game seems to work, others in progress)
    io_conf.pin_bit_mask = 18;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = 1;
    io_conf.pin_bit_mask = 1835272;
    gpio_config(&io_conf);
<<<<<<< HEAD
=======
    // io_conf.pin_bit_mask = 19;
    // gpio_config(&io_conf);
    // io_conf.pin_bit_mask = 20;
    // gpio_config(&io_conf);
    // io_conf.pin_bit_mask = 3;
    // gpio_config(&io_conf);   
>>>>>>> 6f44d07 (Create game and resign button work in progress)
    gpio_install_isr_service(ESP_INTR_FLAG_LEVEL3);
    gpio_isr_handler_add(8, resign_button, (void*) 0);
    gpio_isr_handler_add(3, draw_button, (void*) 0);
    // 0 is making game as white
    gpio_isr_handler_add(19, make_game_button, (void*) 0);
    // 1 is making game as black
    gpio_isr_handler_add(20, make_game_button, (void*) 1);
    gpio_isr_handler_add(18, clock_button, (void*) 0);
    printf("Button init finished\n");
}

void IRAM_ATTR draw_button(void * arg){
    xSemaphoreGiveFromISR(xSemaphore_Draw, NULL);
}

void IRAM_ATTR resign_button(void * arg){
    xSemaphoreGiveFromISR(xSemaphore_Resign, NULL);
}

void IRAM_ATTR make_game_button(void * arg){
<<<<<<< HEAD
    int button = (int)arg;
    if(button == 1){
        make_game = true;
    }
    else if(button == 0){
        test++;
    }

=======
    if(making_game == false){
        making_game = true;
        xSemaphoreGiveFromISR(xSemaphore, NULL);
    }    
>>>>>>> 4247ee6 (Make game seems to work, others in progress)
}

void IRAM_ATTR clock_button(void * arg){
    xSemaphoreGiveFromISR(xSemaphore_MakeMove, NULL);
}