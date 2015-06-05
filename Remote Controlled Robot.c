/*	
Remote Controlled Robot
Written by StanLee Maslonka

Program Purpose: Program robot to be controlled by a remote control in order to go forward, backward, left, or right.
Inputs: Infrared Detection by IR sensor outputting a logic one or logic zero. IR
		signals are detected from a 32-data-bit IR remote in this program.
Outputs: Voltage outputs to L293D Motor Driver (+5 or GND) from PORTB.
		 Outputs to chip then control rotation speed and direction of servo motors.
		 Output of data to LCD display via PORTC.
*/

//include files
#include <avr/io.h>
#include <inttypes.h>
#include <stdlib.h>
#include <math.h>
#include <MSOE/delay.c>
#include <avr/interrupt.h>
#include <MSOE/bit.c>
#include <MSOE/lcd.c>
#include <MSOE/lcdconf.h>

//defined numbers used for readability in the program
#define logical_one		14000	//17856
#define up_arrow		0b01100001101000000100001010111101
#define down_arrow		0b01100001101000001100001000111101
#define right_arrow		0b01100001101000001010100001010111
#define left_arrow		0b01100001101000000110100010010111
#define channel_up		0b01100001101000000101000010101111
#define channel_down	0b01100001101000001101000000101111
#define mute			0b01100001101000000111000010001111
#define on				0b00000000
#define off				0b00000001

//global variables
unsigned int remote[34];	//array of 34 unsigned integers
int bit;

//function prototypes
void forward(void);
void backward(void);
void right(void);
void left(void);
void stop(void);
unsigned long int decipher(unsigned int[]);
int slow_down(int);
int speed_up(int);
int toggle_lights(int);
void asmfunction(void);

int main(void)		//beginning of main function
{
	DDRA=asmfunction();	//set PORTA to write
	DDRB=0xFF;			//set PORTB to write
	DDRC=0xFF;			//set PORTC to write
	DDRD=0b10111011;	//set PIND2 and PIND6 to read and the rest of PORTD to write

	PORTB=PORTB&0b01111111;	//set PINB7 to logic zero (turn on green LED)
	PORTB=PORTB|0b01000000;	//set PINB6 to logic one (turn off red LED)
	PORTD=0b01000100;		//enable pull-up resistors of PIND2 and PIND6

	lcd_init();		//initialize LCD display
	lcd_clear();	//clear LCD display
	lcd_home();		//set cursor of LCD to the first character position

	/*The next two lines configure T/C0 and T/C2 to set OCR0 and OCR2, respectively on compare match when
	  the T/C subsystem's respective counting register is counting up and to clear OCR0 and OCR2 when counting
	  down. Configured as Fast PWM, Phase-Correct with a prescalar of one.*/
	TCCR2=(1<<WGM20)|(1<<COM21)|(1<<COM20)|(1<<CS20);
	TCCR0=(1<<WGM00)|(1<<COM01)|(1<<COM00)|(1<<CS00);

	/*The next two lines configure TCNT1 to increment every clock cycle; Configure Enable Input Capture Interrupt
	  to trigger on rising edge; Enable Input Capture Noise Canceller; Locally enable T/C 1 Input Capture Interrupt
	  and T/C 1 Overflow Interrupt.*/
	TCCR1B=(1<<ICES1)|(1<<CS10)|(1<<ICNC1);
	TIMSK=(1<<TICIE1)|(1<<TOIE1);

	MCUCR=(1<<ISC00)|(1<<ISC01);//configure external interrupt 1 to trigger on rising edge
	GICR=(1<<INT0);				//locally enable external interrupt 1

	sei();	//set global interrupt flag

	unsigned long int button=0;	//unsigned long integer (32 bits--sent by IR remote)
	uint8_t led=0;				//state of LED (used to confirm 34-bit transmission by IR remote)
	int speed=0;				//speed of motors at maximum
	int lights=off;

	lcd_printf("Waiting");		//print "Waiting" on LCD (wait for button to be pushed on IR remote)

	PORTB=PORTB|0b00000010;

	while(1)	//infinite loop
	{
		if(bit==34)	//wait until bit=34--Input Capture ISR called 34 times (1 start bit, 32 data bits, and 1 stop bit sent by IR remote)
		{
			button=decipher(remote);	//decipher the 32 data bits as either 1 or 0 depending on TCNT1 values;
										//pass remote as parameter and set button equal to return value
			bit=0;						//reset bit equal to 0 (prepare to receive a new command)

			if(led==0)	//if the state of led is 0, turn off green LED and turn on red LED
			{			//then switch the state of led to 1
				PORTB=PORTB&0b10111111;
				PORTB=PORTB|0b10000000;
				led++;
			}
			else		//if the state of led is not 0 (state is 1), turn off red LED and turn on green LED
			{			//then switch the state of led to 0
				PORTB=PORTB&0b01111111;
				PORTB=PORTB|0b01000000;
				led--;
			}
			switch(button)	//check the value of button (button that was pushed on remote)
			{
				case up_arrow:			//if up arrow was pushed, make robot go forward
							forward();
							break;
				case down_arrow:		//if down arrow was pushed, make robot go backward
							backward();
							break;
				case left_arrow:		//if left arrow was pushed, make robot go left
							left();
							break;
				case right_arrow:		//if right arrow was pushed, make robot go right
							right();
							break;
				case channel_up:		//if channel up was pushed, speed robot up (speed is parameter sent to speed_up function); speed equals returned value
							speed=speed_up(speed);
							break;
				case channel_down:		//if channel down was pushed, slow robot down (speed is parameter sent to speed_down function); speed equals returned value
							speed=slow_down(speed);
							break;
				case mute:				//if mute was pushed, toggle lights from off to on or on to off; pass lights as parameter and set lights equal to returned value
							lights=toggle_lights(lights);
							break;
				default:				//if any other button was pushed, stop robot
							stop();
							break;
			}
		OCR0=speed;		//set the speed of the left motor depending on value of speed
		OCR2=speed;		//set the speed of the right motor depending on value of speed
		}
	}
}

ISR(TIMER1_CAPT_vect)		//T/C 1 Input Capture ISR
{
		remote[bit]=ICR1;	//set value in remote array at address specified by bit equal to ICR1
		TCNT1=0;			//reset TCNT1 to zero
		bit++;				//increment bit by one
}

unsigned long int decipher(unsigned int array[])	//beginning of decipher function (unsigned int array parameter)
{
	unsigned long int y=0;		//initialize y to be 0
	unsigned int time=0;		//initialize time to be 0

	for(int x=2;x<34;x++)		//initialize x to be 2, and increment x by one every time the loop completes; end loop when x=34
	{
		y=y*2;					//multiply y by 2 (shift bits left)
		time=array[x];			//time equals the value in the array at address specified by x

		if(time>logical_one)	//if time is greater than logic_one, increment y by one
			y++;
	}

return y;						//return the value of y to main function

}

void forward(void)			//beginning of forward function
{
	lcd_clear();			//clear LCD display
	lcd_home();				//set cursor of LCD to the first character position
	lcd_printf("Forward");	//print "Forward" on LCD display
	PORTA=0;				//stop motors for 20 milliseconds
	delay_ms(20);
	PORTA=0b00000101;		//turn both motors forward
}

void backward(void)			//beginning of backward function
{
	lcd_clear();			//clear LCD display
	lcd_home();				//set cursor of LCD to the first character position
	lcd_printf("Reverse");	//print "Reverse" on LCD display
	PORTA=0;				//stop motors for 20 milliseconds
	delay_ms(20);
	PORTA=0b00001010;		//turn both motors backward
}

void left(void)				//beginning of left function
{
	lcd_clear();			//clear LCD display
	lcd_home();				//set cursor of LCD to the first character position
	lcd_printf("Left");		//print "Left" on LCD display
	PORTA=0;				//stop motors for 20 milliseconds
	delay_ms(20);
	PORTA=0b00000100;		//stop left motor and turn right motor forward
}

void right(void)			//beginning of right function
{
	lcd_clear();			//clear LCD display
	lcd_home();				//set cursor of LCD to the first character position
	lcd_printf("Right");	//print "Right" on LCD display
	PORTA=0;				//stop motors for 20 milliseconds
	delay_ms(20);
	PORTA=0b00000001;		//stop right motor and turn left motor forward
}

void stop(void)				//beginning of stop function
{
	lcd_clear();			//clear LCD display
	lcd_home();				//set cursor of LCD to the first character position
	lcd_printf("Stopped");	//print "Stopped" on LCD display
	PORTA=0;				//stop motors
}

int speed_up(int status)	//beginning of speed_up function; integer parameter status
{
	if(status>=10)			//if status is greater than or equal to 10, decrement status by 10
	status-=10;
	return status;			//return status to main function
}

int slow_down(int status)	//beginning of slow_down function
{
	if(status<=60)			//if status is less than or equal to 60, increment status by 10
	status+=10;
	return status;			//return status to main function
}

int toggle_lights(int status)	//beginning of toggle_lights function; integer parameter status
{
	if(status==off)				//if status is off, turn lights on and switch status to on
	{
		PORTB=PORTB&0b11111101;
		status=on;
	}
	else						//if status is not off (on), turn lights off and switch status to off
	{
		PORTB=PORTB|0b00000010;
		status=off;
	}
	return status;				//return status to main function
}

ISR (INT0_vect)				//external interrupt 1 ISR
{
	lcd_clear();			//clear LCD display
	lcd_home();				//set cursor of LCD to the first character position
	lcd_printf("Stopped");	//print "Stopped" on LCD display
	PORTA=0;				//stop motors
}

ISR(TIMER1_OVF_vect)		//T/C 1 overflow ISR
{
	bit=0;					//reset bit to 0 (in case of invalid rising edge triggering Input Capture Interrupt)
}

