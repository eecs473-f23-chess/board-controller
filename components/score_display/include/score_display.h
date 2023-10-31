/*****************************************************************************
* Program for writing to Newhaven Display NHD-0216K1Z-FSW-FBW-L with ST7066U controller.
* This code is written for the Arduino Uno (ATmega328P) in 6800 Mode 8-Bit Parallel Interface
* 
* Newhaven Display invests time and resources providing this open source code,
* Please support Newhaven Display by purchasing products from Newhaven Display!
*
* Copyright 2020, Newhaven Display International, Inc.
*
* This code is provided as an example only and without any warranty by Newhaven Display. 
* Newhaven Display accepts no responsibility for any issues resulting from its use. 
* The developer of the final application incorporating any parts of this 
* sample code is responsible for ensuring its safe and correct operation
* and for any consequences resulting from its use.
* See the GNU General Public License for more details. 
*****************************************************************************/
/*     
  UNO           LCD   
-------------------------- 
  GND           1  (VSS)
  5V            2  (VDD)
                3  (V0)
  10 RS         4  (RS) 
  11 RW         5  (RW)
  12 E          6  (E)
  2             7  (DB0)
  3             8  (DB1)
  4             9  (DB2)
  5             10 (DB3)
  6             11 (DB4)
  7             12 (DB5)
  8             13 (DB6)
  9             14 (DB7)
  5V            15 (LED+)
  GROUND        16 (LED-)
*/

/*
Todo:
Find out which pins we are using and define them
Set up the chess interface (I will be assuming it works the same as the lab one)
Turn off cursor
Need to check if writing a char to the display shifts the cursor by one
Might need to rewrite the data and command send functions, check when we get the screen

*/


#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include <stdio.h>
#include "freertos/timers.h"
#include <string.h>
#include "freertos/task.h"

#define RS  (gpio_num_t)13  //13
#define E   (gpio_num_t)6 //6
#define LOW 0
#define HIGH 1
#define DB4 (gpio_num_t)21  //21
#define DB5 (gpio_num_t)47//47
#define DB6 (gpio_num_t)48 //48
#define DB7 (gpio_num_t)45 //45

//---------------------------------------------------------
/*
8_bit_character.c
Program for writing to Newhaven Display character LCD

(c) Newhaven Display International, Inc. 

 	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
*/
//---------------------------------------------------------
//---------------------------------------------------------
/*
#define E	 P3_4;
#define D_I	 P3_0;
#define R_W	 P3_7;
*/

void scoreboard_command(char i){
  gpio_set_level(E, HIGH);
  gpio_set_level(RS, LOW);

  // Need the left 4 bits
  const uint8_t top_four_masked = 240;
  uint8_t first_four_bits = i & top_four_masked;
  first_four_bits = first_four_bits >> 4; 
  
  gpio_set_level(DB4, first_four_bits & 1);  
  gpio_set_level(DB5, ((first_four_bits >> 1) & 1));  
  gpio_set_level(DB6, ((first_four_bits >> 2) & 1));  
  gpio_set_level(DB7, ((first_four_bits >> 3) & 1));
  
  gpio_set_level(E, HIGH);
  vTaskDelay(pdMS_TO_TICKS(3));
  gpio_set_level(E, LOW);
  vTaskDelay(pdMS_TO_TICKS(3));
 
  // Need the right 4 bits  

  const uint8_t low_masked = 15;
  uint8_t last_four_bits = i & low_masked;  
  
  gpio_set_level(DB4, last_four_bits & 1);
  gpio_set_level(DB5, ((last_four_bits >> 1) & 1));  
  gpio_set_level(DB6, ((last_four_bits >> 2) & 1));  
  gpio_set_level(DB7, ((last_four_bits >> 3) & 1)); 
  
  gpio_set_level(E, HIGH);
  vTaskDelay(pdMS_TO_TICKS(3));
  gpio_set_level(E, LOW);
  vTaskDelay(pdMS_TO_TICKS(3));
}

void scoreboard_write(char i){
	gpio_set_level(E, HIGH);
  gpio_set_level(RS, HIGH);

  // Need the left 4 bits
  const uint8_t top_four_masked = 240;
  uint8_t first_four_bits = i & top_four_masked;
  first_four_bits = first_four_bits >> 4; 
  
  gpio_set_level(DB4, first_four_bits & 1);  
  gpio_set_level(DB5, ((first_four_bits >> 1) & 1));  
  gpio_set_level(DB6, ((first_four_bits >> 2) & 1));  
  gpio_set_level(DB7, ((first_four_bits >> 3) & 1));
  
  vTaskDelay(pdMS_TO_TICKS(10));
  gpio_set_level(E, HIGH);
  vTaskDelay(pdMS_TO_TICKS(10));
  gpio_set_level(E, LOW);
  vTaskDelay(pdMS_TO_TICKS(10));
 

  // Need the right 4 bits   

  const uint8_t low_masked = 15;
  uint8_t last_four_bits = i & low_masked;  
  
  gpio_set_level(DB4, last_four_bits & 1);
  gpio_set_level(DB5, ((last_four_bits >> 1) & 1));  
  gpio_set_level(DB6, ((last_four_bits >> 2) & 1));  
  gpio_set_level(DB7, ((last_four_bits >> 3) & 1));
   
  vTaskDelay(pdMS_TO_TICKS(10));
  gpio_set_level(E, HIGH);
  vTaskDelay(pdMS_TO_TICKS(10));
  gpio_set_level(E, LOW);
  vTaskDelay(pdMS_TO_TICKS(10));  
}

void scoreboard_init(){
  gpio_set_direction(DB4,GPIO_MODE_OUTPUT);          //Set DB4 as output
  gpio_set_direction(DB5,GPIO_MODE_OUTPUT);          //Set DB5 as output
  gpio_set_direction(DB6,GPIO_MODE_OUTPUT);          //Set DB6 as output
  gpio_set_direction(DB7,GPIO_MODE_OUTPUT);          //Set DB7 as output
  gpio_set_direction(RS,GPIO_MODE_OUTPUT);         //Set RS  as output                
  gpio_set_direction(E,GPIO_MODE_OUTPUT);         //Set E   as output

	// gpio_set_level(E, LOW);
	vTaskDelay(pdMS_TO_TICKS(50));
  gpio_set_level(RS, LOW);
  gpio_set_level(E, HIGH);
	gpio_set_level(DB4, HIGH);
  gpio_set_level(DB5, HIGH);  
  gpio_set_level(DB6, LOW);  
  gpio_set_level(DB7, LOW);

  gpio_set_level(E, HIGH);
  vTaskDelay(pdMS_TO_TICKS(3));
  gpio_set_level(E, LOW);
	vTaskDelay(pdMS_TO_TICKS(5)); //First 
  printf("First Done\n");
  scoreboard_command(0x28);
	//Delayms(5); // Second done
  printf("Second Done\n");
  scoreboard_command(0x28);
	//Delayms(5); // Third done
  printf("Third Done\n");
  scoreboard_command(0xE);
  //Fourth Done
  //Delayms(10);
  printf("Fourth Done\n");
  //Delayms(10);
	scoreboard_command(0x1);
  //Delayms(10);
  // command(0x10);
  //Delayms(10);
  scoreboard_command(0x6);
}


void send_string(char str[]){
  for(int i =0; i < strlen(str); ++i){
    // printf(str[i]);
    scoreboard_write(str[i]);
  }
}

void scoreboard_SetLine(int line){
    if(line == 1){
        scoreboard_command(0x80); //Set DDRAM to 0x00
    }
    else if(line == 2){
        scoreboard_command(0xC0); //Set DDRAM to 0x40
    }
    else if(line == 3){
        scoreboard_command(0x94); //Set DDRAM to 0x14
    }
    else if(line == 4){
        scoreboard_command(0xD4); //Set DDRAM to 0x54
    }
}

void scoreboard_SetLineEnd(int line, int strlen){
  uint8_t start = 0x80;
  //Need to change the below to the end of the line
  if(line == 1){
    start += 0x14;
  }
  else if(line == 2){
    start += 0x54;
  }
  else if(line == 3){
    start += 0x28;
  }
  else if(line == 4){
    start += 0x68;
  }
  start -= strlen;
  scoreboard_command(start);
}

void scoreboard_clearline(int line){
  scoreboard_SetLine(line);
  for(int i = 0; i < 20; i++){
    scoreboard_write(' ');
  }
}

void scoreboard_Chess_Setup(char name1[], char name2[], char country1[], char country2[], char rank1[], char rank2[]){
  // CharLCD_clearline(1);
  scoreboard_SetLine(1); // Names of Players
  if(strlen(name1) > 9){
  char name1temp[10];
  strncpy(name1temp, name1, 9);
  name1temp[9] = '\0';
  send_string(name1temp);
  }
  else{
    send_string(name1);
  }

  if(strlen(name2) > 9){
  scoreboard_SetLineEnd(1, 9);
  char name2temp[10];
  strncpy(name2temp, name2, 9);
  name2temp[9] = '\0';
  send_string(name2temp);
  }
  else{
    scoreboard_SetLineEnd(1, strlen(name2));
    send_string(name2);
  }

  //CharLCD_clearline(2);
  scoreboard_SetLine(2); // Country of Players
  scoreboard_write('(');
  if(strlen(country1) > 7){
  char country1temp[10];
  strncpy(country1temp, country1, 7);
  country1temp[7] = '\0';
  send_string(country1temp);
  }
  else{
    send_string(country1);
  }
  scoreboard_write(')');
  
  if(strlen(country2) > 7){
  scoreboard_SetLineEnd(2, 9);
  scoreboard_write('(');
  char country2temp[10];
  strncpy(country2temp, country2, 7);
  country2temp[7] = '\0';
  send_string(country2temp);
  }
  else{
    scoreboard_SetLineEnd(2, strlen(country2)+2);
    scoreboard_write('(');
    send_string(country2);
  }
  scoreboard_write(')');
  //Need to set DDRAM to end of line 2 minus length of 2nd string

  //CharLCD_clearline(3); //Nothing needs to be here unless draw is offered or game is over

  // CharLCD_clearline(4);
  scoreboard_SetLine(4); // Chess Rank?
  send_string(rank1);
  scoreboard_SetLineEnd(4, strlen(rank2));
  send_string(rank2);
  //Need to set DDRAM to end of line 4 minus length of 2nd string

}

void scoreboard_OfferDraw(bool LtR){
  //CharLCD_clearline(3);
  scoreboard_command(0x80+0x1B);
  send_string("Draw?");
  scoreboard_command(0x80+0x5B);
  if(LtR){
    send_string("---->");
  }
  else{
    send_string("<----");
  }
}

void scoreboard_DrawStatus(bool accepted){
  scoreboard_command(0x5B+0x80);
  send_string("     ");
  
  //CharLCD_clearline(3);
  scoreboard_command(0x80+0x17);
  if(accepted){
    send_string("Draw Accepted");
  }
  else{
    send_string("Draw Denied");
  }
}

void scoreboard_WinUpdate(char P1wins[], char P2wins[]){
  scoreboard_clearline(3);
  scoreboard_command(0x80+0x1C);
  send_string(P1wins);
  scoreboard_write('-');
  send_string(P2wins);
}

void scoreboard_clear(){
  scoreboard_command(0x01);
}
