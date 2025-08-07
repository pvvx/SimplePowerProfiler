/*
 * i2c_dev.h
 *
 *  Created on: 22.02.2020
 *      Author: pvvx
 */

#ifndef I2C_DEV_H_
#define I2C_DEV_H_

#if (USE_I2C_DEV)

#include "i2cbus.h"
#include "cmd_cfg.h"

// I2C DEV config (store EEP_ID_I2C_CFG)
extern dev_i2c_cfg_t cfg_i2c;
// default config
//extern dev_i2c_cfg_t def_cfg_i2c;

// I2C DEV flags & buffers
#define I2C_BUF_SIZE 256 // equ 256, 512, ...

#if (I2C_BUF_SIZE != 256)
#error "I2C_BUF_SIZE: Only 256 word!"
#endif

typedef struct {
    reg_rd_t * raddr;               // указатель на адреса для чтения регистров в cfg_i2c
    volatile int timer_flg;         // flag временного отключения чтения I2C regs по irq
    volatile uint8_t i2c_buf_wr;    // указатель записи в буфере i2c_buf
    volatile uint8_t i2c_buf_rd;    // указатель чтения в буфере i2c_buf
    uint16_t i2c_buf[I2C_BUF_SIZE]; // буфер приема по таймеру с i2с, 512 bytes
} i2c_dev_t;

extern i2c_dev_t i2c_dev;

extern blk_tx_pkt_t send_pkt; // буфер подготовки пакета к отправке в Fifo USB

/* Flush I2C Buffer */
void I2CDevFlushBuf(void);
/* Timer Stop */
void I2CDevTimerStop(void);
/* Timer Start */
void I2CDevTimerStart(void);
/* Timer Init (IRQ)*/
void I2CDevTimerInit(uint32_t period_us);
/* I2C Device go Sleep */
void I2CDevSleep();
/* I2C Device WakeUp */
void I2CDevWakeUp();
/* Init I2C Device */
void I2CDevInit(void);
/* Start I2C Device */
int I2CDevStart(void);

int I2CDevWriteSetings(void);
int I2CDevReadSetings(void);

/* Task I2C Device */
void I2CDevTask(void);

#if DEBUG_I2C
#define i2c_printf(X...) printf(X)
#else
#define i2c_printf(X...)
#endif

#endif // USE_I2C_DEV
#endif /* I2C_DEV_H_ */
