#include <stdio.h>
#include "score_display.h"
void app_main(void)
{
    gpio_set_direction(DB4,GPIO_MODE_OUTPUT);          //Set DB4 as output
    gpio_set_direction(DB5,GPIO_MODE_OUTPUT);          //Set DB5 as output
    gpio_set_direction(DB6,GPIO_MODE_OUTPUT);          //Set DB6 as output
    gpio_set_direction(DB7,GPIO_MODE_OUTPUT);          //Set DB7 as output
    gpio_set_direction(RS,GPIO_MODE_OUTPUT);         //Set RS  as output                
    gpio_set_direction(E,GPIO_MODE_OUTPUT);         //Set E   as output

    // gpio_set_level(DB4, LOW);
    // gpio_set_level(DB5, LOW);
    // gpio_set_level(DB6, LOW);
    // gpio_set_level(DB7, LOW);
    // gpio_set_level(RS, LOW);
  
    init();
    CharLCD_Chess_Setup("Aditya", "Raijin", "USA", "USA", "3454", "3453");
	// while(1){
    //     // vTaskDelay(pdMS_TO_TICKS(1000));
	// 	// write('!');
    //     // printf("Printed !\n");
	// 	// vTaskDelay(pdMS_TO_TICKS(1000));
	// }
}
