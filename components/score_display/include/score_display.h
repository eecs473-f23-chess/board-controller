#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include <stdio.h>
#include "freertos/timers.h"
#include <string.h>
#include "freertos/task.h"

#define RS  (gpio_num_t)13  //13
#define E   (gpio_num_t)14 //6
#define LOW 0
#define HIGH 1
#define DB4 (gpio_num_t)21  //21
#define DB5 (gpio_num_t)47//47
#define DB6 (gpio_num_t)48 //48
#define DB7 (gpio_num_t)45 //45

void scoreboard_command(char i);
void scoreboard_write(char i);
void scoreboard_init();
void send_string(char str[]);
void scoreboard_SetLine(int line);
void scoreboard_SetLineEnd(int line, int strlen);
void scoreboard_clearline(int line);
void scoreboard_Chess_Setup(char name1[], char name2[], char country1[], char country2[], char rank1[], char rank2[]);
void scoreboard_OfferDraw(bool LtR);
void scoreboard_DrawDeclined();
void scoreboard_WinUpdate(char P1wins[], char P2wins[]);
void scoreboard_clear();
