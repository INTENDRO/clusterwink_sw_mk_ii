/** ***************************************************************************
 * @file utils.h
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
 * @n Map function for converting variable to a different number range and a 1ms wait routine.
 *
 * @author lopeslen, nosedmar
 * @date 14.11.2017
 *****************************************************************************/


#ifndef UTILS_H_
#define UTILS_H_

#define PORT_PLED			PORTD	///< output port for power LED
#define PIN_PLED			PIND	///< input port for power LED
#define DDR_PLED			DDRD	///< data direction register for power LED
#define PLED_ENABLE			5		///< power LED enable pin
#define PLED_PWM			4		///< power LED PWM pin

#define PORT_VOL			PORTD	///< output port for volume control
#define PIN_VOL				PIND	///< input port for volume control
#define DDR_VOL				DDRD	///< data direction register for volume control
#define VOL_UD				6		///< volume control: up/down pin
#define VOL_MUTE			7		///< volume control: mute pin

#define PORT_UART			PORTD	///< output port for UART interface
#define PIN_UART			PIND	///< input port for UART interface
#define DDR_UART			DDRD	///< data direction register for UART interface
#define UART_RX				0		///< UART receive pin
#define UART_TX				1		///< UART transmit pin

#define PORT_TEMP			PORTA	///< output port for UART interface
#define PIN_TEMP			PINA	///< input port for UART interface
#define DDR_TEMP			DDRA	///< data direction register for UART interface
#define TEMP_ADC			0		///< ADC pin for temperature measurement


//GPIO
void portInit(void);
void enablePLED(void);
void disablePLED(void);

void standbyAudio(void);
void muteAudio(void);
void enableAudio(void);
void incVolume(uint8_t u8steps);
void decVolume(uint8_t u8steps);
void setVolume(uint8_t u8DesiredVolume);
void initAudio(void);

//PWM
void initPWM(uint8_t ucPercent);
void startPWM(void);
void stopPWM(void);
void setDuty(uint8_t ucPercent);

//ADC
void adcInit(void);
uint8_t adcGetValue(void);
uint8_t adcGetTemperature(void);

//UTILITIES
void wait_1ms(uint16_t uiFactor);
int32_t Map(int32_t s32Data, int32_t s32InMin, int32_t s32InMax, int32_t s32OutMin, int32_t s32OutMax);
uint8_t CRC8(uint8_t* au8Data, uint8_t u8Length);


#endif /* UTILS_H_ */