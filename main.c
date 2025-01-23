//For the ATTINY1616
#define F_CPU 20000000
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

//my library will allocate enough memory to store PCF8574_DEVICES_USED device's state (2 bytes per device)
#define PCF8574_DEVICES_USED 1	
//have the library implement uint8_t to string (+4 bytes of ram used)
#define PCF8574_IMPL_UTOS				 
#include "PCF8574T_driver.h"

//The simulated teeth for the respected gearing
#define IN_TEETH 1
#define OUT_TEETH 1

// when the gearbox gets to this many pings, stuff is backed off by IN_PPR to
// avoid overflow the product of nTEETH and nPPR should not exeed this value			      
#define PINGS_REORIGIN 512

// The base pings per rotation for each of the devices. We can give ourselves
// some padding by dividing both by a constant because it is the ratio that
// matteres. (360 PPR for the encoder, 400 PPR for the stepper motor when
// half stepping)
#define IN_PPR 36           
#define OUT_PPR 40

// Ports for each of the devices
#define RENCODER_PORT PORTA
#define RENCODER_PINA PIN1_bm
#define RENCODER_PINB PIN4_bm
#define RENCODER_CTRLA PIN1CTRL
#define RENCODER_CTRLB PIN4CTRL

#define LED_PORT PORTC
#define LED_PORTMASK 0b00001111;

#define MOTOR_PORT PORTB
#define STEP_PIN PIN2_bm
#define DIR_PIN PIN3_bm

void CPU_CLK_init();
void PORT_init();

// Each quadrature possibility, mapped to an position (phases[reading] =
// ordinal) The attiny1616 chip does not have a quadrature decoder, but decoding
// it is easy enough.
const int8_t phases[] = {
  0,
  1,
  3,
  2
};

//varibles static to the ISR are declared volatile because events can intermix*
volatile int16_t pings = 0;
volatile int8_t phase_old = 0;
	
volatile int16_t steps_goal = 0;
volatile int16_t steps = 0;
volatile int8_t skippedpings = 0;

ISR(PORTA_PORT_vect){
  //reset interrupt
  uint8_t portAFlags = RENCODER_PORT.INTFLAGS;
  RENCODER_PORT.INTFLAGS = portAFlags;
  //solve quadrature
  uint8_t filtered = (RENCODER_PORT.IN | RENCODER_PORT.DIR);
  uint8_t positionid = ((filtered & PIN1_bm) >> 1) | ((filtered & PIN4_bm) >> 3);
  int8_t phase = phases[positionid];
  int8_t change = phase - phase_old;
  phase_old = phase;
  if(change == 1 || change == -3){
    pings++;
  } else if (change == -1 || change == 3) {
    pings--;
  } else {
    skippedpings++;    
    pcf8574_lcd_cmd(LCD_CURSOR_BEGINNING_SECOND_LINE);
    pcf8574_lcd_msg(utos(skippedpings));
    _delay_ms(100);
  } 
  //Calculate steps. This is where the magic happenes.
  int16_t longpings = pings;
  steps_goal = longpings * OUT_PPR / IN_PPR;//((longpings * OUTP) / INP);
  //Overflow preventing
  if(pings >= 512){
    pings -= IN_PPR*OUT_TEETH;
    steps_goal -= OUT_PPR*IN_TEETH;
    steps -= OUT_PPR*IN_TEETH;
  }
  if(pings <= -512){
    pings += IN_PPR*OUT_TEETH;
    steps_goal += OUT_PPR*IN_TEETH;
    steps += OUT_PPR*IN_TEETH;
  }
}



int main() {
  
  CPU_CLK_init();
  PORT_init();	
  
  twictl_init(100000, TWI0.CTRLA);			  //Initialize twi at 100kHz with standard settigns
  pcf8574_select_device(0);				  //Use device state at index 0
  pcf8574_initialize_device(0x27);			  //Initialize device at I2C address 0x27
  pcf8574_lcd_init();					  //Initialize the lcd
  
  pcf8574_lcd_msg("started");				  //Display startup message
  _delay_ms(1000);
  pcf8574_lcd_cmd(LCD_CLEAR_DISPLAY);
  _delay_ms(1000);
  pcf8574_lcd_msg("dropped pings");
  pcf8574_lcd_cmd(LCD_CURSOR_BEGINNING_SECOND_LINE);
  pcf8574_lcd_msg("000");
  
  sei();						  //Enable Interrupts
	
  while(1){
    LED_PORT.OUT = steps & LED_PORT.DIR;
    if(steps_goal > steps){
      MOTOR_PORT.OUTSET = STEP_PIN | DIR_PIN;
      steps++;
    } else if(steps_goal < steps){
      MOTOR_PORT.OUTCLR = DIR_PIN;
      MOTOR_PORT.OUTSET = STEP_PIN;
      steps--;
    }
    _delay_us(5);
    MOTOR_PORT.OUTCLR = STEP_PIN;
    _delay_us(100);
  }
}

// Select the 20/16 MHz oscilator to run the cpu and
// disable prescaling by writing the prescaling bits
// to 0.
// (upload.sh writes to a fuse selecting the 20MHz frequency)

void CPU_CLK_init() {
  _PROTECTED_WRITE(CLKCTRL_MCLKCTRLB, 0x00 | CLKCTRL_OSC20MCTRLA);
}

// (in order)
// Set the IO direction for the rotary encoder pins, the LED pins, and the motor
// pins. Then enable pin change interrupts on the rotary encoder pin and set it
// to trigger on both the rising and falling edge

void PORT_init(){
  RENCODER_PORT.DIRCLR = RENCODER_PINA | RENCODER_PINB;
  LED_PORT.DIRSET = LED_PORTMASK;
  MOTOR_PORT.DIRSET = STEP_PIN | DIR_PIN;
  PORTA.PIN1CTRL = PORT_ISC_BOTHEDGES_gc | PORT_PULLUPEN_bm;
  PORTA.PIN4CTRL = PORT_ISC_BOTHEDGES_gc | PORT_PULLUPEN_bm;
}
