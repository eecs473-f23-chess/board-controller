#include <stdio.h>
#include "score_display.h"
void app_main(void)
{
    // gpio_set_level(DB4, LOW);
    // gpio_set_level(DB5, LOW);
    // gpio_set_level(DB6, LOW);
    // gpio_set_level(DB7, LOW);
    // gpio_set_level(RS, LOW);
    
    // gpio_set_direction(48, GPIO_MODE_OUTPUT);

    // while(1){
    //     gpio_set_level(48,0);
    //     vTaskDelay(pdMS_TO_TICKS(1000));
    //     gpio_set_level(48,1);
    //     vTaskDelay(pdMS_TO_TICKS(1000));
    // }


    init();
    CharLCD_Chess_Setup("Adityalalalalalalala", "Rajinlalalalalala", "Yugoslavia", "Australia", "3454", "3");
    CharLCD_OfferDraw(true);
    CharLCD_DrawStatus(false);
    CharLCD_WinUpdate("22","0");
    // gpio_set_level(48, 0);
	// while(1){
    //     // vTaskDelay(pdMS_TO_TICKS(1000));
	// 	// write('!');
    //     // printf("Printed !\n");
	// 	// vTaskDelay(pdMS_TO_TICKS(1000));
	// }
}
