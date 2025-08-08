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

#define BOARD bl702rd

#define USE_ADC 		0

#define USE_INA229 		1
#define USE_INA228 		1

#if USE_INA228
#define USE_I2C_DEV		1
#define I2C_DEV_POWER
#else
#define USE_I2C_DEV		0
#endif

//#define USE_GPIO_IRQ
#define USE_TIMER_IRQ


#endif /* MAIN_CONFIG_H_ */
