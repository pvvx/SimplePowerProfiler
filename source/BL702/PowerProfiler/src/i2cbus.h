/*
 * i2cbus.h
 *
 *  Created on: 14.01.2020
 *      Author: pvvx
 */

#ifndef _I2CBUS_H_
#define _I2CBUS_H_

int I2CBusTrEnd(void);

void I2CBusInit(unsigned int clk, unsigned int clock_stretching);
void I2CBusDeInit(void);
int I2CBusReadWord(unsigned char i2c_addr, unsigned char reg_addr, void *preg_data);
int I2CBusWriteWord(unsigned char i2c_addr, unsigned char reg_addr, unsigned short reg_data);

#endif /* _I2CBUS_H_ */
