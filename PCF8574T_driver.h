#ifndef _PCF8574T_DRIVER_
#define _PCF8574T_DRIVER_
#include "twi.h"

#ifndef PCF8574_DEVICES_USED
#define PCF8574_DEVICES_USED 1
#endif

uint8_t pcf8574_current_address = 0;
uint8_t pcf8574_current_state = 0;
uint8_t pcf8574_current_index = 0;

uint8_t pcf8574_states[PCF8574_DEVICES_USED];
uint8_t pcf8574_addresses[PCF8574_DEVICES_USED];

void pcf8574_select_device(uint8_t index);
void pcf8574_initialize_device(uint8_t index);

void pcf8574_write(uint8_t);
void pcf8574_lcd_4bit_write(uint8_t);
void pcf8574_lcd_cmd(unsigned char x);
void pcf8574_lcd_dwr(unsigned char x);
void pcf8574_lcd_msg(char *c);
void pcf8574_lcd_init();

//##########################################################

#ifdef PCF8574_IMPL_UTOS
uint8_t utos_charstore[4];
uint8_t* utos(uint8_t c){
	uint8_t num = c;
	utos_charstore[3] = 0;
	utos_charstore[0] = (c/100)%10 + *"0";
	utos_charstore[1] = (c/10)%10 + *"0";
	utos_charstore[2] = c%10 + *"0";
	return utos_charstore;
}
#endif

void pcf8574_initialize_device(uint8_t address){
	pcf8574_current_address = address;
}

void pcf8574_select_device(uint8_t index){
	pcf8574_states[pcf8574_current_index] = pcf8574_current_state;
	pcf8574_addresses[pcf8574_current_index] = pcf8574_current_address;
	pcf8574_current_index = index;
	pcf8574_current_state = pcf8574_states[index];
	pcf8574_current_address = pcf8574_addresses[index];
}

void pcf8574_write(uint8_t x){
	twictl_start(pcf8574_current_address, TWI_WRITE);
	twictl_tx(x);
	twictl_stop();
}

void pcf8574_lcd_4bit_write(uint8_t x){
	unsigned char temp = 0x00;						//--- temp variable for data operation
	
	pcf8574_current_state &= 0x0F;						//--- Masking last four bit to prevent the RS, RW, EN, Backlight
	temp = (x & 0xF0);							//--- Masking higher 4-Bit of Data and send it LCD
	pcf8574_current_state |= temp;						//--- 4-Bit Data and LCD control Pin
	pcf8574_current_state |= (0x04);					//--- Latching Data to LCD EN = 1
	pcf8574_write(pcf8574_current_state);					//--- Send Data From PCF8574 to LCD PORT
	_delay_us(1);								//--- 1us Delay
	pcf8574_current_state &= ~(0x04);					//--- Latching Complete
	pcf8574_write(pcf8574_current_state);					//--- Send Data From PCF8574 to LCD PORT 
	_delay_us(5);								//--- 5us Delay to Complete Latching
	
	
	temp = ((x & 0x0F)<<4);							//--- Masking Lower 4-Bit of Data and send it LCD
	pcf8574_current_state &= 0x0F;						//--- Masking last four bit to prevent the RS, RW, EN, Backlight					
	pcf8574_current_state |= temp;						//--- 4-Bit Data and LCD control Pin
	pcf8574_current_state |= (0x04);					//--- Latching Data to LCD EN = 1
	pcf8574_write(pcf8574_current_state);					//--- Send Data From PCF8574 to LCD PORT
	_delay_us(1);								//--- 1us Delay
	pcf8574_current_state &= ~(0x04);					//--- Latching Complete
	pcf8574_write(pcf8574_current_state);					//--- Send Data From PCF8574 to LCD PORT
	_delay_us(5);	
}

void pcf8574_lcd_cmd(unsigned char x){
	pcf8574_current_state = 0x08;						//--- Enable Backlight Pin
	pcf8574_current_state &= ~(0x01);					//--- Select Command Register By RS = 0
	pcf8574_write(pcf8574_current_state);					//--- Send Data From PCF8574 to LCD PORT
	pcf8574_lcd_4bit_write(x);						//--- Function to Write 4-bit data to LCD 
}

void pcf8574_lcd_dwr(unsigned char x){
	pcf8574_current_state |= 0x09;						//--- Enable Backlight Pin & Select Data Register By RS = 1
	pcf8574_write(pcf8574_current_state);					//--- Send Data From PCF8574 to LCD PORT	
	pcf8574_lcd_4bit_write(x);						//--- Function to Write 4-bit data to LCD
}

void pcf8574_lcd_msg(char *c){
	while (*c != '\0')							//--- Check Pointer for Null
		pcf8574_lcd_dwr(*c++);						//--- Send the String of Data
}

void pcf8574_lcd_clear(){
	return pcf8574_lcd_cmd(0x01);
}

void pcf8574_lcd_init(){	
	pcf8574_current_state = 0x04;						//--- EN = 1 for 25us initialize Sequence
	pcf8574_write(pcf8574_current_state);
	_delay_us(26);
	
	pcf8574_lcd_cmd(0x03);				//--- Initialize Sequence
	pcf8574_lcd_cmd(0x03);				//--- Initialize Sequence
	pcf8574_lcd_cmd(0x03);				//--- Initialize Sequence
	pcf8574_lcd_cmd(0x02);				//--- Return to Home
	pcf8574_lcd_cmd(0x28);				//--- 4-Bit Mode 2 - Row Select
	pcf8574_lcd_cmd(0x0F);				//--- Cursor on, Blinking on
	pcf8574_lcd_cmd(0x01);				//--- Clear LCD
	pcf8574_lcd_cmd(0x06);				//--- Auto increment Cursor
	pcf8574_lcd_cmd(0x80);				//--- Row 1 Column 1 Address
}

#define LCD_CLEAR_DISPLAY 0x01
#define LCD_RETURN_HOME 0x02
#define LCD_CURSOR_SHIFT_DECREMENT 0x04
#define LCD_CURSOR_SHIFT_INCREMENT 0x06
#define LCD_SHIFT_DISPLAY_RIGHT 0x05
#define LCD_SHIFT_DISPLAY_LEFT 0x07
#define LCD_DISPLAY_OFF_CURSOR_OFF 0x08
#define LCD_DISPLAY_OFF_CURSOR_ON 0x0A
#define LCD_DISPLAY_ON_CURSOR_OFF 0x0C
#define LCD_DISPLAY_ON_CURSOR_BLINKING 0x0E
#define LCD_DISPLAY_OFF_CURSOR_BLINKING 0x0F
#define LCD_CURSOR_POSITION_SHIFT_LEFT 0x10
#define LCD_CURSOR_POSITION_SHIFT_RIGHT 0x14
#define LCD_SHIFT_FULL_DISPLAY_LEFT 0x18
#define LCD_SHIFT_FULL_DISPLAY_RIGHT 0x1C
#define LCD_CURSOR_BEGINNING_FIRST_LINE 0x80
#define LCD_CURSOR_BEGINNING_SECOND_LINE 0xC0
#define LCD_INIT_2LINE_5X7MATRIX 0x38	

#endif
