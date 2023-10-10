#include <stdio.h>
#include "clock_display.h"

void app_main(void)
{
    gpio_set_direction((gpio_num_t)RES, GPIO_MODE_OUTPUT); // configure RES as output
    gpio_set_direction((gpio_num_t)CS, GPIO_MODE_OUTPUT);  // configure CS as output
    gpio_set_direction((gpio_num_t)RS, GPIO_MODE_OUTPUT);  // configure RS as output
    gpio_set_direction((gpio_num_t)SC, GPIO_MODE_OUTPUT);  // configure SC as output
    gpio_set_direction((gpio_num_t)SI, GPIO_MODE_OUTPUT);  // configure SI as output
    gpio_set_level((gpio_num_t)RES, LOW);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level((gpio_num_t)RES, HIGH);
    vTaskDelay(pdMS_TO_TICKS(100));
    GraphicLCD_init_LCD();

    vTaskDelay(pdMS_TO_TICKS(1000));
    int hours = 0;
    int tens_minutes = 1;
    int ones_minutes = 0;
    int tens_seconds = 0;
    int ones_seconds = 1;
    //While loop is for clock logic
    while(hours + tens_minutes +tens_seconds +ones_minutes + ones_seconds != 0){
        if(ones_seconds == 0){
            ones_seconds = 9;
            if(tens_seconds == 0){
                tens_seconds = 5;
                if(ones_minutes == 0){
                    ones_minutes = 9;
                    if(tens_minutes == 0 && hours !=0){
                        tens_minutes = 5;
                        hours -= 1;
                    }
                    else{
                        tens_minutes -= 1;
                    }
                    
                }
                else{
                    ones_minutes -= 1;
                }
            }
            else{
                tens_seconds -= 1;
            }
        }
        else{
            ones_seconds -=1;
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
        GraphicLCD_DispNHDPic(hours, tens_minutes, ones_minutes, tens_seconds, ones_seconds);
    }
}
