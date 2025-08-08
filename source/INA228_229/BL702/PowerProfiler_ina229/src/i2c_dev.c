/*
 * i2c_dev.c
 *
 *  Created on: 22.02.2020
 *      Author: pvvx
 */
#include "main_config.h"
#if (USE_I2C_DEV)
#include "bl702_common.h"
#include "board.h"
#include "cmd_cfg.h"
#include "i2cbus.h"
#include "i2c_dev.h"
#include "hardware/i2c_reg.h"
#include "bflb_timer.h"
#include "hardware/timer_reg.h"
#include "ring_buffer.h"
#include "adc_dma.h"
#include "usb_buffer.h"
#include "bflb_gpio.h"
#include "..\SDK\drivers\soc\bl702\bl702_std\include\hardware\glb_reg.h"
#include "bflb_clock.h"


#define I2C_PREREAD	1

#define i2c_printf(...) printf(__VA_ARGS__)

extern volatile uint32_t all_read_count;
extern volatile uint32_t all_overflow_cnt; // overflow

extern struct bflb_device_s *i2c0;

// #define GPIO_TST GPIO_PIN_17 	// Test pin
#define MAX_I2C_PKTCNT 60 // (SMPS_BLK_CNT/2)
#define MIN_TIM_IRQ_US 45 // 45 us

struct bflb_device_s *timer1;

#ifdef I2C_DEV_POWER
struct bflb_device_s *gpio_pwr;
#endif

// I2C DEV flags
volatile int timer_flg = 0; // flag временного отключения чтения I2C regs по irq
uint32_t t_rd_us = 0; // flag и время в us работы опроса без таймера

// I2C DEV read buffers
reg_rd_t * raddr;
uint32_t i2c_buf[I2C_BUF_SIZE]; // 512 bytes

typedef struct __attribute__((packed)) _i2c_blk_t{
	uint8_t count;
	uint8_t id;
	uint16_t data[MAX_I2C_PKTCNT];
} i2c_blk_t;

i2c_blk_t i2c_blk;

volatile uint8_t i2c_buf_wr = 0;
volatile uint8_t i2c_buf_rd = 0;

// I2C DEV config (store EEP_ID_I2C_CFG)
dev_i2c_cfg_t cfg_i2c  = {
		.pktcnt = 0, // max = SMPS_BLK_CNT;
		.multiplier = 0,
		.time = 10000, // us
		.clk_khz = 800,
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
		.rd[0].dev_addr = 0x80,
		.rd[0].reg_addr = 0x01,
		.rd[1].dev_addr = 0x80,
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

#if I2C_PREREAD
static inline void I2CBusPreReadWord(unsigned char i2c_addr, unsigned char reg_addr)
{
    uint32_t reg_base = i2c0->reg_base;

    uint32_t regval = getreg32(reg_base + I2C_CONFIG_OFFSET);

    regval |= I2C_CR_I2C_PKT_DIR; // Transfer direction of the packet: 1 - read

    regval |= I2C_CR_I2C_SUB_ADDR_EN;
    regval &= ~I2C_CR_I2C_SUB_ADDR_BC_MASK;
    //regval |= 0 << I2C_CR_I2C_SUB_ADDR_BC_SHIFT);

    regval &= ~I2C_CR_I2C_SLV_ADDR_MASK;
    regval |= (i2c_addr << (I2C_CR_I2C_SLV_ADDR_SHIFT - 1)) & I2C_CR_I2C_SLV_ADDR_MASK;

    regval &= ~I2C_CR_I2C_PKT_LEN_MASK;
    regval |= 1 << I2C_CR_I2C_PKT_LEN_SHIFT;

    putreg32(reg_addr, reg_base + I2C_SUB_ADDR_OFFSET);
    regval |= I2C_CR_I2C_M_EN;
    putreg32(regval, reg_base + I2C_CONFIG_OFFSET);
}
#endif

static void reset_i2c0(void) {
	PERIPHERAL_CLOCK_I2C0_ENABLE();
	uint32_t tmpVal = getreg32(GLB_BASE+GLB_SWRST_CFG1_OFFSET);
	tmpVal |= GLB_SWRST_S1A3_MSK; // software reset I2C
	putreg32(tmpVal, GLB_BASE + GLB_SWRST_CFG1_OFFSET);
	tmpVal &= ~GLB_SWRST_S1A3_MSK; // software reset I2C
	putreg32(tmpVal, GLB_BASE + GLB_SWRST_CFG1_OFFSET);
}


static void timer1_isr(int irq, void *arg) {
#ifdef GPIO_TST
	bflb_gpio_reset(ugpio, GPIO_TST);
#endif
	uint32_t reg_base = timer1->reg_base;
	uint32_t regval = getreg32(reg_base + TIMER_TSR0_OFFSET + 4 * timer1->idx);

	if(timer_flg) {
#if I2C_PREREAD
		i2c_buf[i2c_buf_wr++] = getreg32(i2c0->reg_base + I2C_FIFO_RDATA_OFFSET);
		I2CBusTrEnd();
		I2CBusPreReadWord(raddr->dev_addr, raddr->reg_addr);
#else
		I2CBusReadWord(raddr->dev_addr, raddr->reg_addr, &i2c_buf[i2c_buf_wr++]);
#endif
		raddr++;
		if(raddr->dev_addr == 0 || raddr > &cfg_i2c.rd[MAX_READ_REGS-1]) {
			raddr = &cfg_i2c.rd[0];
		}
	}
	if (regval & 0x07) {
		regval |= getreg32(reg_base + TIMER_TICR0_OFFSET + 4 * timer1->idx);
		putreg32(regval, reg_base + TIMER_TICR0_OFFSET + 4 * timer1->idx);
	}
#ifdef GPIO_TST
	bflb_gpio_set(ugpio, GPIO_TST);
#endif
}

/* Flush I2C Buffer */
void FlushI2CBuf(void) {
	raddr = &cfg_i2c.rd[0]; //	rd_next_cnt = 0;
	i2c_buf_wr = 0;
	i2c_buf_rd = 0;
}
/* Read Timer (IRQ) Stop */
void Timer_Stop(void) {
	t_rd_us = 0;
	timer_flg = 0;
	if(timer1) {
		bflb_timer_stop(timer1);
		bflb_irq_disable(timer1->irq_num);
	}
}

/* Timer Init (IRQ)*/
void Timer_Init(uint32_t period_us) {
    /* timer clk = XCLK/(div + 1 )*/
	if(timer1) {
		bflb_timer_stop(timer1);
		bflb_irq_disable(timer1->irq_num);
	}

    struct bflb_timer_config_s timer1_cfg;
    timer1_cfg.counter_mode = TIMER_COUNTER_MODE_PROLOAD; /* preload when match occur */
    timer1_cfg.clock_source = TIMER_CLKSRC_XTAL;
    timer1_cfg.clock_div = 32; // 32000000 / 32 = 1000000 (1 us)
    timer1_cfg.trigger_comp_id = TIMER_COMP_ID_0;
    timer1_cfg.comp0_val = period_us-1;
	i2c_printf("I2C: Timer %i us\r\n", period_us);
    timer1_cfg.preload_val = 0;    /* preload value */

    timer1 = bflb_device_get_by_name("timer1");

    // Timer init with default configuration
    bflb_timer_init(timer1, &timer1_cfg);

    bflb_irq_attach(timer1->irq_num, timer1_isr, NULL);
//    bflb_irq_set_priority(timer1->irq_num, 1, 0);

    bflb_irq_enable(timer1->irq_num);

}


/* I2C Device go Sleep */
void I2CDevSleep() {
	cfg_i2c.pktcnt = 0;
	t_rd_us = 0;
	Timer_Stop();
	if(cfg_i2c.slp[0].dev_addr) {
		I2CBusInit(cfg_i2c.clk_khz & 0x7fff, cfg_i2c.clk_khz & 0x8000);
		I2CBusWriteWord(cfg_i2c.slp[0].dev_addr,cfg_i2c.slp[0].reg_addr, cfg_i2c.slp[0].data); // Power-Down (or Shutdown)
		if(cfg_i2c.slp[1].dev_addr)
			I2CBusWriteWord(cfg_i2c.slp[0].dev_addr,cfg_i2c.slp[0].reg_addr, cfg_i2c.slp[0].data); // Power-Down (or Shutdown)
	}
	I2CBusDeInit();
#ifdef I2C_DEV_POWER
	i2c_printf("I2C: Power Off\r\n");
	if(gpio_pwr) {
	    bflb_gpio_init(gpio_pwr, I2C_DEV_SDA, GPIO_OUTPUT | GPIO_FLOAT | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_1);
	    bflb_gpio_init(gpio_pwr, I2C_DEV_SCL, GPIO_OUTPUT | GPIO_FLOAT | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_1);
		bflb_gpio_reset(gpio_pwr, I2C_DEV_POWER);
	    bflb_gpio_reset(gpio_pwr, I2C_DEV_SDA);
	    bflb_gpio_reset(gpio_pwr, I2C_DEV_SCL);
	}
#endif
	reset_i2c0();
}

/* I2C Device WakeUp */
void I2CDevWakeUp(void) {
#ifdef I2C_DEV_POWER
    gpio_pwr = bflb_device_get_by_name("gpio");
    /* I2C0_SDA */
    bflb_gpio_init(gpio_pwr, I2C_DEV_SDA, GPIO_FUNC_I2C0 | GPIO_ALTERNATE | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_1);
    /* I2C0_SCL */
    bflb_gpio_init(gpio_pwr, I2C_DEV_SCL, GPIO_FUNC_I2C0 | GPIO_ALTERNATE | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_1);
    /* pin power */
    bflb_gpio_init(gpio_pwr, I2C_DEV_POWER, GPIO_OUTPUT | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_3);
	i2c_printf("I2C: Power On\r\n");
    bflb_mtimer_delay_ms(5);
    bflb_gpio_set(gpio_pwr, I2C_DEV_POWER);
    bflb_mtimer_delay_ms(50);
#endif
    cfg_i2c.pktcnt = 0;
    InitI2CDevice();
	// I2CBusWriteWord(0x18,0x00,0x0000);
}

/* Init I2C Device */
int InitI2CDevice(void) {
	uint32_t i;
	uint32_t clk_khz;
	reset_i2c0();
	timer_flg = 0;
    if(cfg_i2c.pktcnt > MAX_I2C_PKTCNT)
		cfg_i2c.pktcnt = MAX_I2C_PKTCNT;
	clk_khz = cfg_i2c.clk_khz & 0x7fff;
	if(clk_khz < 50 || clk_khz > 2500) {
		clk_khz = 1000;
		cfg_i2c.clk_khz &= 0x8000;
		cfg_i2c.clk_khz |= clk_khz;
	}

    I2CBusInit(clk_khz, cfg_i2c.clk_khz & 0x8000);
	t_rd_us = cfg_i2c.time << cfg_i2c.multiplier;
	if(t_rd_us < (7*10*1000)/clk_khz) { // 2*5w*10bit*1000000/clk = us
		cfg_i2c.pktcnt = 0;
		Timer_Stop();
		printf("I2C: Error timing - step read: %i us, i2c clk: %i kHz!\r\n", t_rd_us, clk_khz);
		return 0; // error t_us
	}
	FlushI2CBuf();
	if (cfg_i2c.pktcnt) {
		Timer_Init(t_rd_us);
		// start (new) counts
		all_read_count = 0;
		all_overflow_cnt = 0;
	} else {
		Timer_Stop(); // там t_rd_us = 0;
		// выход по Stop (cfg_i2c.rd_count = 0), init dev i2c only
	}
	for(i = 0; i < MAX_INIT_REGS && cfg_i2c.init[i].dev_addr; i++) {
	    bflb_mtimer_delay_us(200);
		if (!I2CBusWriteWord(cfg_i2c.init[i].dev_addr, cfg_i2c.init[i].reg_addr, cfg_i2c.init[i].data)) {
			cfg_i2c.pktcnt = 0;
			Timer_Stop();
			printf("I2C: Error write addr: %02x:%02x, data: %04x!\r\n", cfg_i2c.init[i].dev_addr, cfg_i2c.init[i].reg_addr, cfg_i2c.init[i].data);
			// return 0; // error dev i2c
		}
	}
	if (cfg_i2c.pktcnt) {
		i2c_printf("I2C: blk %i word\r\n", cfg_i2c.pktcnt);
		if(raddr->dev_addr) {
#if I2C_PREREAD
			I2CBusPreReadWord(raddr->dev_addr, raddr->reg_addr);
			raddr++;
			if(raddr->dev_addr == 0 || raddr > &cfg_i2c.rd[MAX_READ_REGS-1]) {
				raddr = &cfg_i2c.rd[0];
			}
#endif
			timer_flg = 1;
			bflb_timer_start(timer1); // Enable timer
		} else {
			printf("I2C: Set read I2C addr!\r\n");
			return 0;
		}
	}
	return 1; // ok
}


void task_I2C(void) {
	uint32_t prd = i2c_buf_rd;
	uint32_t pwr = i2c_buf_wr;
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
				*p++ = (uint8_t)(i2c_buf[i] >> 8);
				*p++ = (uint8_t)i2c_buf[i];
				size--;
			}
			i2c_buf_rd = (uint8_t)size;
			for(int i = 0; size; i++) {
				*p++ = (uint8_t)(i2c_buf[i] >> 8);
				*p++ = (uint8_t)i2c_buf[i];
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
				*p++ = (uint8_t)(i2c_buf[prd] >> 8);
				*p++ = (uint8_t)i2c_buf[prd++];
				size--;
			}
			i2c_buf_rd = (uint8_t)prd;
		} else
			return;
	} else
		return;
	size = 2 + i2c_blk.count;
	if(size == Ring_Buffer_Write(&usb_tx_rb, (uint8_t *)&i2c_blk, size)) {
		all_read_count++;
	} else {
		printf("I2C: buffer overflow!\r\n");
		all_overflow_cnt++;
	}

}

#endif // USE_I2C_DEV
