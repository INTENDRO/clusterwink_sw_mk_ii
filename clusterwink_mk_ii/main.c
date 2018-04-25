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
				SPIBUFFER.spiState = IDLE;
			}
		break;

		case READ:

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
						setDuty(SPIBUFFER.au8Buffer[2]);
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
	initPWM(1);
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

