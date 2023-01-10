/*
 * main_config.h
 *
 *  Created on: 19 нояб. 2022 г.
 *      Author: pvvx
 */

#ifndef MAIN_CONFIG_H_
#define MAIN_CONFIG_H_

#define bl702dk 10
#define bl702zb 11
#define bl702rd 12

#ifndef BOARD
#define BOARD bl702rb
#endif

#define USE_I2C_DEV		1
#define USE_USB_CDC		1
#define USE_ADC_DEV 	1

// no implementation
#define USE_UART_DEV	0
#define USE_DAC_DEV		0
#define DAC_DMA_ENABLE	0
#define TESTS_DAC_STEP_ADC	0

#define I2C_DEV_POWER	GPIO_PIN_2
#define I2C_DEV_SDA		GPIO_PIN_1
#define I2C_DEV_SCL		GPIO_PIN_0

#endif /* MAIN_CONFIG_H_ */
