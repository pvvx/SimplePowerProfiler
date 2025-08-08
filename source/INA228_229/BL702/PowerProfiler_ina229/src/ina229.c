/*
 * ina229.c
 *
 *  Created on: 3 дек. 2022 г.
 *      Author: pvvx
 */
#include "main_config.h"
#include "board.h"
#include "bflb_gpio.h"
#include "bflb_spi.h"
//#include "bflb_clock.h"
#include "hardware/spi_reg.h"
#include "bflb_mtimer.h"
#include "bflb_timer.h"
#include "hardware/timer_reg.h"
#include "ring_buffer.h"
#include "usb_buffer.h"

#include "cmd_cfg.h"
#include "ina229.h"
#include "..\SDK\drivers\soc\bl702\bl702_std\include\hardware\glb_reg.h"
#include "..\SDK\drivers\lhal\include\arch\risc-v\e24\clic.h"

#define GPIO_SCLK GPIO_PIN_15	// SPI0 SCLK
#define GPIO_SDO GPIO_PIN_0		// SPI0 SDO
#define GPIO_SDI GPIO_PIN_1		// SPI0 SDI
#define GPIO_NSS GPIO_PIN_2		// SPI0 NSS

#define GPIO_ALRT GPIO_PIN_24  	// INA229 Conversion Ready, pin ALERT
#define GPIO_PWR GPIO_PIN_26	// +3V3
//#define GPIO_TST GPIO_PIN_25 	// Test pin "CTS"
//#define GPIO_SHT GPIO_PIN_25 	// Shunt

struct bflb_device_s *ugpio;
struct bflb_device_s *uspi0;

#define INAWR16B(a,v) (((a)<<26) | ((v)<<8))
#define INARD16B(a) (((a)<<26) | (1<<24))
#define INAWR24B(a,v) (((a)<<26) | (v))
#define INARD24B(a) (((a)<<26) | (1<<24))

struct {
	uint16_t id;
	uint16_t data[DATA16_BLK_CNT];
} data_blk2;


cfg_ina_t cfg_ina;

#ifdef USE_TIMER_IRQ

struct bflb_device_s *timer1;

void timer1_isr(int irq, void *arg) {
#ifdef GPIO_TST
	bflb_gpio_reset(ugpio, GPIO_TST);


#endif
    /* read old data from fifo */
	uint32_t reg_base = uspi0->reg_base;
	//uint32_t regval = (getreg32(reg_base + SPI_FIFO_RDATA_OFFSET)); // << 8; // & 0x00ffffff;
	//int32_t ival = (regval << 24) | ((regval & 0xff0000) >> 8)  | ((regval & 0xff00) << 8);
	int32_t ival = (getreg32(reg_base + SPI_FIFO_RDATA_OFFSET)) << 8;
	/* wtire data to fifo */
    if(cfg_ina.chnls == 3) {
    	if(data_buffer_wr_ptr & 1) { // VBUS
#ifdef GPIO_SHT
    	    if(ival > 0x7ff00000 || ival < -0x7ff00000) { // ((0x7ffff - 0x7ffff/10)<<12)
    	    	// включить шунт bflb_gpio_reset(ugpio, GPIO_SHT);
    	        putreg32(getreg32(ugpio->reg_base + GLB_GPIO_CFGCTL32_OFFSET) & (~(1 << (GPIO_SHT & 0x1f))), ugpio->reg_base + GLB_GPIO_CFGCTL32_OFFSET);
    	    	cfg_ina.shunt = 0x100;
#if 1
    	    } else if (ival < 0x0C000000 && ival > -0x0C000000) {
    	    	// отключить шунт bflb_gpio_set(ugpio, GPIO_SHT);
    	        putreg32(getreg32(ugpio->reg_base + GLB_GPIO_CFGCTL32_OFFSET) | (1 << (GPIO_SHT & 0x1f)), ugpio->reg_base + GLB_GPIO_CFGCTL32_OFFSET);
    	    	cfg_ina.shunt = 0;
#endif
    	    }
#endif
    	    putreg32(INARD24B(INA229_RA_VBUS), reg_base + SPI_FIFO_WDATA_OFFSET); // read VBUS
    	} else { // VSHUNT
    	    putreg32(INARD24B(INA229_RA_VSHUNT), reg_base + SPI_FIFO_WDATA_OFFSET); // read VSHUNT
    	}
    } else if(cfg_ina.chnls == 2) { // VBUS
	    putreg32(INARD24B(INA229_RA_VBUS), reg_base + SPI_FIFO_WDATA_OFFSET); // read VBUS
    } else { // VSHUNT
#ifdef GPIO_SHT
	    if(ival > 0x7ff00000 || ival < -0x7ff00000) { // ((0x7ffff - 0x7ffff/10)<<12)
	    	// включить шунт bflb_gpio_reset(ugpio, GPIO_SHT);
	        putreg32(getreg32(ugpio->reg_base + GLB_GPIO_CFGCTL32_OFFSET) & (~(1 << (GPIO_SHT & 0x1f))), ugpio->reg_base + GLB_GPIO_CFGCTL32_OFFSET);
	    	cfg_ina.shunt = 0x100;
#if 1
	    } else if (ival < 0x0C000000 && ival > -0x0C000000) {
	    	// отключить шунт bflb_gpio_set(ugpio, GPIO_SHT);
	        putreg32(getreg32(ugpio->reg_base + GLB_GPIO_CFGCTL32_OFFSET) | (1 << (GPIO_SHT & 0x1f)), ugpio->reg_base + GLB_GPIO_CFGCTL32_OFFSET);
	    	cfg_ina.shunt = 0;
#endif
	    }
#endif
	    putreg32(INARD24B(INA229_RA_VSHUNT), reg_base + SPI_FIFO_WDATA_OFFSET); // read VSHUNT
    }
	if(cfg_ina.buf_enable) {
		data_buffer[data_buffer_wr_ptr++] = ival | cfg_ina.shunt;
		data_buffer_wr_ptr &= DATA_BUF_CNT-1;
	}
#ifdef GPIO_TST
	bflb_gpio_set(ugpio, GPIO_TST);
#endif
	/* Timer0 Interrupt Clear */
	reg_base = timer1->reg_base;
	uint32_t regval = getreg32(reg_base + TIMER_TSR0_OFFSET + 4 * timer1->idx);
	if (regval & 0x07) {
		regval |= getreg32(reg_base + TIMER_TICR0_OFFSET + 4 * timer1->idx);
		putreg32(regval, reg_base + TIMER_TICR0_OFFSET + 4 * timer1->idx);
	}
}
#endif

#ifdef USE_GPIO_IRQ
void gpio_isr(int irq, void *arg)
{
//    bool intstatus = bflb_gpio_get_intstatus(ugpio, GPIO_ALRT);
//    if (intstatus) {
#ifdef GPIO_TST
		bflb_gpio_reset(ugpio, GPIO_TST);
#endif
		bflb_gpio_int_clear(ugpio, GPIO_ALRT);
        /* read old data from fifo */
        uint32_t reg_base = uspi0->reg_base;
        uint32_t regval = getreg32(reg_base + SPI_FIFO_RDATA_OFFSET) & 0x00ffffff;
    	/* wtire data to fifo */
        putreg32(INARD24B(INA229_RA_VSHUNT), reg_base + SPI_FIFO_WDATA_OFFSET); // read VSHUNT

    	if(!cfg_ina.data_out_null) {
    		data_buffer[data_buffer_wr_ptr++] = regval;
    		data_buffer_wr_ptr &= DATA_BUF_CNT-1;
    	}
#ifdef GPIO_TST
		bflb_gpio_set(ugpio, GPIO_TST);
#endif
//    }
}
#endif

#ifdef USE_TIMER_IRQ
int timer_init(uint32_t comp) {
	struct bflb_timer_config_s timer1_cfg;
	if(timer1 && comp) {
	    timer1_cfg.counter_mode = TIMER_COUNTER_MODE_PROLOAD; /* preload when match occur */
	    timer1_cfg.clock_source = TIMER_CLKSRC_XTAL;
	    timer1_cfg.clock_div = 31; // 32000000 / 32 = 1000000
	    timer1_cfg.trigger_comp_id = TIMER_COMP_ID_0; // TIMER_COMP_ID_0;
	    timer1_cfg.comp0_val = comp; // 1000000 / 50 = 20000
//	    timer1_cfg.comp1_val = comp; // 1000000 / 50 = 20000
//	    timer1_cfg.comp2_val = comp; // 1000000 / 50 = 20000
	    timer1_cfg.preload_val = 0;    /* preload value */


	    // software reset timer

	    uint32_t tmpVal = getreg32(GLB_BASE+GLB_SWRST_CFG1_OFFSET);
		tmpVal |= GLB_SWRST_S1A5_MSK; // software reset timer
		putreg32(tmpVal, GLB_BASE + GLB_SWRST_CFG1_OFFSET);
		tmpVal &= ~GLB_SWRST_S1A5_MSK; // software reset timer
		putreg32(tmpVal, GLB_BASE + GLB_SWRST_CFG1_OFFSET);

	    // Timer init with default configuration
	    bflb_timer_init(timer1, &timer1_cfg);

	    bflb_irq_disable(timer1->irq_num);
	    return 0;
	} else
		return -1;
}
#endif

int ina229_init(void) {

	cfg_ina.inited = 0;
	cfg_ina.buf_enable = 0;
    ugpio = bflb_device_get_by_name("gpio");

    /* pin power */
    bflb_gpio_init(ugpio, GPIO_PWR, GPIO_INPUT | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_0);
	printf("INA229: Power On\r\n");
#ifdef GPIO_TST
    /* pin test */
    bflb_gpio_init(ugpio, GPIO_TST, GPIO_OUTPUT | GPIO_FLOAT | GPIO_SMT_EN | GPIO_DRV_0);
    bflb_gpio_set(ugpio, GPIO_TST);
#endif
#ifdef GPIO_SHT
    /* pin test */
    bflb_gpio_init(ugpio, GPIO_SHT, GPIO_OUTPUT | GPIO_FLOAT | GPIO_SMT_EN | GPIO_DRV_0);
    bflb_gpio_reset(ugpio, GPIO_SHT);
#endif
    bflb_mtimer_delay_ms(10);
    bflb_gpio_init(ugpio, GPIO_PWR, GPIO_OUTPUT | GPIO_FLOAT | GPIO_SMT_EN | GPIO_DRV_1);
    bflb_gpio_set(ugpio, GPIO_PWR);
    /* spi cs */
    bflb_gpio_init(ugpio, GPIO_NSS, GPIO_FUNC_SPI0 | GPIO_ALTERNATE | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_1);
    /* spi clk */
    bflb_gpio_init(ugpio, GPIO_SCLK, GPIO_FUNC_SPI0 | GPIO_ALTERNATE | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_1);
    /* spi miso */
    bflb_gpio_init(ugpio, GPIO_SDI, GPIO_FUNC_SPI0 | GPIO_ALTERNATE | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_1);
    /* spi mosi */
    bflb_gpio_init(ugpio, GPIO_SDO, GPIO_FUNC_SPI0 | GPIO_ALTERNATE | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_1);
    /* Alert */
    bflb_gpio_init(ugpio, GPIO_ALRT, GPIO_INPUT | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_0);

    bflb_mtimer_delay_ms(50);

#ifdef USE_TIMER_IRQ
    timer1 = bflb_device_get_by_name("timer1");
    // Timer init with default configuration
    if(timer1) {
        printf("INA229: timer init...\r\n");
    	timer_init(50);
	    bflb_irq_attach(timer1->irq_num, timer1_isr, NULL);
	} else {
		printf("INA229: Error SPI Init!\r\n");
		return -1;
	}
#endif
#ifdef USE_GPIO_IRQ
    /* pin irq/alert */
//    bflb_gpio_init(ugpio, GPIO_ALRT, GPIO_INPUT | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_0);
    bflb_gpio_int_init(ugpio, GPIO_ALRT, GPIO_INT_TRIG_MODE_ASYNC_FALLING_EDGE);
    bflb_gpio_int_mask(ugpio, GPIO_ALRT, false);

    bflb_irq_attach(ugpio->irq_num, gpio_isr, ugpio);
#endif

    /* spi0 init */
    struct bflb_spi_config_s spi_cfg = {
        .freq = 10 * 1000 * 1000,
        .role = SPI_ROLE_MASTER,
        .mode = SPI_MODE1,
        .data_width = SPI_DATA_WIDTH_24BIT, // SPI_DATA_WIDTH_32BIT,
        .bit_order = SPI_BIT_MSB,
        .byte_order = 1,// SPI_BYTE_LSB,
        .tx_fifo_threshold = 0,
        .rx_fifo_threshold = 0,
    };
    printf("INA229: spi init...\r\n");

    uspi0 = bflb_device_get_by_name("spi0");
	if(uspi0) {
		bflb_spi_init(uspi0, &spi_cfg);
	    if(bflb_spi_feature_control(uspi0, SPI_CMD_SET_CS_INTERVAL, 0) == 0) {
	        printf("INA229: spi init2...\r\n");
		    uint32_t mid = (bflb_spi_poll_send(uspi0, INARD16B(INA229_RA_MAN_ID)) & 0x00ffff00)>>8;
		    uint32_t did = (bflb_spi_poll_send(uspi0, INARD16B(INA229_RA_DEV_ID)) & 0x00ffff00)>>8;
		    printf("INA229: DevID - %04x, ManID - %04x\r\n", did, mid);
		    if (mid != 0x5449 || did != 0x2291) {
		    	  printf("INA229: Unknown ID, not init!\r\n");
		    	  return -1;
		    }
			printf("INA229: Init ok\r\n");
			cfg_ina.inited = 1;
			return 0;
	    }
	} else {
		printf("INA229: Error SPI Init!\r\n");
		return -1;
	}
    return -1;
}

const uint16_t tab_freg[8] = {
		20000, // 50us 1/0.000050=20000.0000Hz
		11905, // 84us 1/0.000084=11904.7619Hz
		6666,  // 150us 1/0.000150=6666.6667Hz
		3571,  // 280us 1/0.000280=3571.4286Hz
		1851,  // 540us 1/0.000540=1851.8519Hz
		950,   // 1052us 1/0.001052=950.5703Hz
		482,   // 2074us 1/0.002074=482.1601Hz
		242    // 4120us 1/0.004120=242.7184Hz
};

const uint16_t tab_time_us[8] = {
		50,
		84,
		150,
		280,
		540,
		1052,
		2074,
		4120
};

int ina229_start(dev_adc_cfg_t * cfg) {
	int i;
	if(!cfg_ina.inited) {
		printf("INA229: Not inint!\r\n");
		return -1;
	}
	cfg_ina.buf_enable = 0;
	blk_read_count = 0;
	blk_overflow_cnt = 0;
#ifdef USE_GPIO_IRQ
	bflb_irq_disable(ugpio->irq_num);
#endif
#ifdef USE_TIMER_IRQ
	bflb_timer_stop(timer1);
//	bflb_irq_disable(timer1->irq_num);
#endif
    bflb_spi_feature_control(uspi0, SPI_CMD_SET_DATA_WIDTH, SPI_DATA_WIDTH_24BIT);
	bflb_spi_poll_send(uspi0, INAWR16B(INA229_RA_CONFIG, 1<<15)); // System Reset sets registers to default values
    // TODO: sleep X us?
	bflb_mtimer_delay_us(300); /* delay for reset INA229 ? */

	if(cfg->sh) // Shunt 0 = 40.96 mV, 1 = 163.84 mV
		cfg_ina.r_shunt = 1<<4; // Shunt full scale range 40.96 mV
	else
		cfg_ina.r_shunt = 0; // Shunt full scale range 163.84 mV

	cfg_ina.r_alert = 1<<14; // Alert pin to be asserted when the Conversion Ready

	cfg_ina.r_config = 0;
	if(cfg->chnl == 3) {
		cfg_ina.r_config = 0x0b<<12; // Continuous Shunt + Vbus
		cfg_ina.chnls = 3;
	} else 	if(cfg->chnl == 2) {
		cfg_ina.r_config = 0x09<<12; // Continuous Vbus only
		cfg_ina.chnls = 2;
	} else {
		cfg_ina.r_config = 0x0a<<12; // Continuous Shunt only
		cfg_ina.chnls = 1;
	}
	// частота
	for(i = 0; i < 7; i++) {
		if(cfg->smpr >= tab_freg[i])
			break;
	}
	cfg_ina.freq = tab_freg[i];
	cfg_ina.r_config |= (i<<3)|(i<<6)|(i<<9);

	// задать конфигурацию INA229
	bflb_spi_poll_send(uspi0, INAWR16B(INA229_RA_CONFIG, cfg_ina.r_shunt)); // Shunt full scale range 40.96 mV
	bflb_spi_poll_send(uspi0, INAWR16B(INA229_RA_DIAG_ALRT, cfg_ina.r_alert)); // Alert pin to be asserted when the Conversion Ready
	bflb_spi_poll_send(uspi0, INAWR16B(INA229_RA_ADC_CONFIG, cfg_ina.r_config)); // Continuous shunt voltage only

	// фиксируем время старта в us
	uint32_t ts = getreg32(CLIC_CTRL_BASE + CLIC_MTIME_OFFSET); // (uint32_t)bflb_mtimer_get_time_us();

    bflb_spi_feature_control(uspi0, SPI_CMD_SET_DATA_WIDTH, SPI_DATA_WIDTH_32BIT);

    printf("INA229: Set %u sps (%u us), shunt: %08x, cfg: %08x, alr: %08x\r\n", cfg_ina.freq, tab_time_us[i], cfg_ina.r_shunt, cfg_ina.r_config, cfg_ina.r_alert);

#ifdef USE_TIMER_IRQ
    timer_init(tab_time_us[i]);

    /* delay for Conversion Ready INA229 */
    // Sample time:    50,  84, 150, 280,  540, 1052  2074  4120 us
    // RDY 2 channel: 188, 248, 379, 640, 1160, 1400, 2431, 8326 us
    uint32_t tx;
    uint32_t te = tab_time_us[i] + 512;
    if(cfg_ina.chnls == 3)
    	te += tab_time_us[i];
    do {
    	// прошло более RDY us ?
    	tx = getreg32(CLIC_CTRL_BASE + CLIC_MTIME_OFFSET) - ts;
    	if(tx > te) { // (uint32_t)bflb_mtimer_get_time_us()
    	    printf("INA229: ALERT pin is not connected!\r\n");
       		break;
    	}
    } while(bflb_gpio_read(ugpio, GPIO_ALRT) != 0);
    printf("INA229: Delay for Conversion Ready %u us, max %u us\r\n", tx, te);
#endif
    // предварительное - стартовое  чтение замерений из INA229.
	uint32_t reg_base = uspi0->reg_base;
    if(cfg_ina.chnls == 3) {
//    		bflb_mtimer_delay_us(tab_time_us[i]); /* delay for start INA229 ? */
//    	    putreg32(INARD24B(INA229_RA_VSHUNT), reg_base + SPI_FIFO_WDATA_OFFSET); // read VSHUNT
    	    putreg32(INARD24B(INA229_RA_VBUS), reg_base + SPI_FIFO_WDATA_OFFSET); // read VBUS
    } else if(cfg_ina.chnls == 2) {
	    putreg32(INARD24B(INA229_RA_VBUS), reg_base + SPI_FIFO_WDATA_OFFSET); // read VBUS
    } else
	    putreg32(INARD24B(INA229_RA_VSHUNT), reg_base + SPI_FIFO_WDATA_OFFSET); // read VSHUNT

	// включить буферизацию
    data_buffer_wr_ptr = 0;
	data_buffer_rd_ptr = 0;

	cfg_ina.buf_enable = 1;

	// включить прерывания
#ifdef USE_GPIO_IRQ
    bflb_gpio_int_clear(ugpio, GPIO_ALRT);
    bflb_irq_enable(ugpio->irq_num);
#endif
#ifdef USE_TIMER_IRQ
    bflb_irq_enable(timer1->irq_num);
    bflb_timer_start(timer1); // Enable timer
#endif
    printf("INA229: Start sampling...\r\n");
    return 0;
}

int ina229_stop(void) {
	if(!cfg_ina.inited) {
		printf("INA229: Not inint!\r\n");
		return -1;
	}
	cfg_ina.buf_enable = 0;
#ifdef USE_GPIO_IRQ
	bflb_irq_disable(ugpio->irq_num);
#endif
#ifdef USE_TIMER_IRQ
	bflb_timer_stop(timer1);
	bflb_irq_disable(timer1->irq_num);
#endif
    bflb_spi_feature_control(uspi0, SPI_CMD_SET_DATA_WIDTH, SPI_DATA_WIDTH_24BIT);
//    bflb_spi_poll_send(uspi0, INARD16B(INA229_RA_MAN_ID));
//    bflb_spi_poll_send(uspi0, INARD16B(INA229_RA_DEV_ID));
	bflb_spi_poll_send(uspi0, INAWR16B(INA229_RA_ADC_CONFIG, 0)); // Shutdown
//  printf("INA229: Shutdownt at: %u\r\n", (uint32_t)bflb_mtimer_get_time_ms());
	printf("INA229: Shutdownt\r\n");
	return 0;
}

void ina229_sleep(void) {
	if(cfg_ina.inited) {
		ina229_stop();
	}
    bflb_timer_deinit(timer1);
    bflb_spi_deinit(uspi0);
#ifdef GPIO_SHT
    bflb_gpio_reset(ugpio, GPIO_SHT);
#endif
    bflb_gpio_init(ugpio, GPIO_ALRT, GPIO_INPUT | GPIO_PULLDOWN | GPIO_SMT_EN | GPIO_DRV_0);
    bflb_gpio_init(ugpio, GPIO_SCLK, GPIO_INPUT | GPIO_PULLDOWN | GPIO_SMT_EN | GPIO_DRV_0);
    bflb_gpio_init(ugpio, GPIO_SDO, GPIO_INPUT | GPIO_PULLDOWN | GPIO_SMT_EN | GPIO_DRV_0);
    bflb_gpio_init(ugpio, GPIO_SDI, GPIO_INPUT | GPIO_PULLDOWN | GPIO_SMT_EN | GPIO_DRV_0);
    bflb_gpio_init(ugpio, GPIO_PWR, GPIO_INPUT | GPIO_PULLDOWN | GPIO_SMT_EN | GPIO_DRV_0);
    bflb_gpio_init(ugpio, GPIO_NSS, GPIO_INPUT | GPIO_FLOAT | GPIO_SMT_EN | GPIO_DRV_0);
	printf("INA229: Power Off\r\n");
    bflb_mtimer_delay_ms(5);
    bflb_gpio_reset(ugpio, GPIO_PWR);
    bflb_gpio_init(ugpio, GPIO_PWR, GPIO_OUTPUT | GPIO_FLOAT | GPIO_SMT_EN | GPIO_DRV_1);
    bflb_gpio_reset(ugpio, GPIO_PWR);
    cfg_ina.inited = 0;
}

void ina229_wakeup(void) {
	if(!cfg_ina.inited) {
		if(ina229_init()){
			ina229_sleep();
		}
	}
}

