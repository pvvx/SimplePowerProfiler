/*
 * i2cbus.h
 *  Author: pvvx
 */

#ifndef _I2CBUS_H_
#define _I2CBUS_H_

int I2CBusTrEnd(void);

void I2CBusInit(); // clk | 0x8000 - stretch enable
void I2CBusDeInit(void);
int I2CBusReadWord(unsigned char i2c_addr, unsigned char reg_addr, void *preg_data);
int I2CBusRead24bits(unsigned char i2c_addr, unsigned char reg_addr, void *preg_data);
int I2CBusWriteWord(unsigned char i2c_addr, unsigned char reg_addr, unsigned short reg_data);

#endif /* _I2CBUS_H_ */
