/*
 * i2c_dev.c
 *
 *  Created on: 22.02.2020
 *      Author: pvvx
 */

#include "common.h"

#if (USE_I2C_DEV)
#include "app_drv_fifo.h"
#include "app_usb.h"
#include "cmd_cfg.h"
#include "i2c_dev.h"

extern volatile uint32_t all_read_count;
extern volatile uint32_t all_overflow_cnt; // overflow

#define MAX_I2C_PKTCNT 60 // (SMPS_BLK_CNT/2)

i2c_dev_t i2c_dev; // I2C DEV flags & buffers

typedef struct __attribute__((packed)) _i2c_blk_t{
	uint8_t count;
	uint8_t id;
	uint16_t data[MAX_I2C_PKTCNT];
} i2c_blk_t;

i2c_blk_t i2c_blk;


// I2C DEV config (store EEP_ID_I2C_CFG)
dev_i2c_cfg_t cfg_i2c  = {
		.pktcnt = 0, // max = SMPS_BLK_CNT;
		.multiplier = 0,
		.time = 10000, // us
		.clk_khz = 1000,
		.init[0].dev_addr = 0x80,
		.init[0].reg_addr = 0x0,
		.init[1].data = 0xc005,
		.init[1].dev_addr = 0x80,
		.init[1].reg_addr = 0x00,
		.init[1].data = 0xc005,
		.init[2].dev_addr = 0x80,
		.init[2].reg_addr = 0x00,
		.init[2].data = 0x4005,
		.init[3].dev_addr = 0x00,
		.init[3].reg_addr = 0x00,
		.init[3].data = 0x0000,
		.rd[0].dev_addr = 0x81,
		.rd[0].reg_addr = 0x01,
		.rd[1].dev_addr = 0x81,
		.rd[1].reg_addr = 0x02,
		.rd[2].dev_addr = 0x00,
		.rd[2].reg_addr = 0x00,
		.rd[3].dev_addr = 0x00,
		.rd[3].reg_addr = 0x00,
		.slp[0].dev_addr = 0x80,
		.slp[0].reg_addr = 0x00,
		.slp[0].data = 0x0000,
		.slp[1].dev_addr = 0x00,
		.slp[1].reg_addr = 0x00,
		.slp[1].data = 0x0000
};

/*********************************************************************
 * @fn      TMR0_IRQHandler
 *
 * @brief   TMR0 IRQ
 *
 * @return  none
 */
__INTERRUPT
__HIGH_CODE
void TMR1_IRQHandler(void) {
#ifdef GPIO_TEST
  GPIOB_SetBits(GPIO_TEST);
#endif
  if(TMR1_GetITFlag(RB_TMR_IF_CYC_END)) {
	  if(i2c_dev.timer_flg) {
	    I2CBusReadWord(i2c_dev.raddr->dev_addr, i2c_dev.raddr->reg_addr, &i2c_dev.i2c_buf[i2c_dev.i2c_buf_wr]);
	    i2c_dev.i2c_buf_wr++;
	    i2c_dev.raddr++;
	    if(i2c_dev.raddr->dev_addr == 0 || i2c_dev.raddr > &cfg_i2c.rd[MAX_READ_REGS-1]) {
	      i2c_dev.raddr = &cfg_i2c.rd[0];
	    }
	  }
    TMR1_ClearITFlag(RB_TMR_IF_CYC_END);
	}
#ifdef GPIO_TEST
	GPIOB_ResetBits(GPIO_TEST);
#endif
}

/* Flush I2C Buffer */
void FlushI2CBuf(void) {
  i2c_printf("Flush I2C Buffer\n");
  i2c_dev.raddr = &cfg_i2c.rd[0]; //	rd_next_cnt = 0;
  i2c_dev.i2c_buf_wr = 0;
  i2c_dev.i2c_buf_rd = 0;
}
/* Read Timer (IRQ) Stop */
void Timer_Stop(void) {
  i2c_printf("I2C: Timer Stop\r\n");
  i2c_dev.timer_flg = 0;
  R8_TMR1_CTRL_MOD = 0;
}

void Timer_Start(void) {
  i2c_printf("I2C: Timer Start\r\n");
  R8_TMR1_CTRL_MOD = RB_TMR_ALL_CLEAR;
  R8_TMR1_CTRL_MOD = RB_TMR_COUNT_EN;
}

/* Timer Init (IRQ)*/
void Timer_Init(uint32_t period_us) {
  i2c_printf("I2C: Timer %i us\r\n", period_us);

  R32_TMR1_CNT_END = (FREQ_SYS/1000000)*period_us;
  R8_TMR1_CTRL_MOD = RB_TMR_ALL_CLEAR;
  R8_TMR1_CTRL_MOD = 0;
  //i2c_printf("I2C: Timer %i cnt\r\n", R32_TMR1_CNT_END);

  TMR1_ITCfg(ENABLE, TMR0_3_IT_CYC_END);
  PFIC_EnableIRQ(TMR1_IRQn);
}


/* I2C Device go Sleep */
void I2CDevSleep(void) {
	cfg_i2c.pktcnt = 0;
	Timer_Stop();
	if(cfg_i2c.slp[0].dev_addr) {
	  i2c_printf("I2C: Set CLK %i kHz, stretch %u\n", cfg_i2c.clk_khz & 0x7fff, cfg_i2c.clk_khz >> 15);
		I2CBusInit();
		if(I2CBusWriteWord(cfg_i2c.slp[0].dev_addr,cfg_i2c.slp[0].reg_addr, cfg_i2c.slp[0].data)) // Power-Down (or Shutdown)
      PRINT("I2C: Error write addr: %02x:%02x, data: %04x!\r\n", cfg_i2c.slp[0].dev_addr, cfg_i2c.slp[0].reg_addr, cfg_i2c.slp[0].data);

		if(cfg_i2c.slp[1].dev_addr) {
		  if(I2CBusWriteWord(cfg_i2c.slp[1].dev_addr,cfg_i2c.slp[1].reg_addr, cfg_i2c.slp[1].data)) // Power-Down (or Shutdown)
		    PRINT("I2C: Error write addr: %02x:%02x, data: %04x!\r\n", cfg_i2c.slp[1].dev_addr, cfg_i2c.slp[1].reg_addr, cfg_i2c.slp[1].data);
		}
	}
	I2CBusDeInit();
#ifdef I2C_DEV_POWER
	i2c_printf("I2C: Power Off\r\n");
	GPIOB_ResetBits(I2C_DEV_POWER);
  GPIOB_ModeCfg(I2C_DEV_POWER | I2C_DEV_SCL | I2C_DEV_SDA, GPIO_ModeIN_Floating);
#endif
}

/* I2C Device WakeUp */
void I2CDevWakeUp(void) {
#ifdef I2C_DEV_POWER
	  GPIOB_ModeCfg(I2C_DEV_POWER | I2C_DEV_SCL | I2C_DEV_SDA, GPIO_ModeIN_PU);
	  DelayUs(128);
	  GPIOB_SetBits(I2C_DEV_POWER);
    GPIOB_ModeCfg(I2C_DEV_POWER, GPIO_ModeOut_PP_5mA);
    i2c_printf("I2C: Power On\r\n");
    GPIOB_ModeCfg(I2C_DEV_POWER, GPIO_ModeOut_PP_20mA);
    DelayMs(2);
#endif
    cfg_i2c.pktcnt = 0;
    InitI2CDevice();
}

/* Init I2C Device */
int InitI2CDevice(void) {
	uint32_t i;
	uint32_t clk_khz;
	uint32_t t_rd_us;
	i2c_dev.timer_flg = 0;
  if(cfg_i2c.pktcnt > MAX_I2C_PKTCNT)
		cfg_i2c.pktcnt = MAX_I2C_PKTCNT;
	clk_khz = cfg_i2c.clk_khz & 0x7fff;
	if(clk_khz < 50 || clk_khz > 1200) {
		clk_khz = 1200;
		cfg_i2c.clk_khz &= 0x8000;
		cfg_i2c.clk_khz |= clk_khz;
	}
	i2c_printf("I2C: Set CLK %i kHz, stretch %u\n", cfg_i2c.clk_khz & 0x7fff, cfg_i2c.clk_khz >> 15);
  I2CBusInit();
	t_rd_us = cfg_i2c.time << cfg_i2c.multiplier;
	if(t_rd_us < (7*10*1000)/clk_khz) { // 2*5w*10bit*1000000/clk = us
		cfg_i2c.pktcnt = 0;
		Timer_Stop();
		PRINT("I2C: Error timing - step read: %i us, i2c clk: %i kHz!\r\n", t_rd_us, clk_khz);
		return 0; // error t_us
	}
	FlushI2CBuf();
	if (cfg_i2c.pktcnt) {
		Timer_Init(t_rd_us);
		// start (new) counts
		all_read_count = 0;
		all_overflow_cnt = 0;
	} else {
		Timer_Stop();
		// выход по Stop (cfg_i2c.rd_count = 0), init dev i2c only
	}
	for(i = 0; i < MAX_INIT_REGS && cfg_i2c.init[i].dev_addr; i++) {
	  DelayUs(200);
		if (I2CBusWriteWord(cfg_i2c.init[i].dev_addr, cfg_i2c.init[i].reg_addr, cfg_i2c.init[i].data)) {
			cfg_i2c.pktcnt = 0;
			Timer_Stop();
			PRINT("I2C: Error write addr: %02x:%02x, data: %04x!\r\n", cfg_i2c.init[i].dev_addr, cfg_i2c.init[i].reg_addr, cfg_i2c.init[i].data);
			// return 0; // error dev i2c
		}
	}
	if (cfg_i2c.pktcnt) {
		i2c_printf("I2C: blk %i word\r\n", cfg_i2c.pktcnt);
		if(i2c_dev.raddr->dev_addr) {
		  i2c_dev.timer_flg = 1;
			Timer_Start(); // Enable timer
		} else {
		  PRINT("I2C: Set read I2C addr!\r\n");
			return 0;
		}
	}
	return 1; // ok
}


void Task_I2C(void) {
	uint32_t prd = i2c_dev.i2c_buf_rd;
	uint32_t pwr = i2c_dev.i2c_buf_wr;
	uint32_t size;
	uint8_t *p = (uint8_t *)i2c_blk.data;
	if(!cfg_i2c.pktcnt)
		return;
	if(pwr < prd) {
		size = I2C_BUF_SIZE - prd + pwr;
		if(size >= cfg_i2c.pktcnt) {
			i2c_blk.count = cfg_i2c.pktcnt << 1;
			size = cfg_i2c.pktcnt;
			i2c_blk.id = CMD_DEV_I2C;
			for(int i = prd; i < I2C_BUF_SIZE && size; i++) {
				*p++ = (uint8_t)i2c_dev.i2c_buf[i];
        *p++ = (uint8_t)(i2c_dev.i2c_buf[i] >> 8);
				size--;
			}
			i2c_dev.i2c_buf_rd = (uint8_t)size;
			for(int i = 0; size; i++) {
				*p++ = (uint8_t)i2c_dev.i2c_buf[i];
        *p++ = (uint8_t)(i2c_dev.i2c_buf[i] >> 8);
				size--;
			}
		} else
			return;

	} else if (pwr > prd) {
		size = pwr - prd;
		if(size >= cfg_i2c.pktcnt) {
			i2c_blk.count = cfg_i2c.pktcnt << 1;
			size = cfg_i2c.pktcnt;
			i2c_blk.id = CMD_DEV_I2C;
			while(size) {
				*p++ = (uint8_t)i2c_dev.i2c_buf[prd];
        *p++ = (uint8_t)(i2c_dev.i2c_buf[prd++] >> 8);
				size--;
			}
			i2c_dev.i2c_buf_rd = (uint8_t)prd;
		} else
			return;
	} else
		return;
	size = 2 + i2c_blk.count;
  if(app_drv_fifo_write(&app_tx_fifo, (uint8_t *)&i2c_blk, (uint16_t *)&size) == APP_DRV_FIFO_RESULT_SUCCESS) {
    all_read_count++;
	} else {
	  PRINT("I2C: buffer overflow!\r\n");
		all_overflow_cnt++;
	}
}

#endif // USE_I2C_DEV
