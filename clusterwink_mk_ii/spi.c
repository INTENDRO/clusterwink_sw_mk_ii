/** ***************************************************************************
 * @file spi.c
 * @brief Initializing the SPI slave interface
 *
 * The SPI interface is used to exchange data with the Raspberry Pi (RPi).
 * Only the slave functionality is implemented since the RPi will always be
 * the master. The Interrupt Service Routine (ISR) in the main.c file processes
 * the received data.
 *
 * @author lopeslen, nosedmar
 * @date 29.12.2017
 *****************************************************************************/

#include <avr/io.h>
#include "spi.h"

/** ***************************************************************************
 * @brief Initializes the SPI peripheral as a slave
 * 
 * Enables the SPI peripheral and its interrupt source. Pin directions of 
 * !SS, SCK and MOSI are automatically overriden to "INPUT". MISO is user
 * defined and is set as an output to be able to send data back to the master.
 * SCK is idle low and data is sampled on the rising edge of SCK.
 * 
 * @param [void] no input
 * @return no return value
 *****************************************************************************/
void spiSlaveInit(void)
{
	volatile uint8_t ucTemp;
	SPSR0 = 0;
	SPCR0 = (1<<SPIE0) | (1<<SPR10) | (1<<SPR00);
	
	ucTemp = SPSR0;					
	ucTemp = SPDR0;
	
	DDRB |= (1<<PINB6);				// MISO needs to be an output
	
	SPCR0 |= (1<<SPE0);
}