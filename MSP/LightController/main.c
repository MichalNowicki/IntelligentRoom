#include <msp430.h> 

// defines of bit numbering
#define bit0 0x01   // 1
#define bit1 0x02   // 2
#define bit2 0x04   // 4
#define bit3 0x08   // 8
#define bit4 0x10   // 16
#define bit5 0x20   // 32
#define bit6 0x40   // 64
#define bit7 0x80   // 128
// Signal definitions
#define ZERO bit0 // 2.0
#define STER bit1 // 2.1
#define MECH bit3 // 1.3 - ON/OFF mechanical
#define MECH2 bit4 // 2.4 - Light up/down in mechanical
// SPI - port 1
#define MISO bit7 // 1.7
#define MOSI bit6 // 1.6
#define SPI_CLK bit5 // 1.5
// global variables
// timer -> counts timer interrupts
// procentMocy -> responsible for setting the lightness of the bulb
int timer = 0;
int procentMocy = 50;
unsigned char z;

// Sets ports to in/out accordingly to their function
void init_IO()
{
	// Setting outputs
	P2DIR |= STER;

	// Setting ZERO pin interrupts (on rising edge)
	P2IE |= ZERO;

	// ZERO is active low, so react on high to low
	P2IES |= ZERO;

	// Clear all interrupt flags
	P2IFG = 0;
}

// Inits system clocks to have 1MHz -> important for timer
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

// Inits SPI
void init_SPI()
{
	// Starting configuration
	USICTL0 |= USISWRST;

	// Input enabled, Output enabled, SCK enabled
	USICTL0 |= USIPE7 + USIPE6 + USIPE5 + USIOE; // Port, SPI slave
	USICTL1 |= USIIE; // interrupt

	// now: CPOL = 0, CKPH = 0
	//USICKCTL |= USICKPL;
	USICTL1 |= USICKPH;

	// Ended configuration, START SPI
	USICTL0 &= ~USISWRST;

}

// Inits clocks, IO and timer and enter sleep mode LPM0 enabling global interrupts
int main(void)
{
	init_Clock();
	init_IO();
	init_Timer();

	init_SPI();
	USISRL = 0x00; // Clear receive buffer
	USICNT |= 0x08; // Waiting for 8 bits and then interrupt

	// Entering LPM0
	_BIS_SR(LPM0_bits + GIE);
	return 0;
}

// USI interrupt service routine
#pragma vector=USI_VECTOR
__interrupt void universal_serial_interface(void)
{
	// Reading from SPI
	z = USISRL;

	// just some borders, so we count at least once and
	// we have enough time to toggle the STER pin
	if (z > 99)
		z = 99;
	else if (z < 10)
		z = 0;

	// Storing the wanted percentage
	procentMocy = (int) z;

	// We stop counting as the percentage has changed
	TACTL = TASSEL_2 + MC_0;

	// Return (value+1), so I know it processed it correctly
	USISRL = z + 1;
	// Wait for next 8 bits
	USICNT |= 0x08;
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
		// MECH2 on -> +60%
		// Using all combinations, one can set {0, 30%, 60%, 90%} of power
		/*if ( (P1IN & MECH) && (P2IN & MECH2) )
		 {
		 procentMocy = 0;
		 }
		 else if ( P2IN & MECH2 )
		 {
		 procentMocy = 40;
		 }
		 else if (P1IN & MECH)
		 {
		 procentMocy = 70;
		 }
		 else
		 procentMocy = 99;*/

		// Start timer to control lightness when the wanted percent of power is more than 0
		if (procentMocy > 0)
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
	if (timer == (100 - procentMocy))
	{
		// STER UP -> active triac
		P2OUT |= STER;
	}
	else if (timer == (100 - procentMocy) + 1)
	{
		// STER DOWN -> turn down, but triac remains active
		P2OUT &= ~STER;
		timer = 0;
		// Stop counting
		TACTL = TASSEL_2 + MC_0;
	}
}
