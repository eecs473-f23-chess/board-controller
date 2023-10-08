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

#define RS  10
#define RW  11
#define E   12
#define LOW 0
#define HIGH 1


void CharLCD_command(unsigned char c){ //Function that sends commands
   for(int i=7;i>=0;i--){
      if( (c&0x80) ==0x00){
        gpio_set_level((gpio_num_t)(i+2),LOW);
      }
      else {
        gpio_set_level((gpio_num_t)(i+2),HIGH);
      }
      c= c<<1; 
    vTaskDelay(pdMS_TO_TICKS(1));
   }

   
   vTaskDelay(pdMS_TO_TICKS(2));  
   gpio_set_level((gpio_num_t)RS,LOW);      //RS set to LOW for command
   vTaskDelay(pdMS_TO_TICKS(1));
   gpio_set_level((gpio_num_t)E, HIGH);     //E set to HIGH for latching
   vTaskDelay(pdMS_TO_TICKS(1));
   gpio_set_level((gpio_num_t)E,LOW);       //E set to LOW for latching
   vTaskDelay(pdMS_TO_TICKS(1));
}

void CharLCD_data(unsigned char d){   //Function that sends data
   for(int i=7;i>=0;i--){
      if( (d&0x80) ==0x00){
        gpio_set_level((gpio_num_t)(i+2),LOW);
      }
      else {
        gpio_set_level((gpio_num_t)(i+2),HIGH);
      }
      d= d<<1; 
    vTaskDelay(pdMS_TO_TICKS(0.002)); //micro
   }
   
   gpio_set_level((gpio_num_t)RS,HIGH);     //RS set to HIGH for data
   vTaskDelay(pdMS_TO_TICKS(1));
   gpio_set_level((gpio_num_t)E,HIGH);      //E set to HIGH for latching
   vTaskDelay(pdMS_TO_TICKS(1)); //micro
   gpio_set_level((gpio_num_t)E,LOW);       //E set to LOW for latching
   vTaskDelay(pdMS_TO_TICKS(1)); //micro
}

void CharLCD_send_string(char str[]){
  for(int i =0; str[i] != '\n'; ++i){
    CharLCD_data(str[i]);
  }
}

void Init_CharLCD(){
  gpio_set_direction((gpio_num_t)6,GPIO_MODE_OUTPUT);          //Set DB4 as output
  gpio_set_direction((gpio_num_t)7,GPIO_MODE_OUTPUT);          //Set DB5 as output
  gpio_set_direction((gpio_num_t)8,GPIO_MODE_OUTPUT);          //Set DB6 as output
  gpio_set_direction((gpio_num_t)9,GPIO_MODE_OUTPUT);          //Set DB7 as output
  gpio_set_direction((gpio_num_t)10,GPIO_MODE_OUTPUT);         //Set RS  as output                
  gpio_set_direction((gpio_num_t)12,GPIO_MODE_OUTPUT);         //Set E   as output
  
  gpio_set_level((gpio_num_t)RS,LOW);       //Set RS LOW 
  gpio_set_level((gpio_num_t)E,HIGH);       //Set E  HIGH 
  vTaskDelay(pdMS_TO_TICKS(10));                  //Delay for 10 miliseconds
  gpio_set_level((gpio_num_t)E, LOW);       //Set E  LOW
  vTaskDelay(pdMS_TO_TICKS(100));
  CharLCD_command(0x30);              //command 0x30 = Wake up
  vTaskDelay(pdMS_TO_TICKS(30)); 
  CharLCD_command(0x30);              //command 0x30 = Wake up #2
  vTaskDelay(pdMS_TO_TICKS(10));
  CharLCD_command(0x30);              //command 0x30 = Wake up #3
  vTaskDelay(pdMS_TO_TICKS(10)); 
  CharLCD_command(0x28);              //Function set: 4-bit/2-line
  CharLCD_command(0x10);              //Set cursor
  CharLCD_command(0x0c);              //Display ON; Cursor ON
  CharLCD_command(0x06);              //Entry mode set
}

void CharLCD_SetLine(int line){
    if(line == 1){
        CharLCD_command(0x80); //Set DDRAM to 0x00
    }
    else if(line == 2){
        CharLCD_command(0xC0); //Set DDRAM to 0x40
    }
    else if(line == 3){
        CharLCD_command(0x94); //Set DDRAM to 0x14
    }
    else if(line == 4){
        CharLCD_command(0xD4); //Set DDRAM to 0x54
    }
}

void CharLCD_SetLineEnd(int line, int strlen){
  uint8_t start = 0x80;
  //Need to change the below to the end of the line
  if(line == 1){
    start += 0x13;
  }
  else if(line == 2){
    start += 0x53;
  }
  else if(line == 3){
    start += 0x27;
  }
  else if(line == 4){
    start += 0x67;
  }
  start -= strlen;
  CharLCD_command(start);
}

void CharLCD_clearline(int line){
  CharLCD_SetLine(line);
  for(int i = 0; i < 20; i++){
    CharLCD_data(' ');
  }
}

void CharLCD_Chess_Setup(char name1[], char name2[], char country1[], char country2[], char rank1[], char rank2[]){
  CharLCD_clearline(1);
  CharLCD_SetLine(1); // Names of Players
  CharLCD_send_string(name1);
  CharLCD_SetLineEnd(1, strlen(name2));
  //Need to set DDRAM to end of line 1 minus length of 2nd string
  CharLCD_send_string(name2);

  CharLCD_clearline(2);
  CharLCD_SetLine(2); // Country of Players
  CharLCD_data('(');
  CharLCD_send_string(country1);
  CharLCD_data(')');
   CharLCD_SetLineEnd(2, strlen(country2)+2);
  CharLCD_data('(');
  CharLCD_send_string(country2);
  CharLCD_data(')');
  //Need to set DDRAM to end of line 2 minus length of 2nd string

  CharLCD_clearline(3); //Nothing needs to be here unless draw is offered or game is over

  CharLCD_clearline(4);
  CharLCD_SetLine(4); // Chess Rank?
  CharLCD_send_string(rank1);
  CharLCD_SetLineEnd(1, strlen(rank2));
  CharLCD_send_string(rank2);
  //Need to set DDRAM to end of line 4 minus length of 2nd string

}

void CharLCD_OfferDraw(bool LtR){
  CharLCD_clearline(3);
  CharLCD_command(0x80+0x1B);
  CharLCD_send_string("Draw?");
  CharLCD_command(0x80+0x5B);
  if(LtR){
    CharLCD_send_string("---->");
  }
  else{
    CharLCD_send_string("<----");
  }
}

void CharLCD_AcceptDraw(char rank1[], char rank2[]){
  CharLCD_clearline(4);
  CharLCD_SetLine(4); // Chess Rank?
  CharLCD_send_string(rank1);
  CharLCD_SetLineEnd(1, strlen(rank2));
  CharLCD_send_string(rank2);
  CharLCD_clearline(3);
  CharLCD_command(0x80+0x17);
  CharLCD_send_string("Draw Accepted");
}

void CharLCD_WinUpdate(char P1wins, char P2wins){
  CharLCD_clearline(3);
  CharLCD_command(0x80+0x1C);
  CharLCD_data(P1wins);
  CharLCD_data('-');
  CharLCD_data(P2wins);
}