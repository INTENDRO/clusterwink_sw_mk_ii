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


static RingBuff_t RINGBUFFER;
static SpiBuf_t SPIBUFFER;


volatile uint8_t u8Status = 0x11;
volatile uint8_t u8Duty = 0;
volatile uint8_t u8Temperature = 0x33;

ISR(SPI_STC_vect)
{
	uint8_t u8spiData = SPDR0;
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
				case 0xC1:
					SPDR0 = 0x01;
					SPIBUFFER.au8Buffer[3] = 4;
					SPIBUFFER.au8Buffer[2] = u8spiData;
					SPIBUFFER.au8Buffer[1] = u8Status;
					SPIBUFFER.au8Buffer[0] = CRC8(&SPIBUFFER.au8Buffer[0],3);
					SPIBUFFER.u8Count = 4;
					SPIBUFFER.spiState == READ_RETURN;
				break;
				
				case 0xC2:
					SPDR0 = 0x01;
					SPIBUFFER.au8Buffer[3] = 4;
					SPIBUFFER.au8Buffer[2] = u8spiData;
					SPIBUFFER.au8Buffer[1] = u8Duty;
					SPIBUFFER.au8Buffer[0] = CRC8(&SPIBUFFER.au8Buffer[0],3);
					SPIBUFFER.u8Count = 4;
					SPIBUFFER.spiState == READ_RETURN;
				break;
				
				case 0xC3:
					SPDR0 = 0x01;
					SPIBUFFER.au8Buffer[3] = 4;
					SPIBUFFER.au8Buffer[2] = u8spiData;
					SPIBUFFER.au8Buffer[1] = u8Temperature;
					SPIBUFFER.au8Buffer[0] = CRC8(&SPIBUFFER.au8Buffer[0],3);
					SPIBUFFER.u8Count = 4;
					SPIBUFFER.spiState == READ_RETURN;
				break;
				
				default:
					SPIBUFFER.spiState == IDLE;
				break;
			}
		break;
		
		case READ_RETURN:
			SPDR0 = SPIBUFFER.au8Buffer[SPIBUFFER.u8Count-1];
			SPIBUFFER.u8Count--;
			if(SPIBUFFER.u8Count == 0);
			{
				SPIBUFFER.spiState == DONE_READ;
			}
		break;
		
		case DONE_WRITE:
		
		break;
		
		case DONE_READ:
		
		break;

		case IDLE:

		break;
	}

}

ISR(PCINT1_vect)
{
	uint8_t u8CRC;
	
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
						case 0x01:
						if(SPIBUFFER.u8Count == 3)
						{
							enablePLED();
						}
						break;

						case 0x02:
						if(SPIBUFFER.u8Count == 3)
						{
							disablePLED();
						}
						break;

						case 0x03:
						if(SPIBUFFER.u8Count == 4)
						{
							if(SPIBUFFER.au8Buffer[2]>100)
							{
								u8Duty = 100;
							}
							else
							{
								u8Duty = SPIBUFFER.au8Buffer[2];
							}
						
							setDuty(u8Duty);
						}
						break;
					
						case 0x04:
						if(SPIBUFFER.u8Count == 3)
						{
							enableAudio();
						}
						break;

						case 0x05:
						if(SPIBUFFER.u8Count == 3)
						{
							standbyAudio();
						}
						break;
					
						case 0x06:
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
	
}



int main(void)
{
    uint8_t u8Duty = 0;
	uint16_t i;
	
	portInit();
	initPWM(u8Duty);
	startPWM();
	spiInitBuffer(&SPIBUFFER);
	spiSlaveInit();
	spiPcInt();

	wait_1ms(100);
	initAudio();

	
	
	sei();
	
	
    while (1) 
    {

    }
}

