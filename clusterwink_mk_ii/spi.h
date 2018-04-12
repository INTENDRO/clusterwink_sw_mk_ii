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


void spiSlaveInit(void);


#endif /* SPI_H_ */