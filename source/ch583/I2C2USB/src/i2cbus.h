/*
 * i2cbus.h
 *  Author: pvvx
 */

#ifndef _I2CBUS_H_
#define _I2CBUS_H_


//int I2CBusTrEnd(void);

void I2CBusInit(); // clk | 0x8000 - stretch enable
void I2CBusDeInit(void);
int I2CBusReadWord(unsigned char i2c_addr, unsigned char reg_addr, void *preg_data);
int I2CBusRead24bits(unsigned char i2c_addr, unsigned char reg_addr, void *preg_data);
int I2CBusWriteWord(unsigned char i2c_addr, unsigned char reg_addr, unsigned short reg_data);

/* Universal I2C/SMBUS read-write transaction struct */
typedef struct _i2c_utr_t {
  unsigned char mode;  // bit0..6: number wr_byte for new START (bit7: =1 - generate STOP/START)
  unsigned char rdlen; // bit0..6: number rd_byte (bit7: =1 - old read byte generate NACK, =0 - ACK)
  unsigned char wrdata[1]; // i2c_addr_wr, wr_byte1, wr_byte2, wr_byte3, ... wr_byte126
} i2c_utr_t;
int I2CBusUtr(void * outdata, i2c_utr_t *tr, unsigned int wrlen);

#endif /* _I2CBUS_H_ */
