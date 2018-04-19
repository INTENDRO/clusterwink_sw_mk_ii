/*
 * clusterwink_mk_ii.c
 *
 * Created: 09/04/2018 16:52:30
 * Author : Mario
 */ 

#include <avr/io.h>
#include "utils.h"


int main(void)
{
    uint8_t u8Duty = 0;
	
	portInit();
	initPWM(0);
	startPWM();
	enablePLED();
	
    while (1) 
    {
		setDuty(0);
		wait_1ms(1000);
		setDuty(1);
		wait_1ms(1000);
    }
}

