/*
 * cmd_decode.c
 *
 *  Created on: 14.01.2020
 *      Author: pvvx
 */
#include "main_config.h"
#include "bl702_common.h"
#include "bflb_adc.h"
#include "ring_buffer.h"
#include "adc_dma.h"
#include "cmd_cfg.h"
#include "i2cbus.h"
#include "i2c_dev.h"

#define INT_DEV_ID	0x1022  // DevID = 0x1022
#define INT_DEV_VER 0x0001  // Ver 1.2.3.4 = 0x1234

// status
volatile uint32_t all_read_count;
volatile uint32_t all_overflow_cnt; // overflow


//blk_rx_pkt_t read_pkt; // приемный буфер
blk_tx_pkt_t send_pkt; // буфер отправки

//--------- < Test!
static inline void test_function(void) {
};
//--------- Test! >

/*******************************************************************************
 * Function Name : usb_ble_cmd_decode.
 * Description	 : Main loop routine.
 * Input		 : blk_tx_pkt_t * pbufo, blk_tx_pkt_t * pbufi, int rxlen
 * Return		 : txlen.
 *******************************************************************************/
unsigned int cmd_decode(blk_tx_pkt_t * pbufo, blk_rx_pkt_t * pbufi, unsigned int rxlen) {
	unsigned int txlen = 0;
	uint32_t tmp;
//	if (rxlen >= sizeof(blk_head_t)) {
//		if (rxlen >= pbufi->head.size + sizeof(blk_head_t)) {
			pbufo->head.cmd = pbufi->head.cmd;
			switch (pbufi->head.cmd) {
			case CMD_DEV_VER: // Get Ver
				pbufo->data.ui[0] = INT_DEV_ID; // DevID = 0x1021
				pbufo->data.ui[1] = INT_DEV_VER; // Ver 1.2.3.4 = 0x1234
				txlen = sizeof(uint16_t) + sizeof(uint16_t) + sizeof(blk_head_t);
				break;
#if (USE_I2C_DEV)
			case CMD_DEV_CFG: // Get/Set CFG/ini & Start measure
				if (pbufi->head.size) {
					timer_flg = 0;
#if 0 // (USE_ADC_DEV)
					if(!pbufi->data.uc[0]) {
						ADC_Stop();
					}
#endif // USE_ADC_DEV
					memcpy(&cfg_i2c, &pbufi->data.ci2c,
							(pbufi->head.size > sizeof(cfg_i2c))? sizeof(cfg_i2c) : pbufi->head.size);
					if (!InitI2CDevice()) {
						pbufo->head.cmd |= CMD_ERR_FLG; // Error cmd
						txlen = 0 + sizeof(blk_head_t);
						break;
					}
				}
				memcpy(&pbufo->data, &cfg_i2c, sizeof(cfg_i2c));
				txlen = sizeof(cfg_i2c) + sizeof(blk_head_t);
				break;
			case CMD_DEV_SCF: // Store CFG/ini in Flash
				if(pbufi->head.size < sizeof(dev_scf_t)) {
					pbufo->head.cmd |= CMD_ERR_FLG; // Error cmd
					txlen = 0 + sizeof(blk_head_t);
					break;
				}
				pbufo->data.ud[0] = 0;
#if 0
#if	(USE_I2C_DEV)
				if(pbufi->data.scf.i2c)
					pbufo->data.scf.i2c = flash_write_cfg(&cfg_i2c, EEP_ID_I2C_CFG, sizeof(cfg_i2c));
#endif
#if	(USE_ADC_DEV)
				if(pbufi->data.scf.adc)
					pbufo->data.scf.adc = flash_write_cfg(&cfg_adc, EEP_ID_ADC_CFG, sizeof(cfg_adc));
#endif
#endif
				txlen = sizeof(dev_scf_t) + sizeof(blk_head_t);
				break;
			//-------
			case CMD_DEV_GRG: // Get reg
				tmp = timer_flg;
				timer_flg = 0;
				I2CBusTrEnd();
				if (I2CBusReadWord(pbufi->data.reg.dev_addr, pbufi->data.reg.reg_addr,
					(uint16_t *)&pbufo->data.reg.data)) {
					pbufo->data.ui[0] = pbufi->data.ui[0];
					txlen = sizeof(reg_wr_t) + sizeof(blk_head_t);
				} else {
					pbufo->head.cmd |= CMD_ERR_FLG; // Error cmd
					txlen = 0 + sizeof(blk_head_t);
				}
				timer_flg = tmp;
				break;
			case CMD_DEV_SRG: // Set reg
				tmp = timer_flg;
				timer_flg = 0;
				I2CBusTrEnd();
				if (I2CBusWriteWord(pbufi->data.reg.dev_addr, pbufi->data.reg.reg_addr,
						pbufi->data.reg.data)) {
					pbufo->data.reg = pbufi->data.reg;
					txlen = sizeof(reg_wr_t) + sizeof(blk_head_t);
				} else {
					pbufo->head.cmd |= CMD_ERR_FLG; // Error cmd
					txlen = 0 + sizeof(blk_head_t);
				};
				timer_flg = tmp;
				break;
#endif // USE_I2C_DEV
#if (USE_ADC_DEV)
			case CMD_DEV_CAD: // Get/Set CFG/ini ADC & Start measure
				if (pbufi->head.size) {
					memcpy(&cfg_adc, &pbufi->data.cadc,
						(pbufi->head.size > sizeof(cfg_adc))? sizeof(cfg_adc) : pbufi->head.size);
					if(pbufi->data.cadc.pktcnt) {
						all_read_count = 0;
						all_overflow_cnt = 0;
						ADC_Start(&cfg_adc);
					} else
						ADC_Stop();
#if 0
					if (!InitADCDevice()) {
						pbufo->head.cmd |= CMD_ERR_FLG; // Error cmd
						txlen = 0 + sizeof(blk_head_t);
						break;
					}
#endif
				}
				memcpy(&pbufo->data, &cfg_adc, sizeof(cfg_adc));
				txlen = sizeof(cfg_adc) + sizeof(blk_head_t);
				break;
#endif // USE_ADC_DEV
			//-------
			case CMD_DEV_STA: // Status
				pbufo->data.sta.rd_cnt = all_read_count;
				pbufo->data.sta.to_cnt = all_overflow_cnt;
				txlen = sizeof(blk_head_t) + sizeof(dev_sta_t);
				break;
			//-------

#if (USE_DAC_DEV)
			case CMD_DEV_DAC: // Dac cfg
				txlen = pbufi->head.size;
				if (txlen) {
					if(txlen > sizeof(cfg_dac))
						txlen = sizeof(cfg_dac);
					memcpy(&cfg_dac, &pbufi->data.dac, txlen);
					dac_cmd(&cfg_dac);
				}
//				pbufo->data.uc[0] = dac_cmd(&pbufi->data.dac);
				memcpy(&pbufo->data.dac, &cfg_dac, sizeof(cfg_dac));
				txlen = sizeof(cfg_dac) + sizeof(blk_head_t);
				break;
#endif
#if 0
			case CMD_DEV_DBG: // Debug
				if(pbufi->head.size > sizeof(dev_dbg_t)) {
					memcpy((u8 *)0x800000 + pbufi->data.dbg.addr, &pbufi->data.ud[1], pbufi->head.size - sizeof(dev_dbg_t));
				} else if(pbufi->head.size < sizeof(dev_dbg_t)) {
					pbufo->head.cmd |= CMD_ERR_FLG; // Error cmd
					txlen = 0 + sizeof(blk_head_t);
					break;
				}
				txlen = pbufi->data.dbg.rd_cnt;
				if(txlen){
					if(txlen > sizeof(pbufi->data))
						txlen = sizeof(pbufi->data);
					memcpy(&pbufo->data, (u8 *)0x800000 + pbufi->data.dbg.addr, txlen);
				}
				txlen += sizeof(blk_head_t);
				break;
#endif
			case CMD_DEV_TST: // 	blk out X data, cfg TST device
				test_function();
				break;
			default:
				pbufo->head.cmd |= CMD_ERR_FLG; // Error cmd
				txlen = 0 + sizeof(blk_head_t);
				break;
			};
			pbufo->head.size = txlen - sizeof(blk_head_t);
//		}
//		else
//			rxlen = 0;
//	}
	return txlen;
}
