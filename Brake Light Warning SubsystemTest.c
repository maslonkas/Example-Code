/*
Brake Light Warning System ***Subsystem Test for Speed and Deceleration***
Written by StanLee Maslonka

Program Purpose: Program the ATmega32 to receive Speed from a Hall Effect Sensor and Deceleration 
				 from an Accelerometer and control a LED Bar's brightness and flashing rate accordingly.
Inputs:	Speed from the Hall Effect Sensor sending a digital logic one (+5V) or logic zero (0V). The 
		signals are detected by the Input Capture One on PD6 using the on board Timer/Counter One
		Deceleration from the Accelerometer sending an analog voltage (0-2.414V). The signals are 
		detected and converted by the on board Analog-to-Digital Converter.
Outputs: PWM (Brightness/Intensity) from OCR0 at PB3 using Timer/Counter Zero to the input of an AND Gate
		 PORTC (Flashing) from PCx to the input of an AND Gate.
		 The Brightness and Flashing will be sent through the AND Gate (IC 7408), then the output of the
		 AND Gate will go to the LED(s).
*/

//include files
#include <avr/io.h>
#include <inttypes.h>
#include <stdlib.h>
#include <math.h>
#include <MSOE/delay.c>
#include <avr/interrupt.h>
#include <avr/signal.h>
#include <MSOE/bit.c>

//declare variables
uint16_t Speed;
uint16_t NOWspeed;
uint16_t PASTspeed;
uint8_t Flash;
uint8_t Deceleration;
#define PowerOff 0x00
#define Power100percent 0xFF
#define Power65percent 0xA5
#define Power30percent 0x4D
#define TwentyMPH 0x05A0
#define FortyMPH 0x03C0
#define SixtyMPH 0x0280
#define GforceThree 0x5F
#define GforceTwo 0x68
#define GforceOne 0x70

int main(void)	//beginning of main function
{
//Assign I/O PINs and/or PORTs
PORTA=0xFF;	//enable internal resistors of PORTA
DDRB=0xFF;	//set PORTB to write
DDRC=0xFF;	//set PORTC to write
DDRD=0xBF;	//set PORTD to write and PD6(ICP1) to input
PORTC=0x00;

/*The next line initializes and configures Timer/Counter Zero to Fast PWM, 8-bit; increment the counter 
register for a prescalar of one (every clock cycle); the Output Compare Mode is set to clear OCR0 on 
compare match and set OC0 at TOP.*/
	TCCR0=(1<<WGM00)|(1<<WGM01)|(1<<COM01)|(1<<CS00);

/*The next two lines initialize and configure Timer/Counter One to Normal PWM, 16-bit; increment the 
counter register for a prescalar of clk/256 cycle; Input Capture Noise Canceller is enabled and Input 
Capture Edge Select is enabled on rising (positive) edge; Timer/Counter Interrupt Mask Register (TIMSK) 
turns on Input Capture Interrupt Enabled.*/
	TCCR1B=(1<<ICES1)|(1<<CS10)|(1<<CS12)|(1<<ICNC1);
	TIMSK=(1<<TICIE1);
	
/*next two lines initialize and configure the ADC to convert the analog voltage at PA0 (ADC0); set 
reference voltage at +5V (AVCC); set the shift left enable bit to shift the converted voltage value to 
the left (used for 8-bit conversion accuracy); set ADC enable bit and ADC conversion bit to begin 
conversion; prescalar set to 128 division factor.*/
	ADMUX=(1<<REFS0)|(1<<ADLAR);						
	ADCSRA=(1<<ADEN)|(1<<ADSC)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);

sei();	//set global interrupt flag
while(1)
{	
	ADMUX=ADMUX&0xFE;	//set ADC to read and convert voltage at PA0 (ADC0)
	ADCSRA=ADCSRA|0x40;	//set ADSC bit to start conversion
	while(ADCSRA==0xC7);//loop and do nothing until ADC conversion has been made
	Deceleration=ADCH;	//store value of ADCH into Deceleration

/*Finite State Machine for comparing Deceleration and assigning the appropriate Power intensity to the 
PWM for LED brightness and for comparing Speed and assigning the appropriate Flash frequency for the LED 
flashing:The variable values are inverted(negative coefficient)that's why the decisions are backwards.*/	
if(Deceleration>GforceOne)
	{	/*Checks to see if Deceleration is greater than the first level of G-Force from the 
		Accelerometer, if true, then set LEDS to Off with OCR0 equal to 0% Duty Cycle, No Power.*/
		OCR0=PowerOff;
		PORTC=0x00;				
	}
else if((Deceleration<=GforceOne)&&(Deceleration>GforceTwo)&&(Speed>TwentyMPH))							
	{	/*Checks to see if Deceleration is less than or equal to the First level and is greater than
		the Second level of G-Force from the Accelerometer, and that the Speed is greater than 20mph,
		if true, then set LEDS to No Flashing constantly ON with OCR0 equal to 30% Duty Cycle.*/
		OCR0=Power30percent;
		PORTC=0xFF;		
	}
else if((Deceleration<=GforceOne)&&(Deceleration>GforceTwo)&&(Speed<=TwentyMPH)&&(Speed>FortyMPH))							
	{	/*Checks to see if Deceleration is less than or equal to the First level and is greater than
		the Second level of G-Force from the Accelerometer, and that the Speed is less than or equal
		to 20mph and is greater than 40mph, if true, then set LEDS to Flash at 4Hz with OCR0 equal to 
		30% Duty Cycle.*/		
		OCR0=Power30percent;
		PORTC=~PORTC;
		delay_ms(90);				
	}
else if((Deceleration<=GforceOne)&&(Deceleration>GforceTwo)&&(Speed<=FortyMPH)&&(Speed>SixtyMPH))							
	{	/*Checks to see if Deceleration is less than or equal to the First level and is greater than
		the Second level of G-Force from the Accelerometer, and that the Speed is less than or equal
		to 40mph and is greater than 60mph, if true, then set LEDS to Flash at 6Hz with OCR0 equal to 
		30% Duty Cycle.*/		
		OCR0=Power30percent;
		PORTC=~PORTC;
		delay_ms(60);				
	}
else if((Deceleration<=GforceOne)&&(Deceleration>GforceTwo)&&(Speed<=SixtyMPH))							
	{	/*Checks to see if Deceleration is less than or equal to the First level and is greater than
		the Second level of G-Force from the Accelerometer, and that the Speed is less than or equal
		to 60mph, if true, then set LEDS to Flash at 8Hz with OCR0 equal to 30% Duty Cycle.*/		
		OCR0=Power30percent;
		PORTC=~PORTC;
		delay_ms(45);				
	}
else if((Deceleration<=GforceTwo)&&(Deceleration>GforceThree)&&(Speed>TwentyMPH))
	{	/*Checks to see if Deceleration is less than or equal to the Second level and is greater than
		the Third level of G-Force from the Accelerometer, and that the Speed is greater than to 20mph, 
		if true, then set LEDS to No Flashing constantly ON with OCR0 equal to 65% Duty Cycle.*/			
		OCR0=Power65percent;	
		PORTC=0xFF;	
	}
else if((Deceleration<=GforceTwo)&&(Deceleration>GforceThree)&&(Speed<=TwentyMPH)&&(Speed>FortyMPH))
	{	/*Checks to see if Deceleration is less than or equal to the Second level and is greater than
		the Third level of G-Force from the Accelerometer, and that the Speed is less than or equal
		to 20mph and is greater than 40mph, if true, then set LEDS to Flash at 4Hz with OCR0 equal to 
		65% Duty Cycle.*/			
		OCR0=Power65percent;	
		PORTC=~PORTC;
		delay_ms(90);
	}
else if((Deceleration<=GforceTwo)&&(Deceleration>GforceThree)&&(Speed<=FortyMPH)&&(Speed>SixtyMPH))
	{	/*Checks to see if Deceleration is less than or equal to the Second level and is greater than
		the Third level of G-Force from the Accelerometer, and that the Speed is less than or equal
		to 40mph and is greater than 60mph, if true, then set LEDS to Flash at 6Hz with OCR0 equal to 
		65% Duty Cycle.*/			
		OCR0=Power65percent;	
		PORTC=~PORTC;
		delay_ms(60);
	}
else if((Deceleration<=GforceTwo)&&(Deceleration>GforceThree)&&(Speed<=SixtyMPH))
	{	/*Checks to see if Deceleration is less than or equal to the Second level and is greater than
		the Third level of G-Force from the Accelerometer, and that the Speed is less than or equal
		to 60mph, if true, then set LEDS to Flash at 8Hz with OCR0 equal to 65% Duty Cycle.*/			
		OCR0=Power65percent;	
		PORTC=~PORTC;
		delay_ms(45);
	}
else if((Deceleration<=GforceThree)&&(Speed>TwentyMPH))	
	{	/*Checks to see if Deceleration is less than or equal to the Third level of G-Force from the 
		Accelerometer and that the Speed is greater than 20mph, if true, then set LEDS to No Flashing
		constantly ON with OCR0 equal to 100% Full Power.*/
		OCR0=Power100percent;
		PORTC=0xFF;
	}
else if((Deceleration<=GforceThree)&&(Speed<=TwentyMPH)&&(Speed>FortyMPH))								
	{	/*Checks to see if Deceleration is less than or equal to the Third level of G-Force from the 
		Accelerometer, and that the Speed is less than or equal to 20mph and is greater than 40mph,
		if true, then set LEDS to Flash at 6Hz with OCR0 equal to 100% Duty Cycle, Full Power.*/			
		OCR0=Power100percent;
		PORTC=~PORTC;
		delay_ms(60);
	}
else if((Deceleration<=GforceThree)&&(Speed<=FortyMPH)&&(Speed>SixtyMPH))									
	{	/*Checks to see if Deceleration is less than or equal to the Third level of G-Force from the 
		Accelerometer, and that the Speed is less than or equal to 40mph and is greater than 60mph,
		if true, then set LEDS to Flash at 8Hz with OCR0 equal to 100% Duty Cycle, Full Power.*/			
		OCR0=Power100percent;
		PORTC=~PORTC;
		delay_ms(45);
	}
else if((Deceleration<=GforceThree)&&(Speed<=SixtyMPH))							
	{	/*Checks to see if Deceleration is less than or equal to the Third level of G-Force from the 
		Accelerometer and that the Speed is less than or equal to 60mph, if true, then set LEDS to 
		Flash at 8Hz with OCR0 equal to 100% Duty Cycle, Full Power.*/			
		OCR0=Power100percent;
		PORTC=~PORTC;
		delay_ms(45);
	}
}
}

/*Interrupt Service Routine for Timer/Counter One Input Capture.*/
ISR(TIMER1_CAPT_vect)
{	/*Stores old Speed (NOWspeed) value in a different register called PASTspeed.*/
	PASTspeed=NOWspeed;	
	/*Stores the Timer/Counter One's clock cycle number into a register called NOWspeed.*/
	NOWspeed=TCNT1;	
	/*Compares PASTspeed and NOWspeed to find which one is larger.*/	 
	if(PASTspeed<NOWspeed)
	{	/*Finds the difference in time from one Input Capture to the next, 
		and stores it in the register called Speed.*/
		Speed=NOWspeed-PASTspeed;
	}	
	else
	{	/*If PASTspeed is greater than NOWspeed, then Do Nothing; Speed stays the same.*/
		Speed=Speed;
	}		
}
