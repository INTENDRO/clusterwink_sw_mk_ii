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
    portInit();
	initPWM(40);
	startPWM();
	
    while (1) 
    {
		enablePLED();
		wait_1ms(100);
		disablePLED();
		wait_1ms(100);
    }
}

