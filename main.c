#include <avr/io.h>
#define F_CPU 20000000
#include <util/delay.h>
#include <avr/interrupt.h>

#define IN_TEETH 1
#define OUT_TEETH 1


#define PIPS_REORIGIN 100     // when the gearbox gets to this many pips, stuff is backed off by IN_PPR to avoid overflow
			      // the product of nTEETH and nPPR should not exeed this value
			      
#define IN_PPR 36           // the ratio is what matters
#define OUT_PPR 40


#define RENCODER_PORT PORTA
#define RENCODER_PINA PIN1_bm
#define RENCODER_PINB PIN4_bm

#define LED_PORT PORTC
#define LED_PORTMASK 0b00001111;

#define MOTOR_PORT PORTB
#define STEP_PIN PIN0_bm
#define DIR_PIN PIN1_bm

void CPU_CLK_init(){
	_PROTECTED_WRITE(CLKCTRL_MCLKCTRLA, CLKCTRL_OSC20MCTRLA); // select OSC20M to run the processor (bit7 - CLKOUT, bit1:0 - CLKSEL) 		
	_PROTECTED_WRITE(CLKCTRL_MCLKCTRLB, 0x00); // disable prescaling, give me SPEED! (set CLKCTRL_PEN_bm to 0)'
}

char phases[] = {
	0,
	1,
	3,
	2
};

void PORT_init(){
	RENCODER_PORT.DIRCLR = RENCODER_PINA | RENCODER_PINB;
	LED_PORT.DIRSET = LED_PORTMASK;
	MOTOR_PORT.DIRSET = STEP_PIN | DIR_PIN;
	PORTA.PIN1CTRL = PORT_ISC_BOTHEDGES_gc | PORT_PULLUPEN_bm;
	PORTA.PIN4CTRL = PORT_ISC_BOTHEDGES_gc | PORT_PULLUPEN_bm;
	
}

volatile int16_t pips = 0;
volatile char phase_old = 0;
	
volatile int16_t steps_goal = 0;
volatile int16_t steps = 0;

ISR(PORTA_PORT_vect){

	uint8_t portAFlags = PORTA.INTFLAGS;
    	PORTA.INTFLAGS = portAFlags;
    	unsigned char shifted = (PORTA.IN | PORTA.DIR);
	unsigned char posid = ((shifted & PIN1_bm) >> 1) | ((shifted & PIN4_bm) >> 3);
	char phase = phases[posid];
	PORTC.OUT = phase_old;
	char change = phase - phase_old;
	if(change == 1 || change == -3){
		pips++;
	} else {
		pips--;
	}
	int16_t longpips = pips;
	steps_goal = longpips * OUT_PPR / IN_PPR;//((longpips * OUTP) / INP);
	if(pips >= 512){
		pips -= IN_PPR*OUT_TEETH;
		steps_goal -= OUT_PPR*IN_TEETH;
		steps -= OUT_PPR*IN_TEETH;
	}
	if(pips <= -512){
		pips += IN_PPR*OUT_TEETH;
		steps_goal += OUT_PPR*IN_TEETH;
		steps += OUT_PPR*IN_TEETH;
	}
	phase_old = phase;
}



int main() {


	CPU_CLK_init();
	PORT_init();	
	sei();
	
	while(1){
		PORTC.OUT = steps & PORTC.DIR;
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
