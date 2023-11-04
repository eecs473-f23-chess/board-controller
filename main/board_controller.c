#include <stdio.h>
#include "Hall_Effect.h"

char board[8][8];

void app_main(void)
{
    ADC_setup();
    int reading;

    while(1){

        for(int i = 0; i < 2; i++){
            printf("Line %d:", i);
            for(int j = 0; j < 8; j++){
                select_xy_sensor(i,j);
                reading = Get_Magnetic();
                printf(" %d ", reading);
                vTaskDelay(pdMS_TO_TICKS(5));
            }
            printf("\n");
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
        printf("\n");
        
    }

}
