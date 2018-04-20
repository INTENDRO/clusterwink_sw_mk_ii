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
	uint16_t i;
	
	portInit();
	initPWM(0);
	startPWM();
	//enablePLED();
	enableAudio();
	
// 	for(i=0;i<4;i++)
// 	{
// 		PORT_VOL |= (1<<VOL_UD);
// 		DDR_VOL |= (1<<VOL_UD);
// 		wait_1ms(1);
// 		
// 		DDR_VOL &= ~(1<<VOL_UD);
// 		PORT_VOL &= ~(1<<VOL_UD);
// 		wait_1ms(1);
// 	}
	
    while (1) 
    {
// 		setDuty(0);
// 		wait_1ms(1000);
// 		setDuty(1);
// 		wait_1ms(1000);
    }
}

