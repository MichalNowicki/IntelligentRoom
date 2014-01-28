/*
 * IntelligentRoom.cpp
 *
 * Created: 2013-12-28 01:40:04
 *  Author: sMi
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#define F_CPU 4000000

#define STER 5

#define TIMER_LIGHT_START TCCR1B = (1<<CS12) // CLK/256
#define TIMER_LIGHT_CLEAR TCNT1 = 0x0000
#define TIMER_LIGHT_STOP  TCCR1B &= ~(1<<CS12)

#define TIMER_SPIKE_START TCCR0B |= (1<<CS02) // CLK/256
#define TIMER_SPIKE_CLEAR TCNT0 = 0x00
#define TIMER_SPIKE_STOP  TCCR0B &= ~(1<<CS02)

uint16_t value;
unsigned char data;
int toSend;

// An interrupt on PD3 - zero crossing
SIGNAL(INT1_vect)
{
	// Current counting value
	if ( value > 0 )
	{
		OCR1A = value;
		
		// Waiting some time to activate optotriac
		TIMER_LIGHT_CLEAR;
		TIMER_LIGHT_START;
	}
	
}

// Activating the optotriac
SIGNAL (TIMER1_COMPA_vect)
{
	// Activating opto-triac
    PORTD |= (1<<STER);
	
	// We will stop activation of opto-triac in another timer
	TIMER_SPIKE_CLEAR;
	TIMER_SPIKE_START;
	
	// Stopping the count of delay
	TIMER_LIGHT_STOP;
}

// Stopping the activation of the optotriac after some time
SIGNAL (TIMER0_COMPA_vect) {
	// Turn off optotriac signaling
	PORTD &= ~(1<<STER);
	
	// Stopping timer
	TIMER_SPIKE_STOP;
}

SIGNAL(USART_RX_vect){

	data = UDR;
	
	if ( data < 5)
		data = 0;
	else if (data > 99)
		data = 99;
	
	if ( data > 0)
		value = (100-data * 1.3 + 33);
	else
		value = 0;
	toSend = 1;
}


inline void timerLightSetup()
{
	// Timer to control the percentage of lightness
	// Setting CTC mode
	TCCR1B |= (1 << WGM12);
	
	// 15625 cycles per 1s
	// 156.25 per 10 ms
	// 50% lightness
	value = 78;
	OCR1A = value;
	
	// Enable interrupt
	TIMSK |= (1 << OCIE1A);
}

inline void timerDelaySetup()
{
	// Timer controlling the spike time activating optotriac
	// Setting CTC mode
	TCCR0A |= (1 << WGM01);
	
	// 156.25 per 10 ms
	OCR0A = 20;
	
	// Enable interrupt
	TIMSK |= (1 << OCIE0A);
}

inline void portSetup()
{
	// Port D6 as output
	DDRD = (1<<6) | (1<<STER);
	PORTD = (1<<6);// | (1<<STER);
	
	// PD3 (INT1 pin) is now an input
	DDRD &= ~(1 << DDD3);
	// PD3 is now an input with pull-up enabled
	PORTD |= (1 << PORTD3);
	
	// Lightness control
	DDRD &= ~(1 << DDD6);
	DDRB &= ~(1 << DDB0);
	DDRB &= ~(1 << DDB1);
}

inline void externalInterruptSetup()
{	
	// Rising edge interrupt
	MCUCR |= (1<<ISC10) | (1<<ISC11);
	// Enabling external interrupt
	GIMSK |= (1<<INT1);
}


void USART_Init(uint16_t ubrr_value)
{
	/* Set baud rate */
	UBRRH = (unsigned char) (ubrr_value >> 8);
	UBRRL = (unsigned char) ubrr_value;
	UCSRA = (1<<U2X);
	// Asyn communication, 8 bit, 1 stop bit
	UCSRC = (3<<UCSZ0);
	/* Enable receiver and transmitter */
	UCSRB = (1<<RXEN)|(1<<TXEN);
	
	UCSRB |= (1<<RXCIE); //RECV INTERRUPT ENABLE 
}

void USART_Transmit( unsigned char data )
{
	/* Wait for empty transmit buffer */
	while ( !( UCSRA & (1<<UDRE)) );
	/* Put data into buffer, sends the data */
	UDR = data;
}

/*unsigned char USART_Receive( void )
{
	while ( !(UCSRA & (1<<RXC)) );
	return UDR;
}*/

int main(void)
{
	// Setups
	portSetup();
	externalInterruptSetup();
	timerLightSetup();
	timerDelaySetup();
	//9600 
	USART_Init(51);
	
	toSend = 0;
	// Enable global interrupt
	sei();
	
    while(1)
	{
		if ( toSend == 1)
		{
			USART_Transmit(data);
			toSend = 0;
		}
		//USART_Transmit('[');
		//USART_Transmit(data);
		//USART_Transmit(']');
		_delay_ms(100);
		/*if ( (PINB & ( 1<<0 )) && ( PINB & ( 1<<1 ) )) 
		{
			PORTD |= (1<<1);
			value = 100;
		}
		else if (PINB & ( 1<<0 ))
		{
			PORTD &= ~(1<<1);
			value = 85;
		}
		else if (PINB & ( 1<<1 ))
		{
			PORTD &= ~(1<<1);
			value = 70;
		}
		else
		{
			value = 0;	
		}*/
	}
}