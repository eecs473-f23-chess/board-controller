#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include <stdio.h>
#include "freertos/timers.h"

#define RES 8 // Reset signal
#define CS 9  // Chip select signal
#define RS 10 // Register select signal
#define SC 11 // Serial clock signal J3 19
#define SI 12 // Serial data signal
#define LOW 0
#define HIGH 1

void GraphicLCD_data_write(unsigned char d);
void GraphicLCD_comm_write(unsigned char d);
void GraphicLCD_DispPic(unsigned char *lcd_string);
void GraphicLCD_ClearLCD(unsigned char *lcd_string);
void GraphicLCD_init_LCD();
