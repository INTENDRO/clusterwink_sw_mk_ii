/*
 * rgbooster.h
 *
 * Created: 01/05/2018 16:14:44
 *  Author: Mario
 */ 


#ifndef RGBOOSTER_H_
#define RGBOOSTER_H_


#define PORT_DATA_LOW		PORTB
#define PIN_DATA_LOW		PINB
#define DDR_DATA_LOW		DDRB
#define DATA_LOW_BITMASK	0x0F

#define PORT_DATA_HIGH		PORTA
#define PIN_DATA_HIGH		PINA
#define DDR_DATA_HIGH		DDRA
#define DATA_HIGH_BITMASK	0xF0

#define PORT_CONTROL		PORTD
#define PIN_CONTROL			PIND
#define DDR_CONTROL			DDRD
#define SEND				3
#define DONE_BUSY			2

void INT0_Init(void);
void initRGBooster(void);


#endif /* RGBOOSTER_H_ */