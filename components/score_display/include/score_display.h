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

#define RS  (gpio_num_t)10
#define RW  (gpio_num_t)11
#define E   (gpio_num_t)12
#define LOW 0
#define HIGH 1
#define DB4 (gpio_num_t)6
#define DB5 (gpio_num_t)7
#define DB6 (gpio_num_t)8
#define DB7 (gpio_num_t)9

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
char const text1[] = {"Newhaven Display"};
char const text2[] = {"Character LCD   "};

void Delayms(int n){
	int i;
	int j;
	for (i=0;i<n;i++)
		for (j=0;j<1000;j++)
		{;}
}

void command(char i){
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

void write(char i){
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
  vTaskDelay(pdMS_TO_TICKS(100));

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

void init(){
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
  command(0x28);
	//Delayms(5); // Second done
  printf("Second Done\n");
  command(0x28);
	//Delayms(5); // Third done
  printf("Third Done\n");
  command(0xE);
  //Fourth Done
  //Delayms(10);
  printf("Fourth Done\n");
  //Delayms(10);
	command(0x1);
  //Delayms(10);
  // command(0x10);
  //Delayms(10);
  command(0x6);
}
void home(){
	command(0x01);
	vTaskDelay(pdMS_TO_TICKS(5));
}
void nextline(){
	command(0xc0);
}
void disp_pic(){
	int i;
	home();
	for (i=0;i<16;i++){
		write(text1[i]);
	}
	 nextline();
	 for (i=0;i<16;i++){
		write(text2[i]);
	}
}

void send_string(char str[]){
  for(int i =0; i < strlen(str); ++i){
    // printf(str[i]);
    write(str[i]);
  }
}

void CharLCD_SetLine(int line){
    if(line == 1){
        command(0x80); //Set DDRAM to 0x00
    }
    else if(line == 2){
        command(0xC0); //Set DDRAM to 0x40
    }
    else if(line == 3){
        command(0x94); //Set DDRAM to 0x14
    }
    else if(line == 4){
        command(0xD4); //Set DDRAM to 0x54
    }
}

void CharLCD_SetLineEnd(int line, int strlen){
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
  command(start);
}

void CharLCD_clearline(int line){
  CharLCD_SetLine(line);
  for(int i = 0; i < 20; i++){
    write(' ');
  }
}

void CharLCD_Chess_Setup(char name1[], char name2[], char country1[], char country2[], char rank1[], char rank2[]){
  CharLCD_clearline(1);
  CharLCD_SetLine(1); // Names of Players
  send_string(name1);
  CharLCD_SetLineEnd(1, strlen(name2));
  //Need to set DDRAM to end of line 1 minus length of 2nd string
  send_string(name2);

  CharLCD_clearline(2);
  CharLCD_SetLine(2); // Country of Players
  write('(');
  send_string(country1);
  write(')');
  CharLCD_SetLineEnd(2, strlen(country2)+2);
  write('(');
  send_string(country2);
  write(')');
  //Need to set DDRAM to end of line 2 minus length of 2nd string

  CharLCD_clearline(3); //Nothing needs to be here unless draw is offered or game is over

  CharLCD_clearline(4);
  CharLCD_SetLine(4); // Chess Rank?
  send_string(rank1);
  CharLCD_SetLineEnd(4, strlen(rank2));
  send_string(rank2);
  //Need to set DDRAM to end of line 4 minus length of 2nd string

}

void CharLCD_OfferDraw(bool LtR){
  CharLCD_clearline(3);
  command(0x80+0x1B);
  send_string("Draw?");
  command(0x80+0x5B);
  if(LtR){
    send_string("---->");
  }
  else{
    send_string("<----");
  }
}

void CharLCD_AcceptDraw(char rank1[], char rank2[]){
  CharLCD_clearline(4);
  CharLCD_SetLine(4); // Chess Rank?
  send_string(rank1);
  CharLCD_SetLineEnd(1, strlen(rank2));
  send_string(rank2);
  CharLCD_clearline(3);
  command(0x80+0x17);
  send_string("Draw Accepted");
}

void CharLCD_WinUpdate(char P1wins, char P2wins){
  CharLCD_clearline(3);
  command(0x80+0x1C);
  write(P1wins);
  write('-');
  write(P2wins);
}
