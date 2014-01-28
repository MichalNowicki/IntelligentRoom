/*
 * RF_Tranceiver.c
 *
 * Created: 2012-08-10 15:24:35
 *  Author: Kalle
 */ 

#include <avr/io.h>
#include <stdio.h>
#define F_CPU 8000000UL  // 8 MHz
#include <util/delay.h>
#include <avr/interrupt.h>

#include "nrf.h"	//inkluderar namnen på bitarna i nrf'en

#define RxTx_start TRANSMITTER	//ange om enheten är Receiver eller Transmitter till att börja med. 
#define dataLen 1				//längd på datapacket som skickas/tas emot (samma på transmitter & reciver!!!)

volatile uint8_t RxTx;	//Global variabel som håller reda på om sändare/mottagare (volatile för att annars skrivs den över och blir fel...)
uint8_t *data;			//global pekare till en array som krävs när man ska läsa data ex: data=WriteToNrf(R, R_RX_PAYLOAD, val, dataLen); //där val måste vara en riktig array å data en pekare till en!
uint8_t val[dataLen];	//global array som kan skrivas till nrfen ex: WriteToNrf(W, CONFIG, Val, 1);	(1 eller datalen...)
volatile uint8_t retr;			//global variabel som håller antaler retryes i Ver_retr_transmit();

/*****************ändrar klockan till 8MHz ist för 1MHz*****************************/
void clockprescale(void)	
{
	CLKPR = 0b10000000;	//initiera ändring av klockprescaler (CLKPCE=1 och resten nollor)
	CLKPR = 0b00000000;	//Skriv önskad klockprescaler (CLKPCE=0 och de fyra första bitarna CLKPS0-3 ger önskad prescale)
}
////////////////////////////////////////////////////


/*****************USART*****************************/  //Skickar data från chip till com-port simulator på datorn
//Initiering
void usart_init(void)	
{
	DDRD |= (1<<1);	//Sätt TXD (portD1) för att möjliggöra usart
	
	unsigned int USART_BAUDRATE = 9600;		//ställs in i terminal.exe (google å ladda hem terminal.exe)
	unsigned int ubrr = (((F_CPU / (USART_BAUDRATE * 16UL))) - 1);	//baud prescale beräknas beroende på F_CPU
	
/*Set baud rate */
	UBRRH = (unsigned char)(ubrr>>8);
	UBRRL = (unsigned char)ubrr;
	
/*	Enable receiver and transmitter */
	UCSRB = (1<<RXEN)|(1<<TXEN);

/* Set frame format: 8data, 2stop bit, kom ihåg att gå in i enhetshanteraren å trycka avancerat på comporten å välj 2 stoppbitar*/
	UCSRC = (1<<USBS)|(3<<UCSZ0);
	
}

//Funktionen som skickar iväg byten till datorn
void USART_Transmit(uint8_t data)	
{
/* Wait for empty transmit buffer */
	while ( !( UCSRA & (1<<UDRE)) );
/* Put data into buffer, sends the data */
		UDR = data;
}

//Funktionen som Tar emot kommandon av datorn som senare ska skickas med transmittern OBS: enabla USART_interrupt_init(); så körs interruptet när data tas emot!
uint8_t USART_Receive( void )
{
/* Wait for data to be received */	
	while ( !(UCSRA & (1<<RXC)) );	//Detta behövs egentligen inte  då Interruptet USART_RX kallar på denna funktion när just RXC0 gått hög...
/* Get and return received data from buffer */
	
	return UDR;
}


/*****************SPI*****************************/  //Skickar data mellan chip och nrf'ens chip
//initiering
void InitSPI(void)
{
	//Set SCK (PB5), MOSI (PB3) , CSN (SS & PB2) & CE (PB1) as outport OBS: koppla MOSI:MO & MISO:MI (tvärt om på attiny med simulerad spi)
	//OBS!!! Måste sättas innan SPI-Enable neadn
	DDRB |= (1<<DDB5) | (1<<DDB3) | (1<<DDB2) |(1<<DDB1);
//	DDRC |= (1<<DDC5) | (1<<DDC4); //  PWR och TXE
	
	/* Enable SPI, Master, set clock rate fck/16 .. kan ändra hastighet utan att det gör så mycket*/
//	SPCR |= (1<<SPE)|(1<<MSTR);// |(1<<SPR0) |(1<<SPR1);
	
	SETBIT(PORTB, 2);	//CSN high to start with, vi ska inte skicka nåt till nrf'en ännu!
	CLEARBIT(PORTB, 1);	//CE low to start with, nrf'en ska inte sända/ta emot nåt ännu!
//	CLEARBIT(PORTC, 4);	//TXE low
//	SETBIT(PORTC, 5); //sätter på Power...

}

//Skickar kommando till nrf'en å får då tillbaka en byte
char WriteByteSPI(unsigned char cData)
{
	//Load byte to Data register
//	SPDR = cData;	
		
	/* Wait for transmission complete */
//	while(!(SPSR & (1<<SPIF)));
	
	//Returnera det som sänts tillbaka av nrf'en (första gången efter csn-låg kommer Statusregistert)
//	return SPDR;
}
////////////////////////////////////////////////////


/*****************in/out***************************/  //ställ in t.ex. LED
//sätter alla I/0 portar för t.ex. LED
void ioinit(void)			
{
	DDRB |= (1<<DDB0);		//  LED as output (visar om data mottagits)
}
////////////////////////////////////////////////////


/*****************interrupt***************************/ //orsaken till att köra med interrupt är att de avbryter koden var den än är och kör detta som är viktigast!

//när data tas emot/skickas så går interruptet INT0 näst längst ner igång
void INT0_interrupt_init(void)
{
	DDRD &= ~(1<<DDD2);	//Extern interrupt på INT0, dvs sätt den till input!
	
//	EICRA |=  (1<<ISC01) | (1<<ISC00);// INT0 rising edge	PD2
	//EICRA  &=  ~(1<<ISC00);// INT0 falling edge	PD2

//	EIMSK |=  (1<<INT0);	//enablar int0
	//sei();              // Enable global interrupts görs sen
}

//när chipets RX (usart) får ett meddelande fårn datorn går interruptet USART_RX igång längst ner.
void USART_interrupt_init(void)
{
	UCSRB |= (1<<RXCIE);	//Enable interrupt när USART-data mottages,
}
//////////////////////////////////////////////////////

/*****************nrf-setup***************************/ //Ställer in nrf'en genoma att först skicka vilket register, sen värdet på registret.
void nrf905_init(void)
{
	//Frekvensen
	_delay_us(10);		//alla delay är så att nrfen ska hinna med! (microsekunder)
	CLEARBIT(PORTB, 2);	//CSN low = nrf-chippet börjar lyssna
	_delay_us(10);
	WriteByteSPI(0x00);	//första SPI-kommandot efter CSN-låg berättar för nrf'en vilket av dess register som ska läsas/redigeras ex: 0x01 = CH_NO -registret (bla frekvensen)
	_delay_us(10);
	WriteByteSPI(0x73);	//fRF = ( 422.4 + CH_NOd/10)*(1+HFREQ_PLLd) MHz //CH:NOd = 115 ger 433,9hz (115 i dec=0x73 i hex) (HFREQ_PLLd=0)
	_delay_us(10);
	SETBIT(PORTB, 2);
	
	//TX & RX address width
	_delay_us(10);
	CLEARBIT(PORTB, 2);
	_delay_us(10);
	WriteByteSPI(0x02);
	_delay_us(10);
	WriteByteSPI(0x44);	//bit 4-6 = TX, bit 0-2=RX (001=1byte, 100=4byte) dvs, "(0)001 (0)001"
	_delay_us(10);
	SETBIT(PORTB, 2);
	
	//RX address
	_delay_us(10);
	CLEARBIT(PORTB, 2);
	_delay_us(10);
	WriteByteSPI(0x05);
	_delay_us(10);
	WriteByteSPI(0xe7);	//Adressen = 0000 0001	//gör en loop å skriv ut 4ggr om väljer 4byte adress!
	_delay_us(10);
	WriteByteSPI(0x7e);	//Adressen = 0000 0001	//gör en loop å skriv ut 4ggr om väljer 4byte adress!
	_delay_us(10);
	WriteByteSPI(0x7e);	//Adressen = 0000 0001	//gör en loop å skriv ut 4ggr om väljer 4byte adress!
	_delay_us(10);
	WriteByteSPI(0xe7);	//Adressen = 0000 0001	//gör en loop å skriv ut 4ggr om väljer 4byte adress!
	_delay_us(10);
	SETBIT(PORTB, 2);
	
	//TX address
	_delay_us(10);
	CLEARBIT(PORTB, 2);
	_delay_us(10);
	WriteByteSPI(0x22);
	_delay_us(10);
	WriteByteSPI(0xe7);	//Adressen = 0000 0001	//gör en loop å skriv ut 4ggr om väljer 4byte adress!
	_delay_us(10);
	WriteByteSPI(0x7e);	//Adressen = 0000 0001	//gör en loop å skriv ut 4ggr om väljer 4byte adress!
	_delay_us(10);
	WriteByteSPI(0x7e);	//Adressen = 0000 0001	//gör en loop å skriv ut 4ggr om väljer 4byte adress!
	_delay_us(10);
	WriteByteSPI(0xe7);	//Adressen = 0000 0001	//gör en loop å skriv ut 4ggr om väljer 4byte adress!
	_delay_us(10);
	SETBIT(PORTB, 2);
	
	//RX Payload width
	_delay_us(10);
	CLEARBIT(PORTB, 2);
	_delay_us(10);
	WriteByteSPI(0x03);
	_delay_us(10);
	WriteByteSPI(0x01);	//0x20 = 32 i decimal... 32byte
	_delay_us(10);
	SETBIT(PORTB, 2);
	
	//TX Payload width
	_delay_us(10);
	CLEARBIT(PORTB, 2);
	_delay_us(10);
	WriteByteSPI(0x04);
	_delay_us(10);
	WriteByteSPI(0x01);	//0x20 = 32 i decimal... 32byte
	_delay_us(10);
	SETBIT(PORTB, 2);
	
	//
	////Sätt TX Payload
	//_delay_us(10);
	//CLEARBIT(PORTB, 2);
	//_delay_us(10);
	//WriteByteSPI(0x20);
	//_delay_us(10);
	//WriteByteSPI(0xee);	//Packetet
	//_delay_us(10);
	//SETBIT(PORTB, 2);
	
	//Sätt output power
	_delay_us(10);
	CLEARBIT(PORTB, 2);
	_delay_us(10);
	WriteByteSPI(0x01);
	_delay_us(10);
	WriteByteSPI(0b00001100);	//höjer strömen till +10db...
	_delay_us(10);
	SETBIT(PORTB, 2);
		
	//Sätt [7] CRC_mode on, [6] 16bit crc, [5,4,3]XOF=011=16Mhz(kristallen), [2]UP CLK en = 0=off (om extern kristall på "NC=uCK"-benet står olika på nrferna), [1-0] UP clk frek....
	_delay_us(10);
	CLEARBIT(PORTB, 2);
	_delay_us(10);
	WriteByteSPI(0x09);
	_delay_us(10);
	WriteByteSPI(0b11011000);	//höjer strömen till +10db...
	_delay_us(10);
	SETBIT(PORTB, 2);
		
	//
	////Sätt av crc
	//_delay_us(10);
	//CLEARBIT(PORTB, 2);
	//_delay_us(10);
	//WriteByteSPI(0x09);
	//_delay_us(10);
	//WriteByteSPI(0xA7);	//Packetet 1010 0111
	//_delay_us(10);
	//SETBIT(PORTB, 2);
	
}



void Skickadata(void)
{
	SETBIT(PORTB, 1);	//CE Hög, skicka
//	SETBIT(PORTC, 4);	//TXE Hög, Skicka
	//once we start sending the packet, we wait for the DR pin to go high
	//indicating that the data is finished sending
	//Så man hade kunnat göra det ist för att bara vänta lite...
	_delay_us(100);
	CLEARBIT(PORTB, 1);	//CE låg
//	CLEARBIT(PORTC, 4);	//TXE låg
}

void Mottagdata(void)
{
//	CLEARBIT(PORTC, 4); //Ser till att TXE fortfarande är låg (onödigt)
	SETBIT(PORTB, 1); //CE hög (då txe=låg => receive)	
	
	_delay_ms(1000); //Tryck på först den andra nrfen sen, fjärrisen!
	
	
	CLEARBIT(PORTB, 1); //CE låg igen = sluta lyssna





//_delay_us(10);
//CLEARBIT(PORTB, 2);	//CSN låg = spi
//_delay_us(10);
//WriteByteSPI(0x24);
//_delay_us(10);
//uint8_t test;
//test = WriteByteSPI(0x00);
//
//_delay_us(10);
//SETBIT(PORTB, 2); //Csn hi
//
//USART_Transmit(test);


}

//initierar nrf'en (obs nrfen måste vala i vila när detta sker - CE-låg)


/////////////////////////////////////////////////////



/*****************Funktioner***************************/ //Funktioner som används i main

int main(void)
{
	clockprescale();		//sätter klockan till 8Mhz, får inte usart att prata med datorn annars, nrfen behöver det inte...
	usart_init();			//starta kommunikation med datorn
	InitSPI();				//Startar kommunikation chip-nrf
    ioinit();				//Initierar alla i/o portar för LED mm...
	INT0_interrupt_init();	//initierar interrupt som går igång när data mottagits eller en lyckad sändning utförd (efter den fått ett automatiskt svar om EN_AA och MASK_TX_DS är satta)
	//USART_interrupt_init(); //Om denna är igång MÅSTE usart-usb vara inkopplad annars går interruptet igång å ser ut som att inget händer...
	
	nrf905_init();		//initierar nrf'en å avslutar med "power up"

	SETBIT(PORTB,0);		//För att se att dioden fungerar!
	_delay_ms(1000);
	CLEARBIT(PORTB,0);
	
	sei();	//Enable global interrupt annars går inte usart-interruptet igång nedan...

	while(1)	//loop forever
	{

		Mottagdata();
		
	

	}
	return 0;
}
   
ISR(INT0_vect)	//vektorn som går igång när CD (carrier detect) blir hög = alla bytes är mottagna (Annars finns AM som blir hög direkt när adress match finns)
{
	SETBIT(PORTB,0);
	//R_RX_Payload (Läs ut mottagen data)
	_delay_us(10);
	CLEARBIT(PORTB, 2);	//CSN låg = spi
	_delay_us(10);
	WriteByteSPI(0x24);
	_delay_us(10);
		
	//uint8_t payload[dataLen];
	//for(uint8_t i = 0; i < dataLen; ++ i)
	//{
		//payload[i] = WriteByteSPI(0x00);
		//USART_Transmit(payload[i]);
	//}
	uint8_t test;
	test = WriteByteSPI(0x00);
	USART_Transmit(test);
	_delay_us(10);
	SETBIT(PORTB, 2); //Csn low
		
		

	USART_Transmit('#');
	_delay_ms(500);
	CLEARBIT(PORTB,0);
}

ISR(USART_RX_vect)	///vektorn som går igång när nåt har skickats från datorn till USART RX
{
	
	
}