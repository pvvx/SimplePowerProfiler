/*
 * common.h
 *
 *  Created on: Jul 25, 2025
 *      Author: pvvx
 */

#ifndef SRC_COMMON_H_
#define SRC_COMMON_H_

#include "CH58x_common.h"

#define bl702dk 10
#define bl702zb 11
#define bl702rd 12

#define USE_I2C_DEV   1
#define USE_USB_CDC   1
#define USE_ADC_DEV   0 // TODO

// no implementation
#define USE_UART_DEV  0
#define USE_DAC_DEV   0
#define DAC_DMA_ENABLE  0
#define TESTS_DAC_STEP_ADC  0

#define I2C_DEV_POWER GPIO_Pin_23 // PortB
#define I2C_DEV_SDA   GPIO_Pin_12 // PortB
#define I2C_DEV_SCL   GPIO_Pin_13 // PortB

//#define GPIO_TEST     GPIO_Pin_7 // PortB

#define USE_TEST_ADC    0 // =0 COM Echo
#define USE_TIMER_SPS   0 // =1 debug print sps adc

#define DEBUG_USB       0 // =DEBUG debug print usb
#define DEBUG_I2C       0 // =DEBUG debug print i2c



#endif /* SRC_COMMON_H_ */
