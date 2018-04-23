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


ISR(SPI_STC_vect)
{
	
}

ISR(PCINT1_vect)
{
	if(PIN_SPI & (1<<SPI_SS))
	{
		enablePLED();
	}
	
}


int main(void)
{
    uint8_t u8Duty = 0;
	uint16_t i;
	
	portInit();
	initPWM(10);
	startPWM();
	//spiSlaveInit();
	spiPcInt();

	
	
	sei();
	
	
    while (1) 
    {

    }
}

