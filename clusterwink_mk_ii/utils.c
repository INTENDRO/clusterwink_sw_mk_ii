/** ***************************************************************************
 * @file utils.c
 * @brief Standard utilities for uC internals (timer,adc,gpio,...)
 *
 * - GPIO
 * @n Set pin directions and enable/disable the power LED driver circuit.
 * 
 * - PWM
 * @n Setup and control the PWM pin used for dimming the power LED
 * 
 * - ADC
 * @n Measure the voltage of the temperature sensor to determine the power LED temperature.
 * 
 * - Utilities
 * @n Map function for converting variables to a different number range and a 1ms wait routine.
 *
 * @author lopeslen, nosedmar
 * @date 14.11.2017
 *****************************************************************************/

#include <avr/io.h>
#include "utils.h"


///////////////////////////////////////////////////////////////////////////////
// GPIO
///////////////////////////////////////////////////////////////////////////////


/** ***************************************************************************
 * @brief Initialize the GPIOs (power LED enable and PWM pin)
 * 
 * @param [void] no input
 * @return no return value
 *****************************************************************************/
void portInit(void)
{
	DDR_PLED |= (1<<PLED_ENABLE) | (1<<PLED_PWM);
	PORT_PLED |= (1<<PLED_ENABLE); // disable PLED driver
	PORT_PLED &= ~(1<<PLED_PWM); // PWM pin low
	
	DDR_VOL |= (1<<VOL_MUTE);
	PORT_VOL |= (1<<VOL_MUTE); // mute pin high -> standby
	DDR_VOL &= ~(1<<VOL_UD); // up/down pin floating -> no change of volume
	
	DDR_UART &= ~((1<<UART_RX)|(1<<UART_TX));
	PORT_UART |= UART_TX | UART_RX;
	
	DDR_TEMP &= ~(1<<TEMP_ADC);
	PORT_TEMP &= ~(1<<TEMP_ADC); // disable the pull-up resistor
	DIDR0 |= (1<<TEMP_ADC); // disable the digital input buffer on the ADC pin
	
	// UNUSED PINS: SET AS INPUT WITH PULL-UP RESISTOR
	DDRA &= ~0b00001110;
	PORTA |= 0b00001110;
	
	DDRC &= ~0b11111111;
	PORTC |= 0b11111111;
}


/** ***************************************************************************
 * @brief Enables the power LED by pulling the enable pin low
 * 
 * @param [void] no input
 * @return no return value
 *****************************************************************************/
void enablePLED(void)
{
	PORT_PLED &= ~(1<<PLED_ENABLE); 
}


/** ***************************************************************************
 * @brief Disables the power LED by pulling the enable pin high
 * 
 * @param [void] no input
 * @return no return value
 *****************************************************************************/
void disablePLED(void)
{
	PORT_PLED |= (1<<PLED_ENABLE);
}


/** ***************************************************************************
 * @brief Pulls mute pin high (trinary pin)
 * 
 * @param [void] no input
 * @return no return value
 *****************************************************************************/
void standbyAudio(void)
{
	PORT_VOL |= (1<<VOL_MUTE);
	DDR_VOL |= VOL_MUTE;
}


/** ***************************************************************************
 * @brief Lets mute pin float (trinary pin)
 * 
 * @param [void] no input
 * @return no return value
 *****************************************************************************/
void muteAudio(void)
{
	DDR_VOL &= ~(1<<VOL_MUTE);
}


/** ***************************************************************************
 * @brief Pulls mute pin low (trinary pin)
 * 
 * @param [void] no input
 * @return no return value
 *****************************************************************************/
void enableAudio(void)
{
	PORT_VOL &= ~(1<<VOL_MUTE);
	DDR_VOL |= VOL_MUTE;
}


///////////////////////////////////////////////////////////////////////////////
// PWM
///////////////////////////////////////////////////////////////////////////////


/** ***************************************************************************
 * @brief Initialize the PWM pin and setup timer1.
 *
 * - non-interted output of pwm signal
 * - mode: 9bit fast pwm
 * - frequency @20MHz and prescaler 1: 39kHz
 * - no interrupts
 * 
 * @param [in] ucPercent: dutycycle in percent [0-100]
 * @return no return value
 *****************************************************************************/
void initPWM(uint8_t ucPercent)
{
	if(ucPercent>100)
	{
		ucPercent = 100;				// catch too high percentages
	}
	
	TCCR1A = (1<<COM1B1) | (1<<WGM11);	// non inverting output on pwm pin | mode: fast pwm 9bit
	TCCR1B = (1<<WGM12);				// mode: fast pwm 9bit
	TCNT1 = 0;							// clear counter
	OCR1B = Map(ucPercent,0,100,0,511); // dutycycle | converted from percent into 9bit value
	TIMSK1 = 0;							// no interrupts
}


/** ***************************************************************************
 * @brief Start the PWM output
 *
 * Timer1 is started by setting the prescaler to a non-zero value.
 * A prescaler of 1 yields the desired 39kHz.
 * 
 * @param [void] no input
 * @return no return value
 *****************************************************************************/
void startPWM(void)
{
	TCCR1B |= 0b00000001; // set prescaler to 1
}


/** ***************************************************************************
 * @brief Stop the PWM output
 *
 * Timer1 is stopped by setting the prescaler to zero.
 * 
 * @param [void] no input
 * @return no return value
 *****************************************************************************/
void stopPWM(void)
{
	TCCR1B &= ~0b00000111; // clear prescaler (turn off the counter)
}


/** ***************************************************************************
 * @brief Set the dutycycle of the PWM output
 *
 * The map function converts the percentage to the corresponding 9bit value
 * 
 * @param [in] ucPercent: dutycycle in percent [0-100]
 * @return no return value
 *****************************************************************************/
void setDuty(uint8_t ucPercent)
{
	if(ucPercent>100)
	{
		ucPercent = 100;				// catch too high percentages
	}
	OCR1B = Map(ucPercent,0,100,0,511);
}


///////////////////////////////////////////////////////////////////////////////
// ADC
///////////////////////////////////////////////////////////////////////////////


/** ***************************************************************************
 * @brief Initialize the ADC for the temperature measurement (ADC0)
 *
 * - reference: AVCC pin
 * - left adjusted (8bit)
 * - channel: ADC0 pin
 * - ADC clock prescaler: 128
 * - freerunning
 * 
 * @param [void] no input
 * @return no return value
 *****************************************************************************/
void adcInit(void)
{
	DDRA &= ~(1<<PINA0); // define ADC0 as input
	DIDR0 |= (1<<ADC0D); // disable digital circuitry on ADC0
	
	ADMUX = (1<<REFS0) | (1<<ADLAR); // AVCC as reference | left adjusted result (8bit) | ADC0 pin
	ADCSRA = (1<<ADATE) | (1<<ADIF) | (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0); // ADC enable | ADC free running mode | ADC clock prescaler 128
	ADCSRB = 0x00;
	
	ADCSRA |= (1<<ADEN);
}


/** ***************************************************************************
 * @brief Get last ADC value
 * 
 * @param [void] no input
 * @return 8bit analog value
 *****************************************************************************/
uint8_t adcGetValue(void)
{
	return(ADCH);
}

///////////////////////////////////////////////////////////////////////////////
// UTILITIES
///////////////////////////////////////////////////////////////////////////////


/** ***************************************************************************
 * @brief wait 1ms*factor
 * 
 * Use timer0 to wait a multiple of 1ms. Timermode: CTC
 *
 * @param [in] uiFactor: 1ms multiplier
 * @return no return value
 *****************************************************************************/
void wait_1ms(uint16_t uiFactor)
{
	uint16_t i;
	TCCR0A = (1<<WGM01);	// mode: CTC
	TCCR0B = 0;				// mode: CTC
	TIFR0 =  (1<<OCF0A);	// only OCFA flag needs to be cleared
	TIMSK0 = 0;				// no interrupts
	TCNT0 = 0;
	OCR0A = 77;			// used formula on page 99 in datasheet to calculate this value and prescaler for 1ms //77
	
	TCCR0B |= 0x04;			// set prescaler to 256 //0x04
	
	for(i=0;i<uiFactor;i++)
	{
		while(!(TIFR0&(1<<OCF0A)));	// poll flag
		TIFR0 =  (1<<OCF0A);		// clear flag
	}
	TCCR0B &= ~(0x07);	// set prescaler to 0 (stop timer)
}


/** ***************************************************************************
 * @brief Map function for converting variables to a different number range.
 *
 * @param [in] lData: data to be converted
 * @param [in] InMin: minimal value of input range
 * @param [in] InMax: maximal value of input range
 * @param [in] OutMin: minimal value of output range
 * @param [in] OutMax: maximal value of output range
 * @return data in new number range
 *****************************************************************************/
int32_t Map(int32_t lData, int32_t InMin, int32_t InMax, int32_t OutMin, int32_t OutMax)
{
	return((lData-InMin)*(OutMax-OutMin)/(InMax-InMin)+OutMin);
}
