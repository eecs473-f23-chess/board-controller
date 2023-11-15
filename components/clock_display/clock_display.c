#include <stdio.h>
#include "clock_display.h"
#include "hex_tables.h"

/*****************************************************************************
 *
/ Program for writing to NHD-C12864A1Z display Series with the ST7565P Controller.
/ This code is written for the Arduino Uno R3 using Serial Interface
/
/ Newhaven Display invests time and resources providing this open source code,
/ Please support Newhaven Display by purchasing products from Newhaven Display!

* Copyright (c) 2019, Newhaven Display International
*
* This code is provided as an example only and without any warranty by Newhaven Display.
* Newhaven Display accepts no responsibility for any issues resulting from its use.
* The developer of the final application incorporating any parts of this
* sample code is responsible for ensuring its safe and correct operation
* and for any consequences resulting from its use.
* See the GNU General Public License for more details.
*
* Use Vertical Orientation when converting BMP to hex code to display 
* custom image using LCD assistant.
*
*****************************************************************************/

/****************************************************
 *               Hex Table for NHD Pic               
 *****************************************************/


/****************************************************
 *               Pinout on Arduino Uno               
 *****************************************************/

//TODO: ALL delays are currently in ms, need to convert to ticks
//TODO: TRY and figure out where on the screen corresponds to which address
//TODO:

/****************************************************
 *                 Function Commands                 
 *****************************************************/

void GraphicLCD_data_write(unsigned char d, bool left) // Data Output Serial Interface
{
  unsigned int n;
  if(left){
    gpio_set_level((gpio_num_t)(CS_C2), LOW);
    gpio_set_level((gpio_num_t)RS_C2, HIGH);

    gpio_set_level((gpio_num_t)(CS_C1), HIGH);
  }
  else{
    gpio_set_level((gpio_num_t)(CS_C1), LOW);
    gpio_set_level((gpio_num_t)RS_C1, HIGH);
    
    gpio_set_level((gpio_num_t)(CS_C2), HIGH);
  }
  for (n = 0; n < 8; n++)
  {
    if ((d & 0x80) == 0x80)
      gpio_set_level((gpio_num_t)SI, HIGH);
    else
      gpio_set_level((gpio_num_t)SI, LOW);
    while (0);
    d = (d << 1);
    gpio_set_level((gpio_num_t)SC, LOW);
    while (0);
    gpio_set_level((gpio_num_t)SC, HIGH);
    while (0);
    gpio_set_level((gpio_num_t)SC, LOW);
  }
  if(left){
    gpio_set_level((gpio_num_t)CS_C2, HIGH);
  }
  else{
    gpio_set_level((gpio_num_t)CS_C1, HIGH);
  }
}

void GraphicLCD_comm_write(unsigned char d, bool left) // Command Output Serial Interface
{
  unsigned int n;
  if(left){
    gpio_set_level((gpio_num_t)CS_C2, LOW);
    gpio_set_level((gpio_num_t)RS_C2, LOW);

    gpio_set_level((gpio_num_t)CS_C1, HIGH);
    // gpio_set_level((gpio_num_t)RS_C1, HIGH);
  }
  else{
    gpio_set_level((gpio_num_t)CS_C1, LOW);
    gpio_set_level((gpio_num_t)RS_C1, LOW);

    gpio_set_level((gpio_num_t)CS_C2, HIGH);
    // gpio_set_level((gpio_num_t)RS_C2, HIGH);
  }
  
  for (n = 0; n < 8; n++)
  {
    if ((d & 0x80) == 0x80)
      gpio_set_level((gpio_num_t)SI, HIGH);
    else
      gpio_set_level((gpio_num_t)SI, LOW);
    while (0);
    d = (d << 1);
    gpio_set_level((gpio_num_t)SC, LOW);
    while (0);
    gpio_set_level((gpio_num_t)SC, HIGH);
    while (0);
    gpio_set_level((gpio_num_t)SC, LOW);
  }
  if(left){
    gpio_set_level((gpio_num_t)CS_C1, HIGH);
  }
  else{
    gpio_set_level((gpio_num_t)CS_C2, HIGH);
  }
}

void GraphicLCD_DispPic(unsigned char *lcd_string, bool left)
{
  unsigned int i, j;
  unsigned char page = 0xB0;
  //GraphicLCD_comm_write(0xAE, left); // Display OFF
  GraphicLCD_comm_write(0x40, left); // Display start address + 0x40
  for (i = 0; i < 8; i++)
  {                   // 64 pixel display / 8 pixels per page = 8 pages
    GraphicLCD_comm_write(page, left); // send page address
    GraphicLCD_comm_write(0x10, left); // column address upper 4 bits + 0x10
    GraphicLCD_comm_write(0x00, left); // column address lower 4 bits + 0x00
    for (j = 0; j < 128; j++)
    {                          // 128 columns wide
      GraphicLCD_data_write(*lcd_string, left); // send picture data
      lcd_string++;
    }
    page++; // after 128 columns, go to next page
  }
  //GraphicLCD_comm_write(0xAF, left);
}

void GraphicLCD_ClearLCD(bool left)
{
  unsigned int i, j;
  unsigned char page = 0xB0;
  //GraphicLCD_comm_write(0xAE, left); // Display OFF
  GraphicLCD_comm_write(0x40, left); // Display start address + 0x40
  for (i = 0; i < 8; i++)
  {                   // 64 pixel display / 8 pixels per page = 8 pages
    GraphicLCD_comm_write(page, left); // send page address
    GraphicLCD_comm_write(0x10, left); // column address upper 4 bits + 0x10
    GraphicLCD_comm_write(0x00, left); // column address lower 4 bits + 0x00
    for (j = 0; j < 128; j++)
    {                   // 128 columns wide
      GraphicLCD_data_write(0x00, left); // write clear pixels
    }
    page++; // after 128 columns, go to next page
  }
  //GraphicLCD_comm_write(0xAF, left);
}

/****************************************************
 *           Initialization For controller           
 *****************************************************/

void GraphicLCD_init_LCD()
{
  gpio_set_direction((gpio_num_t)RES, GPIO_MODE_OUTPUT); // configure RES as output
  gpio_set_direction((gpio_num_t)CS_C1, GPIO_MODE_OUTPUT);  // configure CS as output
  gpio_set_direction((gpio_num_t)RS_C1, GPIO_MODE_OUTPUT);  // configure RS as output
  gpio_set_direction((gpio_num_t)SC, GPIO_MODE_OUTPUT);  // configure SC as output
  gpio_set_direction((gpio_num_t)SI, GPIO_MODE_OUTPUT);  // configure SI as output
  gpio_set_direction((gpio_num_t)RS_C2, GPIO_MODE_OUTPUT);
  gpio_set_direction((gpio_num_t)CS_C2, GPIO_MODE_OUTPUT);
  gpio_set_level((gpio_num_t)RES, LOW);
  vTaskDelay(pdMS_TO_TICKS(100));
  gpio_set_level((gpio_num_t)RES, HIGH);
  vTaskDelay(pdMS_TO_TICKS(100));

  //Clock 1

  GraphicLCD_comm_write(0xA0, true); // ADC select
  GraphicLCD_comm_write(0xAE, true); // Display OFF
  GraphicLCD_comm_write(0xC8, true); // COM direction scan
  GraphicLCD_comm_write(0xA2, true); // LCD bias set
  GraphicLCD_comm_write(0x2F, true); // Power Control set
  GraphicLCD_comm_write(0x26, true); // Resistor Ratio Set
  GraphicLCD_comm_write(0x81, true); // Electronic Volume Command (set contrast) Double Byte: 1 of 2
  GraphicLCD_comm_write(0x11, true); // Electronic Volume value (contrast value) Double Byte: 2 of 2
  GraphicLCD_comm_write(0xAF, true); // Display ON

  //Clock 2

  GraphicLCD_comm_write(0xA0, false); // ADC select
  GraphicLCD_comm_write(0xAE, false); // Display OFF
  GraphicLCD_comm_write(0xC8, false); // COM direction scan
  GraphicLCD_comm_write(0xA2, false); // LCD bias set
  GraphicLCD_comm_write(0x2F, false); // Power Control set
  GraphicLCD_comm_write(0x26, false); // Resistor Ratio Set
  GraphicLCD_comm_write(0x81, false); // Electronic Volume Command (set contrast) Double Byte: 1 of 2
  GraphicLCD_comm_write(0x11, false); // Electronic Volume value (contrast value) Double Byte: 2 of 2
  GraphicLCD_comm_write(0xAF, false); // Display ON
  GraphicLCD_ClearLCD(true);
  GraphicLCD_ClearLCD(false);
}

void GraphicLCD_DispClock(int ms, bool left)
{
  int seconds = ms/1000;
  int minutes = seconds/60;
  seconds -= minutes*60;
  int sec_ones = seconds%10;
  int sec_tens = seconds/10;
  int hour = minutes/60;
  minutes -= hour*60;
  int min_ones = minutes%10;
  int min_tens = minutes/10;

  unsigned char mask[1024];
  for(int i = 0; i <1024; ++i){
    mask[i] = (hours[hour])[i] | (minutes_ones[min_ones])[i] | Colons[i] | (minutes_tens[min_tens])[i] | (seconds_ones[sec_ones])[i] | (seconds_tens[sec_tens])[i];
  }
  GraphicLCD_DispPic(mask, left);
}

void decrement_time(void * pvParameters){
  TickType_t xLastWakeTime = xTaskGetTickCount();
  while(1){
    if(our_turn && white_time != -1){
      portDISABLE_INTERRUPTS();
      white_time -= 1000;
      GraphicLCD_DispClock(white_time, our_turn);
      portENABLE_INTERRUPTS();
    }
    else if(!our_turn && black_time != -1){
      portDISABLE_INTERRUPTS();
      black_time -= 1000;
      GraphicLCD_DispClock(black_time, our_turn);
      portENABLE_INTERRUPTS();
    }
    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1000));
  }
}
