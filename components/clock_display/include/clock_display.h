#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include <stdio.h>
#include "freertos/timers.h"


#define RES 4 // Reset signal
#define CS_C1 5  // Chip select signal
#define RS_C1 6 // Register select signal
#define SC 11 // Serial clock signal J3 19
#define SI 12 // Serial data signal
#define CS_C2 7  // Chip select signal
#define RS_C2 15 // Register select signal
#define LOW 0
#define HIGH 1

// unsigned char NHD[];
extern uint32_t white_time;
extern uint32_t black_time;
extern bool white_turn;
extern bool black_turn;

void GraphicLCD_data_write(unsigned char d, bool left);
void GraphicLCD_comm_write(unsigned char d, bool left);
void GraphicLCD_DispPic(unsigned char *lcd_string, bool left);
void GraphicLCD_ClearLCD(bool left);
void GraphicLCD_init_LCD();
void GraphicLCD_DispClock(int ms, bool left);
void decrement_time(void * pvParameters);