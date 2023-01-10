#include "main_config.h"
#if (USE_ADC_DEV)
#include "bflb_mtimer.h"
#include "bflb_dma.h"
#include "bflb_dac.h"
#include "cmd_cfg.h"
#include "adc_dma.h"
#include "hardware/adc_reg.h"
#include "hardware/dma_reg.h"
#include "board.h"
#include "usb_buffer.h"

#define ADC_SPS_AUTO
//#define ADC_RESOLUTION_12B 0
#define ADC_RESOLUTION_13B 1
//#define ADC_RESOLUTION_14B 2
#define ADC_RESOLUTION_15B 3
//#define ADC_RESOLUTION_16B 4

#define ADC_printf(...) printf(__VA_ARGS__)

struct bflb_device_s *adc;
struct bflb_device_s *dma0_ch0;

struct {
	uint16_t id;
	uint16_t data[ADC_DATA_BLK_CNT];
} adc_blk;

struct bflb_adc_channel_s adc_chan[1];
struct bflb_dma_channel_lli_pool_s adc_lli[1]; /* max trasnfer size 4064 * 1 */


ATTR_NOCACHE_NOINIT_RAM_SECTION uint32_t adc_raw_data[ADC_DATA_BLK_CNT];

dev_adc_cfg_t cfg_adc = {
		.pktcnt = 0,
		.chnl = ADC_CHANNEL_1,
		.sps = 3906,
		.pga20db = 0,
		.pga2db5 = 0
};

static void dma0_ch0_isr(void *arg)
{
#if TESTS_DAC_STEP_ADC   // test DAC
#if DAC_DMA_ENABLE
#error "Set DAC_DMA_ENABLE = 0!"
#endif
    extern struct bflb_device_s *dac;
    bflb_dac_set_value(dac, DAC_CHANNEL_B, adc_read_cnt);
#endif
   	//bflb_adc_channel_config(adc, &chan[adc_read_cnt&1], 1);
    for (uint8_t i = 0; i < ADC_DATA_BLK_CNT; i++) {
    	adc_blk.data[i] = (uint16_t)adc_raw_data[i];
    }
//	if(Ring_Buffer_Get_Empty_Length(&usb_tx_rb) >= sizeof(adc_blk)) {
	adc_blk.id = 0x0A00 + sizeof(adc_blk.data);
	if(sizeof(adc_blk) == Ring_Buffer_Write(&usb_tx_rb, (uint8_t *)&adc_blk, sizeof(adc_blk))) {
		all_read_count++;
	} else {
		ADC_printf("adc buffer overflow!\r\n");
		all_overflow_cnt++;
	}
}


void bflb_adc_set_pga(struct bflb_device_s *dev, int pga1_gain, int pga2_gain)
{
    uint32_t regval;
    uint32_t reg_base = dev->reg_base;

    regval = getreg32(reg_base + AON_GPADC_REG_CONFIG2_OFFSET);
    regval &= ~AON_GPADC_PGA1_GAIN_MASK;
    regval &= ~AON_GPADC_PGA2_GAIN_MASK;
    regval |= pga1_gain << AON_GPADC_PGA1_GAIN_SHIFT;
    regval |= pga2_gain << AON_GPADC_PGA2_GAIN_SHIFT;

    if(pga1_gain == 0 && pga2_gain == 0) {
    	regval &= ~AON_GPADC_PGA_EN;
    } else {
    	regval |= AON_GPADC_PGA_EN;
    }
    putreg32(regval, reg_base + AON_GPADC_REG_CONFIG2_OFFSET);
}

//void adc_set_pga(int pga1_gain, int pga2_gain) {
//	bflb_adc_set_pga(adc, pga1_gain, pga2_gain);
//}

#ifdef ADC_SPS_AUTO
static unsigned int _csmpdiv(unsigned int dsps) {
	if(dsps >= 24 + (32-24)/2)
		return ADC_CLK_DIV_32;
	if(dsps >= 20 + (24-20)/2)
		return ADC_CLK_DIV_24;
	if(dsps >= 16 + (20-16)/2)
		return ADC_CLK_DIV_20;
	if(dsps >= 12 + (16-12)/2)
		return ADC_CLK_DIV_16;
	return ADC_CLK_DIV_12;
}
#endif


/* sps:
 * 3906,  16 bits
 * 5208,  16 bits
 * 6250,  16 bits
 * 7812,  16 bits
 * 10416, 16 bits
 * 12500, 15 bits
 * 15625, 15 bits
 * 20833, 15 bits
 * 25000, 14 bits
 * 31250, 14 bits
 * 41666, 14 bits
 * 62500,  13 bits
 * 83333,  13 bits
 * 100000, 13 bits
 * 125000, 13 bits
 * 166666, 13 bits
 * 1000000, 12 bits
 * 1333333, 12 bits
 * 1600000, 12 bits
 * 2000000, 12 bits
 * 2666666, 12 bits
 */
void adc_dma_init(dev_adc_cfg_t *cadc)
{
	uint32_t sps = cadc->sps;
    board_adc_gpio_init();
    //all_read_count = 0;
    //all_overflow_cnt = 0;

    adc = bflb_device_get_by_name("adc");

    struct bflb_adc_config_s cfg;
#ifdef ADC_SPS_AUTO
    if(sps < 32000000/256/32) { // = 3.906 ksps
    	cfg.resolution = ADC_RESOLUTION_16B;
    	cfg.clk_div = ADC_CLK_DIV_32;
    } else if(sps <= 32000000/256/12) {
    	cfg.resolution = ADC_RESOLUTION_16B;
    	cfg.clk_div = _csmpdiv((32000000/256)/sps);
    } else if(sps <= 32000000/128/12) {
    	cfg.resolution = ADC_RESOLUTION_15B;
    	cfg.clk_div = _csmpdiv((32000000/128)/sps);
    } else if(sps <= 32000000/64/12) {
    	cfg.resolution = ADC_RESOLUTION_14B;
    	cfg.clk_div = _csmpdiv((32000000/64)/sps);
    } else if(sps <= 32000000/16/12) {
    	cfg.resolution = ADC_RESOLUTION_13B;
    	cfg.clk_div = _csmpdiv((32000000/16)/sps);
    } else if(sps <= 32000000/12) {
    	cfg.resolution = ADC_RESOLUTION_12B;
    	cfg.clk_div = _csmpdiv(32000000/sps);
    } else {	// default 20 ksps
        cfg.resolution = ADC_RESOLUTION_15B;
        cfg.clk_div = ADC_CLK_DIV_12;
    }
#else
	// ADC_CLK_DIV_32, ADC_RESOLUTION_16B = 3906.25 sps  	# 32000000/256/32 = 3906.25
	// ADC_CLK_DIV_24, ADC_RESOLUTION_16B = 5208.33 sps  	# 32000000/256/24 = 5208.33
	// ADC_CLK_DIV_20, ADC_RESOLUTION_16B = 6250 sps     	# 32000000/256/20 = 6250
	// ADC_CLK_DIV_16, ADC_RESOLUTION_16B = 7812.5 sps   	# 32000000/256/16 = 7812.5
	// ADC_CLK_DIV_12, ADC_RESOLUTION_16B = 10416.66 sps 	# 32000000/256/12 = 10416.66

	// ADC_CLK_DIV_32, ADC_RESOLUTION_15B = 7812.5 sps		# 32000000/128/32 = 7812.5
	// ADC_CLK_DIV_24, ADC_RESOLUTION_15B = 10416.66 sps	# 32000000/128/24 = 10416.66
	// ADC_CLK_DIV_20, ADC_RESOLUTION_15B = 12500 sps		# 32000000/128/20 = 12500
	// ADC_CLK_DIV_16, ADC_RESOLUTION_15B = 15625 sps		# 32000000/128/16 = 15625
	// ADC_CLK_DIV_12, ADC_RESOLUTION_15B = 20833.33 sps	# 32000000/128/12 = 20833.33

	// ADC_CLK_DIV_32, ADC_RESOLUTION_14B = 15625 sps		# 32000000/64/32 = 15625
	// ADC_CLK_DIV_24, ADC_RESOLUTION_14B = 20833.33 sps	# 32000000/64/24 = 20833.33
	// ADC_CLK_DIV_20, ADC_RESOLUTION_14B = 25000 sps		# 32000000/64/20 = 25000
	// ADC_CLK_DIV_16, ADC_RESOLUTION_14B = 31250 sps		# 32000000/64/16 = 31250
	// ADC_CLK_DIV_12, ADC_RESOLUTION_14B = 41666.66 sps	# 32000000/64/12 = 41666.66

	// ADC_CLK_DIV_32, ADC_RESOLUTION_13B = 62500 sps		# 32000000/16/32 = 62500
	// ADC_CLK_DIV_24, ADC_RESOLUTION_13B = 83333.33 sps	# 32000000/16/24 = 83333.33
	// ADC_CLK_DIV_20, ADC_RESOLUTION_13B = 100000 sps		# 32000000/16/20 = 100000
	// ADC_CLK_DIV_16, ADC_RESOLUTION_13B = 125000 sps		# 32000000/16/16 = 125000
	// ADC_CLK_DIV_12, ADC_RESOLUTION_13B = 166666.66 sps	# 32000000/16/12 = 166666.66

	// ADC_CLK_DIV_32, ADC_RESOLUTION_12B = 1000000 sps		# 32000000/32 = 1000000
	// ADC_CLK_DIV_24, ADC_RESOLUTION_12B = 1333333.33 sps	# 32000000/24 = 1333333.33
	// ADC_CLK_DIV_20, ADC_RESOLUTION_12B = 1600000 sps		# 32000000/20 = 1600000
	// ADC_CLK_DIV_16, ADC_RESOLUTION_12B = 2000000 sps		# 32000000/16 = 2000000
	// ADC_CLK_DIV_12, ADC_RESOLUTION_12B = 2666666.66 sps	# 32000000/12 = 2666666.66

    // set 20 ksps, 15 bits
    cfg.clk_div = ADC_CLK_DIV_12;
    cfg.resolution = ADC_RESOLUTION_15B;
#endif
    cfg.scan_conv_mode = false; //true if use N chls
    cfg.continuous_conv_mode = true;
    cfg.differential_mode = false;

    cfg.vref = ADC_VREF_3P2V; // ADC_VREF_3P2V; // ADC_VREF_2P0V or ADC_VREF_3P2V

    bflb_adc_init(adc, &cfg);


	adc_chan[0].pos_chan = cadc->chnl;
	adc_chan[0].neg_chan = ADC_CHANNEL_GND;

    if(bflb_adc_channel_config(adc, &adc_chan[0], 1) != 0) {
    	printf("ADC: Error channel config!\r\n");
    }

	bflb_adc_set_pga(adc, cadc->pga2db5 & 0x0f, cadc->pga2db5 >> 4);

    bflb_adc_link_rxdma(adc, true);

    struct bflb_dma_channel_config_s config;

    dma0_ch0 = bflb_device_get_by_name("dma0_ch0");

    config.direction = DMA_PERIPH_TO_MEMORY;
    config.src_req = DMA_REQUEST_ADC;
    config.dst_req = DMA_REQUEST_NONE;
    config.src_addr_inc = DMA_ADDR_INCREMENT_DISABLE;
    config.dst_addr_inc = DMA_ADDR_INCREMENT_ENABLE;
    config.src_burst_count = DMA_BURST_INCR1;
    config.dst_burst_count = DMA_BURST_INCR1;
    config.src_width = DMA_DATA_WIDTH_32BIT;
    config.dst_width = DMA_DATA_WIDTH_32BIT;
    bflb_dma_channel_init(dma0_ch0, &config);

    bflb_dma_channel_irq_attach(dma0_ch0, dma0_ch0_isr, NULL);

    struct bflb_dma_channel_lli_transfer_s transfers[1];

    memset(adc_raw_data, 0, sizeof(adc_raw_data));

    transfers[0].src_addr = (uint32_t)DMA_ADDR_ADC_RDR;
    transfers[0].dst_addr = (uint32_t)adc_raw_data;
    transfers[0].nbytes = sizeof(adc_raw_data);

    // set DMA cycles mode
    int used_dma_count = bflb_dma_channel_lli_reload(dma0_ch0, adc_lli, 1, transfers, 1);
    if(used_dma_count >= 0) {
    	//    adc_lli[0].nextlli = (uint32_t)&lli[0];
    	//    putreg32(adc_lli[0].nextlli, dma0_ch0->reg_base + DMA_CxLLI_OFFSET);
    	bflb_dma_channel_lli_link_head(dma0_ch0, adc_lli, used_dma_count);
    } else {
    	printf("ADC: Error dma_channel_reload!\r\n");
    }


    if(bflb_dma_channel_start(dma0_ch0) != 0) {
    	printf("ADC: Error start dma!\r\n");
    }
}

void ADC_Start(dev_adc_cfg_t * cadc) {

	if(adc) {
		bflb_dma_channel_stop(dma0_ch0);
		bflb_adc_stop_conversion(adc);
	}
	ADC_printf("ADC: enable\r\n");
	if(cadc->pktcnt) {
		adc_dma_init(cadc);
		bflb_adc_start_conversion(adc);
	}
}

void ADC_Stop(void) {
	if(adc) {
		bflb_adc_stop_conversion(adc);
		bflb_dma_channel_stop(dma0_ch0);
		bflb_adc_deinit(adc);
		ADC_printf("ADC: disable\r\n");
		adc = NULL;
	}
}

#endif
