/*
 * rgbooster.c
 *
 * Created: 01/05/2018 16:14:32
 *  Author: Mario
 */ 

#include <avr/io.h>
#include "utils.h"
#include "rgbooster.h"


void INT0_Init(void)
{
	EICRA |= (1<<ISC01); // interrupt on falling edge
	EIFR  = (1<<INTF0); // clear flag
	EIMSK = (1<<INT0);  // INT1 enable
}

void initRGBooster(void)
{
	DDR_DATA_LOW |= DATA_LOW_BITMASK; //RGB DATA LOWER NIBBLE -> OUTPUT
	PORT_DATA_LOW &= ~DATA_LOW_BITMASK; //RGB DATA LOWER NIBBLE -> LOW
	DDR_DATA_HIGH |= DATA_HIGH_BITMASK; //RGB DATA HIGHER NIBBLE -> OUTPUT
	PORT_DATA_HIGH &= ~DATA_HIGH_BITMASK; //RGB DATA HIGHER NIBBLE -> LOW

	DDR_CONTROL |= (1<<SEND); //SEND PIN -> OUTPUT
	PORT_CONTROL &= ~(1<<SEND); //SEND PIN LOW
	DDR_CONTROL &= ~(1<<DONE_BUSY); //DONE BUSY PIN -> INPUT
}