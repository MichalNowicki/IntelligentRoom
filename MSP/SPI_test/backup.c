#include <msp430.h>

#define bit0 0x01   // 1
#define bit1 0x02   // 2
#define bit2 0x04   // 4
#define bit3 0x08   // 8
#define bit4 0x10   // 16
#define bit5 0x20   // 32
#define bit6 0x40   // 64
#define bit7 0x80   // 128

// SPI - port 1
#define MISO bit6
#define MOSI bit7
#define SPI_CLK bit5

// Signal definitions
#define ZERO bit0 // 2.0
#define STER bit1 // 2.1
#define MECH bit3 // 2.3 - ON/OFF mechanical
#define MECH2 bit4 // 2.4 - Light up/down in mechanical

// global variables
// timer -> counts timer interrupts
// procentMocy -> responsible for setting the lightness of the bulb
volatile int timer = 0;
volatile int procentMocy = 50;


void init_SPI()
{
	// Starting configuration
	UCB0CTL1 |= UCSWRST;

	P1SEL = MISO | MOSI |  SPI_CLK;
	P1SEL2 = MISO | MOSI |  SPI_CLK;

	// MISO | MOSI | SPI enable, slave
	UCB0CTL0 |= UCCKPH + UCMSB /*+ UCSYNC*/+ UCMODE_0;

	//UCA0CTL1 |= UCSSEL_2;

	// Ended configuration, START SPI
	UCB0CTL1 &= ~UCSWRST;

}

void init_Clock()
{
	// Stop watchdog timer
	WDTCTL = WDTPW | WDTHOLD;

	// Setting clock: 1 MHz at DCO=4 & RSELx = 7
	DCOCTL = 0x80; // DC0 = 4
	BCSCTL1 = bit7 | bit2 | bit1 | bit0; // external off, RSELx = 7
	BCSCTL2 |= bit2; // SMCLK = DCOCLK / 4 -> 250000 per sec
}

// Inits timer and is responsible for delayed triac activation
void init_Timer()
{
	// Timer counted interrupt enabled: CCR0 interrupt
	CCTL0 = CCIE;
	CCR0 = 25; // 10000 interrupts per sec
	TACTL = TASSEL_2 + MC_0; // SMCLK timer, STOPPED
}

// Sets ports to in/out accordingly to their function
void init_IO()
{
	// Setting outputs
	P2DIR |= STER;

	// Setting MECH/MECH2/ZERO pin interrupts (on rising edge)
	P2IE |= MECH | MECH2 | ZERO;

	// ZERO is active low, so react on high to low
	P2IES |= ZERO;

	// Clear all interrupt flags
	P2IFG = 0;
}

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer

    init_Clock();
    init_IO();

    init_SPI();
    init_Timer();

    IE2 |= UCB0RXIE; // enable rx interrupt
    IE2 |= UCB0TXIE; // enable rx interrupt

    // Entering LPM0
    _BIS_SR(LPM0_bits + GIE);

	return 0;
}

// SPI echo character
#pragma vector=USCIAB0TX_VECTOR
__interrupt void USCI0TX_ISR (void)
{

}

// SPI echo character
#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR (void)
{
	volatile char rdata = UCB0RXBUF;
	if ( rdata  == 'U')
	{
		procentMocy += 30;
		if (procentMocy > 90)
			procentMocy = 90;
	}
	else if ( rdata == 'D')
	{
		procentMocy -= 30;
		if (procentMocy < 0)
			procentMocy = 0;
	}
	else
	{
		if (procentMocy > 0)
			procentMocy = 0;
		else
			procentMocy = 90;
	}
}

// Port 2 interrupt service routine
// Responsible for handling zero detection
//
#pragma vector=PORT2_VECTOR
__interrupt void Port_2_interrupts(void)
{
	if (P2IFG & ZERO)
	{
		TACTL = TASSEL_2 + MC_0;

		// Sets the power accordingly to the position of mechanical switches
		// MECH on -> +30%
		// MECH2 on -> + 60%
		// Using all combinations, one can set {0, 30%, 60%, 90%} of power
		/*procentMocy = 0;
		if ( P2IN & MECH )
		{
			procentMocy += 30;
		}
		if ( P2IN & MECH2 )
		{
			procentMocy += 60;
		}*/
		// Start timer to control lightness when the wanted percent of power is more than 0
		if ( procentMocy > 0 )
			TACTL = TASSEL_2 + MC_1;

		// Changing rising/falling edge interrupt detection
		P2IES ^= ZERO;
		// ZERO IFG cleared
		P2IFG &= ~ZERO;
	}
}

//Timer interrupt vector handler
// Handler the triac controling pin
// 10000 interrupts per sec -> 200 per one sin period -> 100 per half of sin period
// The delay of triac activating depends on procentMocy's value
#pragma vector=TIMER0_A0_VECTOR
__interrupt void TimerA_procedure(void)
{
	timer++;
	if (timer == ( 100 - procentMocy ))
	{
		// STER UP -> active triac
		P2OUT |= STER;
	}
	if ( timer == ( 100 - procentMocy ) + 1)
	{
		// STER DOWN -> turn down, but triac remains active
		P2OUT &= ~STER;
		timer = 0;
		TACTL = TASSEL_2 + MC_0;
	}
}

