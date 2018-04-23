/** ***************************************************************************
 * @file spi.h
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


#ifndef SPI_H_
#define SPI_H_


#define PORT_SPI			PORTB	///< output port for power LED
#define PIN_SPI				PINB	///< input port for power LED
#define DDR_SPI				DDRB	///< data direction register for power LED
#define SPI_SS				4		///< power LED enable pin
#define SPI_SCK				7		///< power LED PWM pin
#define SPI_MOSI			5		///< power LED PWM pin
#define SPI_MISO			6		///< power LED PWM pin

void spiSlaveInit(void);
void spiPcInt(void);

#endif /* SPI_H_ */