/*
 * clusterwink_mk_ii.c
 *
 * Created: 09/04/2018 16:52:30
 * Author : Mario
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>
#include "utils.h"
#include "spi.h"
#include "ringbuffer.h"
#include "rgbooster.h"

#define STATUS_PLED		0
#define STATUS_AUDIO	1

#define TX_DEBUG
#define RX_DEBUG

#define LED_COUNT		20
	
volatile uint8_t au8Red[LED_COUNT] =		{0xFF,0x00,0x00,0xFF,0x00,0x00,0xFF,0x00,0x00,0xFF,0x00,0x00,0xFF,0x00,0x00,0xFF,0x00,0x00,0xFF,0x00}; //data buffer for RGB leds
volatile uint8_t au8Green[LED_COUNT] =		{0x00,0xFF,0x00,0x00,0xFF,0x00,0x00,0xFF,0x00,0x00,0xFF,0x00,0x00,0xFF,0x00,0x00,0xFF,0x00,0x00,0xFF};
volatile uint8_t au8Blue[LED_COUNT] =		{0x00,0x00,0xFF,0x00,0x00,0xFF,0x00,0x00,0xFF,0x00,0x00,0xFF,0x00,0x00,0xFF,0x00,0x00,0xFF,0x00,0x00};
volatile uint8_t u8RGBIdx = LED_COUNT;		// RGB ISR variables
volatile uint8_t u8RGBByteIdx = 0;
volatile uint8_t u8RGBSingleColor = 0;
volatile uint8_t u8RGBDataState = 0; //0:idle 1:data ready to be written 2:data is being written
volatile uint8_t u8RGBRed = 0;
volatile uint8_t u8RGBGreen = 0;
volatile uint8_t u8RGBBlue = 10;
volatile uint8_t u8RGBStartRed = 0;
volatile uint8_t u8RGBStartGreen = 0;
volatile uint8_t u8RGBStartBlue = 0;
volatile uint8_t u8RGBStartMiddleRed = 0;
volatile uint8_t u8RGBStartMiddleGreen = 0;
volatile uint8_t u8RGBStartMiddleBlue = 0;
volatile uint8_t u8RGBStopRed = 0;
volatile uint8_t u8RGBStopGreen = 0;
volatile uint8_t u8RGBStopBlue = 0;
volatile uint8_t u8RGBStopMiddleRed = 0;
volatile uint8_t u8RGBStopMiddleGreen = 0;
volatile uint8_t u8RGBStopMiddleBlue = 0;
volatile uint8_t u8RGBBounce = 0;
volatile uint8_t u8RGBBounceDirection = 0;
volatile uint16_t u16RGBTime = 0;
volatile uint16_t u16RGBTimeCounter = 0;
volatile uint8_t u8RGBAnimation = 0; //0:off 1:fade 2:gradient fade

volatile uint8_t u8PLEDFadeStartPercent = 0;
volatile uint8_t u8PLEDFadeStopPercent = 0;
volatile uint8_t u8PLEDFadeTime = 0;
volatile uint16_t u16PLEDFadeStopValue = 0;
volatile uint16_t u16PLEDFadeStartValue = 0;
volatile uint16_t u16PLEDFadeCurrValue = 0;
volatile uint32_t u32PLEDFadeIntStep = 0;
volatile uint32_t u32PLEDFadeIntCount = 0;
volatile uint8_t u8PLEDFadeDirection = 0; // 0:fall 1:rise

volatile uint16_t u16PLEDStrobeCount = 0;
volatile uint16_t u16PLEDStrobeOn = 0;
volatile uint16_t u16PLEDStrobeTotal = 0;

volatile uint8_t u8PLEDAnimation = 0; //0:off 1:fade 2:strobe






static RingBuff_t RINGBUFFER;
static SpiBuf_t SPIBUFFER;


volatile uint8_t u8Status = 0x00;
volatile uint8_t u8Duty = 0;


ISR(INT0_vect)	// external interrupt (handshake from RGBooster board)
{				// start RGBooster send sequence: reset "ucRGBIdx" and "ucByteIdx" to zero. then start with calling the ISR directly "INT1_vect();"
	#ifdef TX_DEBUG
	PORTD |= (1<<PORTD1);
	#endif
	
	if(u8RGBSingleColor)
	{
		if(u8RGBIdx<(LED_COUNT))
		{
			switch(u8RGBByteIdx) // red green and blue are sent in 3 separate bytes. this variable remembers the next color to be sent
			{
				case 0:
				PORT_DATA_HIGH = (PORT_DATA_HIGH & ~DATA_HIGH_BITMASK) | (u8RGBGreen & DATA_HIGH_BITMASK);
				PORT_DATA_LOW = (PORT_DATA_LOW & ~DATA_LOW_BITMASK) | (u8RGBGreen & DATA_LOW_BITMASK);
				PORT_CONTROL |= (1<<SEND); // generate send impulse
				PORT_CONTROL &= ~(1<<SEND);
				u8RGBByteIdx++;
				break;

				case 1:
				PORT_DATA_HIGH = (PORT_DATA_HIGH & ~DATA_HIGH_BITMASK) | (u8RGBRed & DATA_HIGH_BITMASK);
				PORT_DATA_LOW = (PORT_DATA_LOW & ~DATA_LOW_BITMASK) | (u8RGBRed & DATA_LOW_BITMASK);
				PORT_CONTROL |= (1<<SEND); // generate send impulse
				PORT_CONTROL &= ~(1<<SEND);
				u8RGBByteIdx++;
				break;

				case 2:
				PORT_DATA_HIGH = (PORT_DATA_HIGH & ~DATA_HIGH_BITMASK) | (u8RGBBlue & DATA_HIGH_BITMASK);
				PORT_DATA_LOW = (PORT_DATA_LOW & ~DATA_LOW_BITMASK) | (u8RGBBlue & DATA_LOW_BITMASK);
				PORT_CONTROL |= (1<<SEND); // generate send impulse
				PORT_CONTROL &= ~(1<<SEND);
				u8RGBByteIdx=0;
				u8RGBIdx++;
				break;
			}
		}
		else
		{
			u8RGBDataState = 0;
		}
	}
	else
	{
		if(u8RGBIdx<(LED_COUNT))
		{
			switch(u8RGBByteIdx) // red green and blue are sent in 3 separate bytes. this variable remembers the next color to be sent
			{
				case 0:
				PORT_DATA_HIGH = (PORT_DATA_HIGH & ~DATA_HIGH_BITMASK) | (au8Green[u8RGBIdx] & DATA_HIGH_BITMASK);
				PORT_DATA_LOW = (PORT_DATA_LOW & ~DATA_LOW_BITMASK) | (au8Green[u8RGBIdx] & DATA_LOW_BITMASK);
				PORT_CONTROL |= (1<<SEND); // generate send impulse
				PORT_CONTROL &= ~(1<<SEND);
				u8RGBByteIdx++;
				break;

				case 1:
				PORT_DATA_HIGH = (PORT_DATA_HIGH & ~DATA_HIGH_BITMASK) | (au8Red[u8RGBIdx] & DATA_HIGH_BITMASK);
				PORT_DATA_LOW = (PORT_DATA_LOW & ~DATA_LOW_BITMASK) | (au8Red[u8RGBIdx] & DATA_LOW_BITMASK);
				PORT_CONTROL |= (1<<SEND); // generate send impulse
				PORT_CONTROL &= ~(1<<SEND);
				u8RGBByteIdx++;
				break;

				case 2:
				PORT_DATA_HIGH = (PORT_DATA_HIGH & ~DATA_HIGH_BITMASK) | (au8Blue[u8RGBIdx] & DATA_HIGH_BITMASK);
				PORT_DATA_LOW = (PORT_DATA_LOW & ~DATA_LOW_BITMASK) | (au8Blue[u8RGBIdx] & DATA_LOW_BITMASK);
				PORT_CONTROL |= (1<<SEND); // generate send impulse
				PORT_CONTROL &= ~(1<<SEND);
				u8RGBByteIdx=0;
				u8RGBIdx++;
				break;
			}
		}
		else
		{
			u8RGBDataState = 0;
		}
	}
	
	
	#ifdef TX_DEBUG
	PORTD &= ~(1<<PORTD1);
	#endif
}

ISR(TIMER2_COMPA_vect)
{
	#ifdef TX_DEBUG
	PORTD |= (1<<PORTD1);
	#endif
	
	switch(u8PLEDAnimation)
	{
		case 1:
		if(u8PLEDFadeDirection) // rise
		{
			u32PLEDFadeIntCount++;
			if(u32PLEDFadeIntCount>=u32PLEDFadeIntStep)
			{
				u32PLEDFadeIntCount = 0;
				u16PLEDFadeCurrValue++;
				setPWMDuty(u16PLEDFadeCurrValue);
				u8Duty = Map(u16PLEDFadeCurrValue,0,511,0,100);

				if(u16PLEDFadeCurrValue>=u16PLEDFadeStopValue)
				{
					u8PLEDAnimation = 0;
				}
			}
		}
		else // fall
		{
			u32PLEDFadeIntCount++;
			if(u32PLEDFadeIntCount>=u32PLEDFadeIntStep)
			{
				u32PLEDFadeIntCount = 0;
				u16PLEDFadeCurrValue--;
				setPWMDuty(u16PLEDFadeCurrValue);
				u8Duty = Map(u16PLEDFadeCurrValue,0,511,0,100);

				if(u16PLEDFadeCurrValue<=u16PLEDFadeStopValue)
				{
					u8PLEDAnimation = 0;
				}
			}
		}
		break;
		
		case 2:
		if(u16PLEDStrobeCount<u16PLEDStrobeOn)
		{
			enablePLED();
		}
		else
		{
			disablePLED();
		}
		
		u16PLEDStrobeCount++;
		if(u16PLEDStrobeCount>=u16PLEDStrobeTotal)
		{
			u16PLEDStrobeCount = 0;
		}
		break;
	}
	
	if(u8RGBAnimation)
	{
		if(u8RGBBounce)
		{
			if(u8RGBBounceDirection==0) //normal
			{
				u16RGBTimeCounter++;
				if(u16RGBTimeCounter>=u16RGBTime)
				{
					u8RGBBounceDirection = 1;
					//u16RGBTimeCounter--;
				}
			}
			else //reverse
			{
				u16RGBTimeCounter--;
				if(u16RGBTimeCounter==0)
				{
					u8RGBBounceDirection = 0;
					//u16RGBTimeCounter++;
				}
			}
		}
		else
		{
			u16RGBTimeCounter++;
			if(u16RGBTimeCounter>=u16RGBTime)
			{
				u8RGBAnimation = 0;
			}
		}
		
		
	}
	
	if(u8RGBDataState==1)
	{
		u8RGBDataState = 2;
		
		u8RGBByteIdx = 0;
		u8RGBIdx = 0;
		INT0_vect();
	}

	#ifdef TX_DEBUG
	PORTD &= ~(1<<PORTD1);
	#endif
}


ISR(SPI_STC_vect)
{
	uint8_t u8spiData = SPDR0;
	
	#ifdef TX_DEBUG
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
					if(u8PLEDAnimation) // ongoing fade
					{
						SPIBUFFER.au8Buffer[0] = 7;
						SPIBUFFER.au8Buffer[1] = u8spiData;
						SPIBUFFER.au8Buffer[2] = u8PLEDFadeStartPercent;
						SPIBUFFER.au8Buffer[3] = u8PLEDFadeStopPercent;
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
	#ifdef TX_DEBUG
	PORTD &= ~(1<<PORTD1);
	#endif
}

ISR(PCINT1_vect)
{
	#ifdef TX_DEBUG
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
							if(u8PLEDAnimation == 0)
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
						u8PLEDAnimation = 0;
						disablePLED();
						u8Status &= ~(1<<STATUS_PLED);
						break;

						case 0x15:
						if(SPIBUFFER.u8Count == 6)
						{	
							if(SPIBUFFER.au8Buffer[2]>100)
							{
								u8PLEDFadeStartPercent=100;
							}
							else
							{
								u8PLEDFadeStartPercent = SPIBUFFER.au8Buffer[2];
							}
							
							if(SPIBUFFER.au8Buffer[3]>100)
							{
								u8PLEDFadeStopPercent=100;
							}
							else
							{
								u8PLEDFadeStopPercent = SPIBUFFER.au8Buffer[3];
							}
							
							u16PLEDFadeStartValue = Map(u8PLEDFadeStartPercent,0,100,0,511);
							u16PLEDFadeStopValue = Map(u8PLEDFadeStopPercent,0,100,0,511);
							u8PLEDFadeTime = SPIBUFFER.au8Buffer[4];
							
							setPWMDuty(u16PLEDFadeStartValue);
							u8Duty = u8PLEDFadeStartPercent;
							u16PLEDFadeCurrValue = u16PLEDFadeStartValue;
							u32PLEDFadeIntCount = 0;
							
							if(u8PLEDFadeStartPercent<u8PLEDFadeStopPercent) // PLED rise
							{
								u32PLEDFadeIntStep = 6000*u8PLEDFadeTime/(u16PLEDFadeStopValue-u16PLEDFadeStartValue);
								u8PLEDFadeDirection = 1;
							}
							else if(u8PLEDFadeStartPercent>u8PLEDFadeStopPercent) // PLED fall
							{
								u32PLEDFadeIntStep = 6000*u8PLEDFadeTime/(u16PLEDFadeStartValue-u16PLEDFadeStopValue);
								u8PLEDFadeDirection = 0;
							}
							enablePLED();
							u8Status |= (1<<STATUS_PLED);
							u8PLEDAnimation = 1;
						}
						break;
						
						case 0x16:
						if(SPIBUFFER.u8Count == 6)
						{
							u16PLEDStrobeCount = 0;
							u16PLEDStrobeOn = 2*((uint16_t)SPIBUFFER.au8Buffer[3]);
							u16PLEDStrobeTotal = 2*(((uint16_t)SPIBUFFER.au8Buffer[3])+((uint16_t)SPIBUFFER.au8Buffer[4]));
							
							if(SPIBUFFER.au8Buffer[2]>100)
							{
								u8Duty = 100;
							}
							else
							{
								u8Duty = SPIBUFFER.au8Buffer[2];
							}
							setPWMDutyPercent(u8Duty);
							u8Status |= (1<<STATUS_PLED);
							u8PLEDAnimation = 2;
						}
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
						
						case 0x31:
						RingBuffer_Insert(&RINGBUFFER,0x31);
						RingBuffer_Insert(&RINGBUFFER,0xFF);						
						break;
						
						case 0x32:
						if(SPIBUFFER.u8Count == 6)
						{
							RingBuffer_Insert(&RINGBUFFER,0x32);
							RingBuffer_Insert(&RINGBUFFER,SPIBUFFER.au8Buffer[2]);
							RingBuffer_Insert(&RINGBUFFER,SPIBUFFER.au8Buffer[3]);
							RingBuffer_Insert(&RINGBUFFER,SPIBUFFER.au8Buffer[4]);
							RingBuffer_Insert(&RINGBUFFER,0xFF);
						}
						
						case 0x33:
						if(SPIBUFFER.u8Count == 9)
						{
							RingBuffer_Insert(&RINGBUFFER,0x33);
							RingBuffer_Insert(&RINGBUFFER,SPIBUFFER.au8Buffer[2]);
							RingBuffer_Insert(&RINGBUFFER,SPIBUFFER.au8Buffer[3]);
							RingBuffer_Insert(&RINGBUFFER,SPIBUFFER.au8Buffer[4]);
							RingBuffer_Insert(&RINGBUFFER,SPIBUFFER.au8Buffer[5]);
							RingBuffer_Insert(&RINGBUFFER,SPIBUFFER.au8Buffer[6]);
							RingBuffer_Insert(&RINGBUFFER,SPIBUFFER.au8Buffer[7]);
							RingBuffer_Insert(&RINGBUFFER,0xFF);
						}
						break;
						
						case 0x41:
						RingBuffer_Insert(&RINGBUFFER,0x41);
						RingBuffer_Insert(&RINGBUFFER,0xFF);
						break;
						
						case 0x42:
						if(SPIBUFFER.u8Count == 11)
						{
							RingBuffer_Insert(&RINGBUFFER,0x42);
							RingBuffer_Insert(&RINGBUFFER,SPIBUFFER.au8Buffer[2]);
							RingBuffer_Insert(&RINGBUFFER,SPIBUFFER.au8Buffer[3]);
							RingBuffer_Insert(&RINGBUFFER,SPIBUFFER.au8Buffer[4]);
							RingBuffer_Insert(&RINGBUFFER,SPIBUFFER.au8Buffer[5]);
							RingBuffer_Insert(&RINGBUFFER,SPIBUFFER.au8Buffer[6]);
							RingBuffer_Insert(&RINGBUFFER,SPIBUFFER.au8Buffer[7]);
							RingBuffer_Insert(&RINGBUFFER,SPIBUFFER.au8Buffer[8]);
							RingBuffer_Insert(&RINGBUFFER,SPIBUFFER.au8Buffer[9]);
							RingBuffer_Insert(&RINGBUFFER,0xFF);
						}
						break;

						case 0x43:
						if(SPIBUFFER.u8Count == 17)
						{
							RingBuffer_Insert(&RINGBUFFER,0x43);
							RingBuffer_Insert(&RINGBUFFER,SPIBUFFER.au8Buffer[2]);
							RingBuffer_Insert(&RINGBUFFER,SPIBUFFER.au8Buffer[3]);
							RingBuffer_Insert(&RINGBUFFER,SPIBUFFER.au8Buffer[4]);
							RingBuffer_Insert(&RINGBUFFER,SPIBUFFER.au8Buffer[5]);
							RingBuffer_Insert(&RINGBUFFER,SPIBUFFER.au8Buffer[6]);
							RingBuffer_Insert(&RINGBUFFER,SPIBUFFER.au8Buffer[7]);
							RingBuffer_Insert(&RINGBUFFER,SPIBUFFER.au8Buffer[8]);
							RingBuffer_Insert(&RINGBUFFER,SPIBUFFER.au8Buffer[9]);
							RingBuffer_Insert(&RINGBUFFER,SPIBUFFER.au8Buffer[10]);
							RingBuffer_Insert(&RINGBUFFER,SPIBUFFER.au8Buffer[11]);
							RingBuffer_Insert(&RINGBUFFER,SPIBUFFER.au8Buffer[12]);
							RingBuffer_Insert(&RINGBUFFER,SPIBUFFER.au8Buffer[13]);
							RingBuffer_Insert(&RINGBUFFER,SPIBUFFER.au8Buffer[14]);
							RingBuffer_Insert(&RINGBUFFER,SPIBUFFER.au8Buffer[15]);
							RingBuffer_Insert(&RINGBUFFER,0xFF);
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
	#ifdef TX_DEBUG
	PORTD &= ~(1<<PORTD1);
	#endif
}

// int main(void)
// {
// 	uint8_t u8Temp;
// 	uint8_t au8Temp[64];
// 	
// 	RingBuffer_InitBuffer(&RINGBUFFER);
// 
// 	RingBuffer_Insert(&RINGBUFFER,0x31);
// 	RingBuffer_Insert(&RINGBUFFER,0x1);
// 	RingBuffer_Insert(&RINGBUFFER,0x2);
// 	RingBuffer_Insert(&RINGBUFFER,0x3);
// 	RingBuffer_Insert(&RINGBUFFER,0xFF);
// 	RingBuffer_Insert(&RINGBUFFER,0x31);
// 	RingBuffer_Insert(&RINGBUFFER,0xFF);
// 	RingBuffer_Insert(&RINGBUFFER,0xFF);
// 
// 	u8Temp = RingBuffer_IsEmpty(&RINGBUFFER);
// 	u8Temp = RingBuffer_IsFull(&RINGBUFFER);
// 	u8Temp = RingBuffer_GetCount(&RINGBUFFER);
// 	
// 	u8Temp = RingBuffer_CountChar(&RINGBUFFER,0xFF);
// 	
// 	RingBuffer_RemoveUntilChar(&RINGBUFFER,au8Temp,0xFF,1);
// 	RingBuffer_RemoveUntilChar(&RINGBUFFER,au8Temp,0xFF,1);
// 	RingBuffer_RemoveUntilChar(&RINGBUFFER,au8Temp,0xFF,1);
// 	
// 	while(1)
// 	{
// 		
// 	}
// }

int main(void)
{
	uint16_t i;
	uint8_t au8Command[8];
	uint8_t u8Temp,u8Count,u8StartTemp,u8StopTemp;
	portInit();
	adcInit();
	initPWM(0);
	startPWM();
	spiInitBuffer(&SPIBUFFER);
	spiSlaveInit();
 	spiPcInt();
	RingBuffer_InitBuffer(&RINGBUFFER);
	initRGBooster();
	INT0_Init();
	INT_5ms_Init();

	wait_1ms(100);
	initAudio();
	
	#ifdef TX_DEBUG
	DDRD |= (1<<DDRD1);
	PORTD &= ~(1<<PORTD1);
	#endif

	#ifdef RX_DEBUG
	DDRD |= (1<<DDRD0);
	PORTD &= ~(1<<PORTD0);
	#endif
		
	sei();
	
	u8RGBSingleColor = 0;
	u8RGBByteIdx = 0;
	u8RGBIdx = 0;
	INT0_vect();
	wait_1ms(100);
	
	
	RingBuffer_Insert(&RINGBUFFER,0x31);
	RingBuffer_Insert(&RINGBUFFER,0xFF);
	
	RingBuffer_Insert(&RINGBUFFER,0x32);
	RingBuffer_Insert(&RINGBUFFER,0x01);
	RingBuffer_Insert(&RINGBUFFER,0x01);
	RingBuffer_Insert(&RINGBUFFER,0x10);
	RingBuffer_Insert(&RINGBUFFER,0xFF);
	
	RingBuffer_Insert(&RINGBUFFER,0x31);
	RingBuffer_Insert(&RINGBUFFER,0xFF);
	
	RingBuffer_Insert(&RINGBUFFER,0x32);
	RingBuffer_Insert(&RINGBUFFER,0x01);
	RingBuffer_Insert(&RINGBUFFER,0x01);
	RingBuffer_Insert(&RINGBUFFER,0x10);
	RingBuffer_Insert(&RINGBUFFER,0xFF);
	
	RingBuffer_Insert(&RINGBUFFER,0x31);
	RingBuffer_Insert(&RINGBUFFER,0xFF);
	
	RingBuffer_Insert(&RINGBUFFER,0x32);
	RingBuffer_Insert(&RINGBUFFER,0x01);
	RingBuffer_Insert(&RINGBUFFER,0x01);
	RingBuffer_Insert(&RINGBUFFER,0x10);
	RingBuffer_Insert(&RINGBUFFER,0xFF);
	
	RingBuffer_Insert(&RINGBUFFER,0x31);
	RingBuffer_Insert(&RINGBUFFER,0xFF);
	
	RingBuffer_Insert(&RINGBUFFER,0x32);
	RingBuffer_Insert(&RINGBUFFER,0x01);
	RingBuffer_Insert(&RINGBUFFER,0x01);
	RingBuffer_Insert(&RINGBUFFER,0x10);
	RingBuffer_Insert(&RINGBUFFER,0xFF);
	
	RingBuffer_Insert(&RINGBUFFER,0x31);
	RingBuffer_Insert(&RINGBUFFER,0xFF);
	
	RingBuffer_Insert(&RINGBUFFER,0x32);
	RingBuffer_Insert(&RINGBUFFER,0x01);
	RingBuffer_Insert(&RINGBUFFER,0x01);
	RingBuffer_Insert(&RINGBUFFER,0x10);
	RingBuffer_Insert(&RINGBUFFER,0xFF);
		
	
    while (1) 
    {
		if(u8RGBAnimation)
		{
			if(RingBuffer_CountChar(&RINGBUFFER,0xFF))
			{
				RingBuffer_RemoveUntilChar(&RINGBUFFER,au8Command,0xFF,0);
				
				switch(au8Command[0])
				{
					case 0x41:
					u8RGBAnimation = 0;
					break;
					
					case 0x42:
					if(strlen(au8Command) == 9)
					{
						u8RGBStartRed = au8Command[1]-1;
						u8RGBStartGreen = au8Command[2]-1;
						u8RGBStartBlue = au8Command[3]-1;
						u8RGBStopRed = au8Command[4]-1;
						u8RGBStopGreen = au8Command[5]-1;
						u8RGBStopBlue = au8Command[6]-1;
						u16RGBTime = ((uint16_t)au8Command[7])*200;
						u8RGBBounce = au8Command[8]-1;
						u8RGBBounceDirection = 0;
						u16RGBTimeCounter = 0;
						u8RGBAnimation = 1;
					}
					break;

					case 0x43:
					if(strlen(au8Command) == 15)
					{
						u8RGBStartRed = au8Command[1]-1;
						u8RGBStartGreen = au8Command[2]-1;
						u8RGBStartBlue = au8Command[3]-1;
						u8RGBStartMiddleRed = au8Command[4]-1;
						u8RGBStartMiddleGreen = au8Command[5]-1;
						u8RGBStartMiddleBlue = au8Command[6]-1;
						u8RGBStopRed = au8Command[7]-1;
						u8RGBStopGreen = au8Command[8]-1;
						u8RGBStopBlue = au8Command[9]-1;
						u8RGBStopMiddleRed = au8Command[10]-1;
						u8RGBStopMiddleGreen = au8Command[11]-1;
						u8RGBStopMiddleBlue = au8Command[12]-1;
						u16RGBTime = ((uint16_t)au8Command[13])*200;
						u8RGBBounce = au8Command[14]-1;
						u8RGBBounceDirection = 0;
						u16RGBTimeCounter = 0;
						u8RGBAnimation = 2;
					}
					break;
				}
			}
			
			if(u8RGBDataState==0)
			{
				#ifdef RX_DEBUG
				PORTD |= (1<<PORTD0);
				#endif
				
				switch(u8RGBAnimation)
				{
					case 1:
					if(u8RGBStartRed<u8RGBStopRed)
					{
						u8RGBRed = (uint8_t)(((uint32_t)(u8RGBStopRed-u8RGBStartRed))*u16RGBTimeCounter/u16RGBTime + u8RGBStartRed);
					}
					else
					{
						u8RGBRed = (uint8_t)(u8RGBStartRed - ((uint32_t)(u8RGBStartRed-u8RGBStopRed))*u16RGBTimeCounter/u16RGBTime);
					}
					
					if(u8RGBStartGreen<u8RGBStopGreen)
					{
						u8RGBGreen = (uint8_t)(((uint32_t)(u8RGBStopGreen-u8RGBStartGreen))*u16RGBTimeCounter/u16RGBTime + u8RGBStartGreen);
					}
					else
					{
						u8RGBGreen = (uint8_t)(u8RGBStartGreen - ((uint32_t)(u8RGBStartGreen-u8RGBStopGreen))*u16RGBTimeCounter/u16RGBTime);
					}
					
					if(u8RGBStartBlue<u8RGBStopBlue)
					{
						u8RGBBlue = (uint8_t)(((uint32_t)(u8RGBStopBlue-u8RGBStartBlue))*u16RGBTimeCounter/u16RGBTime + u8RGBStartBlue);
					}
					else
					{
						u8RGBBlue = (uint8_t)(u8RGBStartBlue - ((uint32_t)(u8RGBStartBlue-u8RGBStopBlue))*u16RGBTimeCounter/u16RGBTime);
					}
					u8RGBSingleColor = 1;
					u8RGBDataState = 1;
					break;

					case 2:
					u8Count = LED_COUNT/2;
					for(i=0;i<u8Count;i++)
					{
						if(u8RGBStartRed<u8RGBStartMiddleRed)
						{
							u8StartTemp = (uint8_t)(((uint32_t)(u8RGBStartMiddleRed-u8RGBStartRed))*i/(u8Count-1)+u8RGBStartRed);
						}
						else
						{
							u8StartTemp = (uint8_t)(u8RGBStartRed-((uint32_t)(u8RGBStartRed-u8RGBStartMiddleRed))*i/(u8Count-1));
						}

						if(u8RGBStopRed<u8RGBStopMiddleRed)
						{
							u8StopTemp = (uint8_t)(((uint32_t)(u8RGBStopMiddleRed-u8RGBStopRed))*i/(u8Count-1)+u8RGBStopRed);
						}
						else
						{
							u8StopTemp = (uint8_t)(u8RGBStopRed-((uint32_t)(u8RGBStopRed-u8RGBStopMiddleRed))*i/(u8Count-1));
						}

						if(u8StartTemp<u8StopTemp)
						{
							u8Temp = (uint8_t)(((uint32_t)(u8StopTemp-u8StartTemp))*u16RGBTimeCounter/u16RGBTime + u8StartTemp);
						}
						else
						{
							u8Temp = (uint8_t)(u8StartTemp-((uint32_t)(u8StartTemp-u8StopTemp))*u16RGBTimeCounter/u16RGBTime);
						}
						au8Red[i] = u8Temp;
						au8Red[LED_COUNT-i-1] = u8Temp;

						if(u8RGBStartGreen<u8RGBStartMiddleGreen)
						{
							u8StartTemp = (uint8_t)(((uint32_t)(u8RGBStartMiddleGreen-u8RGBStartGreen))*i/(u8Count-1)+u8RGBStartGreen);
						}
						else
						{
							u8StartTemp = (uint8_t)(u8RGBStartGreen-((uint32_t)(u8RGBStartGreen-u8RGBStartMiddleGreen))*i/(u8Count-1));
						}

						if(u8RGBStopGreen<u8RGBStopMiddleGreen)
						{
							u8StopTemp = (uint8_t)(((uint32_t)(u8RGBStopMiddleGreen-u8RGBStopGreen))*i/(u8Count-1)+u8RGBStopGreen);
						}
						else
						{
							u8StopTemp = (uint8_t)(u8RGBStopGreen-((uint32_t)(u8RGBStopGreen-u8RGBStopMiddleGreen))*i/(u8Count-1));
						}

						if(u8StartTemp<u8StopTemp)
						{
							u8Temp = (uint8_t)(((uint32_t)(u8StopTemp-u8StartTemp))*u16RGBTimeCounter/u16RGBTime + u8StartTemp);
						}
						else
						{
							u8Temp = (uint8_t)(u8StartTemp-((uint32_t)(u8StartTemp-u8StopTemp))*u16RGBTimeCounter/u16RGBTime);
						}
						au8Green[i] = u8Temp;
						au8Green[LED_COUNT-i-1] = u8Temp;
						
						if(u8RGBStartBlue<u8RGBStartMiddleBlue)
						{
							u8StartTemp = (uint8_t)(((uint32_t)(u8RGBStartMiddleBlue-u8RGBStartBlue))*i/(u8Count-1)+u8RGBStartBlue);
						}
						else
						{
							u8StartTemp = (uint8_t)(u8RGBStartBlue-((uint32_t)(u8RGBStartBlue-u8RGBStartMiddleBlue))*i/(u8Count-1));
						}

						if(u8RGBStopBlue<u8RGBStopMiddleBlue)
						{
							u8StopTemp = (uint8_t)(((uint32_t)(u8RGBStopMiddleBlue-u8RGBStopBlue))*i/(u8Count-1)+u8RGBStopBlue);
						}
						else
						{
							u8StopTemp = (uint8_t)(u8RGBStopBlue-((uint32_t)(u8RGBStopBlue-u8RGBStopMiddleBlue))*i/(u8Count-1));
						}

						if(u8StartTemp<u8StopTemp)
						{
							u8Temp = (uint8_t)(((uint32_t)(u8StopTemp-u8StartTemp))*u16RGBTimeCounter/u16RGBTime + u8StartTemp);
						}
						else
						{
							u8Temp = (uint8_t)(u8StartTemp-((uint32_t)(u8StartTemp-u8StopTemp))*u16RGBTimeCounter/u16RGBTime);
						}
						au8Blue[i] = u8Temp;
						au8Blue[LED_COUNT-i-1] = u8Temp;
					}
					u8RGBSingleColor = 0;
					u8RGBDataState = 1;
					break;
				}

				#ifdef RX_DEBUG
				PORTD &= ~(1<<PORTD0);
				#endif
			}
		}
		else // no animation active
		{
			if(RingBuffer_CountChar(&RINGBUFFER,0xFF) && (u8RGBDataState==0))
			{
				RingBuffer_RemoveUntilChar(&RINGBUFFER,au8Command,0xFF,0);
				
				switch(au8Command[0])
				{
					case 0x31:
					u8RGBRed = 0;
					u8RGBGreen = 0;
					u8RGBBlue = 0;
					u8RGBSingleColor = 1;
					u8RGBDataState = 1;
					break;
					
					case 0x32:
					if(strlen(au8Command) == 4)
					{
						u8RGBRed = au8Command[1]-1;
						u8RGBGreen = au8Command[2]-1;
						u8RGBBlue = au8Command[3]-1;
						u8RGBSingleColor = 1;
						u8RGBDataState = 1;
					}
					break;
					
					case 0x33:
					if(strlen(au8Command) == 7)
					{
						u8RGBStartRed = au8Command[1]-1;
						u8RGBStartGreen = au8Command[2]-1;
						u8RGBStartBlue = au8Command[3]-1;
						u8RGBStopRed = au8Command[4]-1;
						u8RGBStopGreen = au8Command[5]-1;
						u8RGBStopBlue = au8Command[6]-1;
						
						
						u8Count = LED_COUNT/2;
						for(i=0;i<u8Count;i++)
						{
							if(u8RGBStartRed<u8RGBStopRed)
							{
								u8Temp = (uint8_t)(((uint32_t)(u8RGBStopRed-u8RGBStartRed))*i/(u8Count-1) + u8RGBStartRed);
							}
							else
							{
								u8Temp = (uint8_t)(u8RGBStartRed - ((uint32_t)(u8RGBStartRed-u8RGBStopRed))*i/(u8Count-1));
							}
							au8Red[i] = u8Temp;
							au8Red[LED_COUNT-i-1] = u8Temp;
							
							if(u8RGBStartGreen<u8RGBStopGreen)
							{
								u8Temp = (uint8_t)(((uint32_t)(u8RGBStopGreen-u8RGBStartGreen))*i/(u8Count-1) + u8RGBStartGreen);
							}
							else
							{
								u8Temp = (uint8_t)(u8RGBStartGreen - ((uint32_t)(u8RGBStartGreen-u8RGBStopGreen))*i/(u8Count-1));
							}
							au8Green[i] = u8Temp;
							au8Green[LED_COUNT-i-1] = u8Temp;
							
							if(u8RGBStartBlue<u8RGBStopBlue)
							{
								u8Temp = (uint8_t)(((uint32_t)(u8RGBStopBlue-u8RGBStartBlue))*i/(u8Count-1) + u8RGBStartBlue);
							}
							else
							{
								u8Temp = (uint8_t)(u8RGBStartBlue - ((uint32_t)(u8RGBStartBlue-u8RGBStopBlue))*i/(u8Count-1));
							}
							au8Blue[i] = u8Temp;
							au8Blue[LED_COUNT-i-1] = u8Temp;
						}
						u8RGBSingleColor = 0;
						u8RGBDataState = 1;
					}
					break;
					
					case 0x42:
					if(strlen(au8Command) == 9)
					{
						u8RGBStartRed = au8Command[1]-1;
						u8RGBStartGreen = au8Command[2]-1;
						u8RGBStartBlue = au8Command[3]-1;
						u8RGBStopRed = au8Command[4]-1;
						u8RGBStopGreen = au8Command[5]-1;
						u8RGBStopBlue = au8Command[6]-1;
						u16RGBTime = ((uint16_t)au8Command[7])*200;
						u8RGBBounce = au8Command[8]-1;
						u8RGBBounceDirection = 0;
						u16RGBTimeCounter = 0;
						u8RGBAnimation = 1;
					}
					break;

					case 0x43:
					if(strlen(au8Command) == 15)
					{
						u8RGBStartRed = au8Command[1]-1;
						u8RGBStartGreen = au8Command[2]-1;
						u8RGBStartBlue = au8Command[3]-1;
						u8RGBStartMiddleRed = au8Command[4]-1;
						u8RGBStartMiddleGreen = au8Command[5]-1;
						u8RGBStartMiddleBlue = au8Command[6]-1;
						u8RGBStopRed = au8Command[7]-1;
						u8RGBStopGreen = au8Command[8]-1;
						u8RGBStopBlue = au8Command[9]-1;
						u8RGBStopMiddleRed = au8Command[10]-1;
						u8RGBStopMiddleGreen = au8Command[11]-1;
						u8RGBStopMiddleBlue = au8Command[12]-1;
						u16RGBTime = ((uint16_t)au8Command[13])*200;
						u8RGBBounce = au8Command[14]-1;
						u8RGBBounceDirection = 0;
						u16RGBTimeCounter = 0;
						u8RGBAnimation = 2;
					}
					break;
				}
			}
		}
    }
}

