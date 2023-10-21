#include <stdio.h>
#include "score_display.h"
void app_main(void)
{
    // gpio_set_level(DB4, LOW);
    // gpio_set_level(DB5, LOW);
    // gpio_set_level(DB6, LOW);
    // gpio_set_level(DB7, LOW);
    // gpio_set_level(RS, LOW);
  
    init();
    CharLCD_Chess_Setup("Aditya", "Raijin", "Texas", "Holland", "3454", "3453");
    CharLCD_OfferDraw(true);
    CharLCD_DrawStatus(false);
    CharLCD_WinUpdate("22","0");
	// while(1){
    //     // vTaskDelay(pdMS_TO_TICKS(1000));
	// 	// write('!');
    //     // printf("Printed !\n");
	// 	// vTaskDelay(pdMS_TO_TICKS(1000));
	// }
}
