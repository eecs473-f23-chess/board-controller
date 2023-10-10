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

void GraphicLCD_data_write(unsigned char d) // Data Output Serial Interface
{
  unsigned int n;
  gpio_set_level((gpio_num_t)(CS), LOW);
  gpio_set_level((gpio_num_t)RS, HIGH);
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
  gpio_set_level((gpio_num_t)CS, HIGH);
}

void GraphicLCD_comm_write(unsigned char d) // Command Output Serial Interface
{
  unsigned int n;
  gpio_set_level((gpio_num_t)CS, LOW);
  gpio_set_level((gpio_num_t)RS, LOW);
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
  gpio_set_level((gpio_num_t)CS, HIGH);
}

void GraphicLCD_DispPic(unsigned char *lcd_string)
{
  unsigned int i, j;
  unsigned char page = 0xB0;
  GraphicLCD_comm_write(0xAE); // Display OFF
  GraphicLCD_comm_write(0x40); // Display start address + 0x40
  for (i = 0; i < 8; i++)
  {                   // 64 pixel display / 8 pixels per page = 8 pages
    GraphicLCD_comm_write(page); // send page address
    GraphicLCD_comm_write(0x10); // column address upper 4 bits + 0x10
    GraphicLCD_comm_write(0x00); // column address lower 4 bits + 0x00
    for (j = 0; j < 128; j++)
    {                          // 128 columns wide
      GraphicLCD_data_write(*lcd_string); // send picture data
      lcd_string++;
    }
    page++; // after 128 columns, go to next page
  }
  GraphicLCD_comm_write(0xAF);
}

void GraphicLCD_ClearLCD(unsigned char *lcd_string)
{
  unsigned int i, j;
  unsigned char page = 0xB0;
  GraphicLCD_comm_write(0xAE); // Display OFF
  GraphicLCD_comm_write(0x40); // Display start address + 0x40
  for (i = 0; i < 8; i++)
  {                   // 64 pixel display / 8 pixels per page = 8 pages
    GraphicLCD_comm_write(page); // send page address
    GraphicLCD_comm_write(0x10); // column address upper 4 bits + 0x10
    GraphicLCD_comm_write(0x00); // column address lower 4 bits + 0x00
    for (j = 0; j < 128; j++)
    {                   // 128 columns wide
      GraphicLCD_data_write(0x00); // write clear pixels
      lcd_string++;
    }
    page++; // after 128 columns, go to next page
  }
  GraphicLCD_comm_write(0xAF);
}

/****************************************************
 *           Initialization For controller           
 *****************************************************/

void GraphicLCD_init_LCD()
{
  GraphicLCD_comm_write(0xA0); // ADC select
  GraphicLCD_comm_write(0xAE); // Display OFF
  GraphicLCD_comm_write(0xC8); // COM direction scan
  GraphicLCD_comm_write(0xA2); // LCD bias set
  GraphicLCD_comm_write(0x2F); // Power Control set
  GraphicLCD_comm_write(0x26); // Resistor Ratio Set
  GraphicLCD_comm_write(0x81); // Electronic Volume Command (set contrast) Double Byte: 1 of 2
  GraphicLCD_comm_write(0x11); // Electronic Volume value (contrast value) Double Byte: 2 of 2
  GraphicLCD_comm_write(0xAF); // Display ON
}

void GraphicLCD_DispNHDPic(int hour, int min_tens, int min_ones, int sec_tens, int sec_ones)
{
  unsigned char mask[1024];
  for(int i = 0; i <1024; ++i){
    mask[i] = (hours[hour])[i] | (minutes_ones[min_ones])[i] | Colons[i] | (minutes_tens[min_tens])[i] | (seconds_ones[sec_ones])[i] | (seconds_tens[sec_tens])[i];
  }
  GraphicLCD_DispPic(mask);
}
