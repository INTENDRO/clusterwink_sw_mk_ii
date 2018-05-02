/*
 * clusterwink_mk_ii.c
 *
 * Created: 09/04/2018 16:52:30
 * Author : Mario
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include "utils.h"
#include "spi.h"
#include "ringbuffer.h"
#include "rgbooster.h"

#define STATUS_PLED		0
#define STATUS_AUDIO	1

#define INT_OUT

#define LED_COUNT		3
// volatile uint8_t aucRed[LED_COUNT] =		{0xFF,0xA3,0xE5}; //data buffer for RGB leds
// volatile uint8_t aucGreen[LED_COUNT] =		{0x12,0x34,0x56};
// volatile uint8_t aucBlue[LED_COUNT] =		{0xC3,0x87,0xE1};
	
volatile uint8_t aucRed[LED_COUNT] =		{0xFF,0x00,0x00}; //data buffer for RGB leds
volatile uint8_t aucGreen[LED_COUNT] =		{0x00,0xFF,0x00};
volatile uint8_t aucBlue[LED_COUNT] =		{0x00,0x00,0xFF};
volatile uint8_t ucRGBIdx = LED_COUNT;		// RGB ISR variables
volatile uint8_t ucByteIdx = 0;

volatile uint8_t u8PLEDFadeMinPercent = 0;
volatile uint8_t u8PLEDFadeMaxPercent = 0;
volatile uint8_t u8PLEDFadeTime = 0;
volatile uint16_t u16PLEDFadeMaxValue = 0;
volatile uint16_t u16PLEDFadeMinValue = 0;
volatile uint16_t u16PLEDFadeCurrValue = 0;
volatile uint32_t u32PLEDFadeIntStep = 0;
volatile uint32_t u32PLEDFadeIntCount = 0;
volatile uint8_t u8PLEDFadeActive = 0;



static RingBuff_t RINGBUFFER;
static SpiBuf_t SPIBUFFER;


volatile uint8_t u8Status = 0x00;
volatile uint8_t u8Duty = 0;

ISR(TIMER2_COMPA_vect)
{
	#ifdef INT_OUT
	PORTD |= (1<<PORTD1);
	#endif

	if(u8PLEDFadeActive)
	{
		u32PLEDFadeIntCount++;
		if(u32PLEDFadeIntCount>=u32PLEDFadeIntStep)
		{
			u32PLEDFadeIntCount = 0;
			u16PLEDFadeCurrValue++;
			setPWMDuty(u16PLEDFadeCurrValue);
			u8Duty = Map(u16PLEDFadeCurrValue,0,511,0,100);

			if(u16PLEDFadeCurrValue>=u16PLEDFadeMaxValue)
			{
				u8PLEDFadeActive = 0;
			}
		}
	}

	#ifdef INT_OUT
	PORTD &= ~(1<<PORTD1);
	#endif
}


ISR(INT0_vect)	// external interrupt (handshake from RGBooster board)
{				// start RGBooster send sequence: reset "ucRGBIdx" and "ucByteIdx" to zero. then start with calling the ISR directly "INT1_vect();"
	#ifdef INT_OUT
	PORTD |= (1<<PORTD1);
	#endif
	
	if(ucRGBIdx<(LED_COUNT))
	{
		switch(ucByteIdx) // red green and blue are sent in 3 separate bytes. this variable remembers the next color to be sent
		{
			case 0:
			PORT_DATA_HIGH = (PORT_DATA_HIGH & ~DATA_HIGH_BITMASK) | (aucGreen[ucRGBIdx] & DATA_HIGH_BITMASK);
			PORT_DATA_LOW = (PORT_DATA_LOW & ~DATA_LOW_BITMASK) | (aucGreen[ucRGBIdx] & DATA_LOW_BITMASK);
			PORT_CONTROL |= (1<<SEND); // generate send impulse
			PORT_CONTROL &= ~(1<<SEND);
			ucByteIdx++;
			break;

			case 1:
			PORT_DATA_HIGH = (PORT_DATA_HIGH & ~DATA_HIGH_BITMASK) | (aucRed[ucRGBIdx] & DATA_HIGH_BITMASK);
			PORT_DATA_LOW = (PORT_DATA_LOW & ~DATA_LOW_BITMASK) | (aucRed[ucRGBIdx] & DATA_LOW_BITMASK);
			PORT_CONTROL |= (1<<SEND); // generate send impulse
			PORT_CONTROL &= ~(1<<SEND);
			ucByteIdx++;
			break;

			case 2:
			PORT_DATA_HIGH = (PORT_DATA_HIGH & ~DATA_HIGH_BITMASK) | (aucBlue[ucRGBIdx] & DATA_HIGH_BITMASK);
			PORT_DATA_LOW = (PORT_DATA_LOW & ~DATA_LOW_BITMASK) | (aucBlue[ucRGBIdx] & DATA_LOW_BITMASK);
			PORT_CONTROL |= (1<<SEND); // generate send impulse
			PORT_CONTROL &= ~(1<<SEND);
			ucByteIdx=0;
			ucRGBIdx++;
			break;
		}
	}
	
	#ifdef INT_OUT
	PORTD &= ~(1<<PORTD1);
	#endif
}


ISR(SPI_STC_vect)
{
	uint8_t u8spiData = SPDR0;
	
	#ifdef INT_OUT
	PORTD |= (1<<PORTD1);
	#endif
	
	SPDR0 = 0;

	switch(SPIBUFFER.spiState)
	{
		case READY:
			if(u8spiData>=3)
			{
				SPIBUFFER.au8Buffer[SPIBUFFER.u8Count] = u8spiData;
				SPIBUFFER.u8Count++;
				SPIBUFFER.spiState = WRITE;
			}
			else
			{
				SPIBUFFER.spiState = READ;
			}
		break;

		case WRITE:
			SPIBUFFER.au8Buffer[SPIBUFFER.u8Count] = u8spiData;
			SPIBUFFER.u8Count++;
			if((SPIBUFFER.u8Count)>=(SPIBUFFER.au8Buffer[0]))
			{
				SPIBUFFER.spiState = DONE_WRITE;
			}
		break;

		case READ:
			SPIBUFFER.u8Count = 0;
			
			switch(u8spiData)
			{
				case 0xF1:
					SPDR0 = 0x01;
					SPIBUFFER.au8Buffer[0] = 4;
					SPIBUFFER.au8Buffer[1] = u8spiData;
					SPIBUFFER.au8Buffer[2] = u8Status;
					SPIBUFFER.au8Buffer[3] = CRC8(&SPIBUFFER.au8Buffer[0],3);
					SPIBUFFER.u8Count = 4;
					SPIBUFFER.u8ReadReturnCount = 0;
					SPIBUFFER.spiState = READ_RETURN;
				break;
				
				case 0xF2:
					SPDR0 = 0x01;
					SPIBUFFER.au8Buffer[0] = 4;
					SPIBUFFER.au8Buffer[1] = u8spiData;
					SPIBUFFER.au8Buffer[2] = u8Duty;
					SPIBUFFER.au8Buffer[3] = CRC8(&SPIBUFFER.au8Buffer[0],3);
					SPIBUFFER.u8Count = 4;
					SPIBUFFER.u8ReadReturnCount = 0;
					SPIBUFFER.spiState = READ_RETURN;
				break;
				
				case 0xF3:
					SPDR0 = 0x01;
					SPIBUFFER.au8Buffer[0] = 4;
					SPIBUFFER.au8Buffer[1] = u8spiData;
					SPIBUFFER.au8Buffer[2] = adcGetTemperature();
					SPIBUFFER.au8Buffer[3] = CRC8(&SPIBUFFER.au8Buffer[0],3);
					SPIBUFFER.u8Count = 4;
					SPIBUFFER.u8ReadReturnCount = 0;
					SPIBUFFER.spiState = READ_RETURN;
				break;

				case 0xF4:
					SPDR0 = 0x01;
					if(u8PLEDFadeActive) // ongoing fade
					{
						SPIBUFFER.au8Buffer[0] = 7;
						SPIBUFFER.au8Buffer[1] = u8spiData;
						SPIBUFFER.au8Buffer[2] = u8PLEDFadeMinPercent;
						SPIBUFFER.au8Buffer[3] = u8PLEDFadeMaxPercent;
						SPIBUFFER.au8Buffer[4] = u8PLEDFadeTime;
						SPIBUFFER.au8Buffer[5] = u8Duty;
						SPIBUFFER.au8Buffer[6] = CRC8(&SPIBUFFER.au8Buffer[0],6);
						SPIBUFFER.u8Count = 7;
					}
					else // no ongoing fade
					{
						SPIBUFFER.au8Buffer[0] = 3;
						SPIBUFFER.au8Buffer[1] = u8spiData;
						SPIBUFFER.au8Buffer[2] = CRC8(&SPIBUFFER.au8Buffer[0],2);
						SPIBUFFER.u8Count = 3;
					}
					SPIBUFFER.u8ReadReturnCount = 0;
					SPIBUFFER.spiState = READ_RETURN;
				break;
				
				default:
					SPIBUFFER.spiState = IDLE;
				break;
			}
		break;
		
		case READ_RETURN:
			SPDR0 = SPIBUFFER.au8Buffer[SPIBUFFER.u8ReadReturnCount];
			SPIBUFFER.u8ReadReturnCount++;
			if(SPIBUFFER.u8Count == SPIBUFFER.u8ReadReturnCount)
			{
				SPIBUFFER.spiState = DONE_READ;
			}
		break;
		
		case DONE_WRITE:
		
		break;
		
		case DONE_READ:
		
		break;

		case IDLE:

		break;
	}
	#ifdef INT_OUT
	PORTD &= ~(1<<PORTD1);
	#endif
}

ISR(PCINT1_vect)
{
	#ifdef INT_OUT
	PORTD |= (1<<PORTD1);
	#endif
	
	SPDR0 = 0;
	if(PIN_SPI & (1<<SPI_SS)) // SS HIGH
	{
		
		if(SPIBUFFER.spiState == DONE_WRITE)
		{
			if(SPIBUFFER.u8Count == SPIBUFFER.au8Buffer[0]) // correct amount of bytes in buffer
			{
				if(CRC8(&SPIBUFFER.au8Buffer[0],SPIBUFFER.u8Count) == 0) // CRC8 correct
				{
					switch(SPIBUFFER.au8Buffer[1]) // command
					{
						case 0x11:
						if(SPIBUFFER.u8Count == 3)
						{
							enablePLED();
							u8Status |= (1<<STATUS_PLED);
						}
						break;

						case 0x12:
						if(SPIBUFFER.u8Count == 3)
						{
							disablePLED();
							u8Status &= ~(1<<STATUS_PLED);
						}
						break;

						case 0x13:
						if(SPIBUFFER.u8Count == 4)
						{
							if(u8PLEDFadeActive == 0)
							{
								if(SPIBUFFER.au8Buffer[2]>100)
								{
									u8Duty = 100;
								}
								else
								{
									u8Duty = SPIBUFFER.au8Buffer[2];
								}
								setPWMDutyPercent(u8Duty);
							}
						}
						break;

						case 0x14:
						if(SPIBUFFER.u8Count == 6)
						{
							if(SPIBUFFER.au8Buffer[2]>100)
							{
								u16PLEDFadeMinValue=511;
								u8PLEDFadeMinPercent=100;
							}
							else
							{
								u16PLEDFadeMinValue = Map(SPIBUFFER.au8Buffer[2],0,100,0,511);
								u8PLEDFadeMinPercent = SPIBUFFER.au8Buffer[2];
							}

							if(SPIBUFFER.au8Buffer[3]>100)
							{
								u16PLEDFadeMaxValue=511;
								u8PLEDFadeMaxPercent=100;
							}
							else
							{
								u16PLEDFadeMaxValue = Map(SPIBUFFER.au8Buffer[3],0,100,0,511);
								u8PLEDFadeMaxPercent = SPIBUFFER.au8Buffer[3];
							}
							
							u8PLEDFadeTime = SPIBUFFER.au8Buffer[4];

							setPWMDuty(u16PLEDFadeMinValue);
							u8Duty = SPIBUFFER.au8Buffer[2];
							u16PLEDFadeCurrValue = u16PLEDFadeMinValue;
							u32PLEDFadeIntCount = 0;
							u32PLEDFadeIntStep = 6000*SPIBUFFER.au8Buffer[4]/(u16PLEDFadeMaxValue-u16PLEDFadeMinValue);
							u8PLEDFadeActive = 1;
						}
						break;
						u8PLEDFadeActive = 0;
						case 0x15:

						break;
					
						case 0x21:
						if(SPIBUFFER.u8Count == 3)
						{
							enableAudio();
							u8Status |= (1<<STATUS_AUDIO);
						}
						break;

						case 0x22:
						if(SPIBUFFER.u8Count == 3)
						{
							standbyAudio();
							u8Status &= ~(1<<STATUS_AUDIO);
						}
						break;
					
						case 0x23:
						if(SPIBUFFER.u8Count == 4)
						{
							setVolume(SPIBUFFER.au8Buffer[2]);
						}
						break;
					}
				}
				else // CRC8 incorrect
				{

				}
			}
			else // incorrect amount of bytes in buffer
			{
			
			}
		} //DONE_WRITE
		else if(SPIBUFFER.spiState == DONE_READ)
		{
			
		}
	}
	else // SS LOW
	{
		SPIBUFFER.u8Count = 0;
		SPIBUFFER.spiState = READY;
	}
	#ifdef INT_OUT
	PORTD &= ~(1<<PORTD1);
	#endif
}



int main(void)
{
	uint16_t i;
	
	portInit();
	adcInit();
	initPWM(0);
	startPWM();
	spiInitBuffer(&SPIBUFFER);
	spiSlaveInit();
 	spiPcInt();
	initRGBooster();
	INT0_Init();
	INT_5ms_Init();

	wait_1ms(100);
	initAudio();


	DDRD |= (1<<DDRD1);
	#ifdef INT_OUT
	DDRD |= (1<<DDRD1);
	PORTD &= ~(1<<PORTD1);
	#endif
		
	sei();
	
// 	enablePLED();
// 	u16PLEDFadeMinValue = 0;
// 	u16PLEDFadeMaxValue = 51;
// 	u16PLEDFadeCurrValue = u16PLEDFadeMinValue;
// 	setPWMDuty(u16PLEDFadeMinValue);
// 	u32PLEDFadeIntCount = 0;
// 	u32PLEDFadeIntStep = 39;
// 	u8PLEDFadeActive = 1;
	
		
	
    while (1) 
    {
		wait_1ms(1000);
		ucByteIdx = 0;
		ucRGBIdx = 0;
		INT0_vect();
    }
}

