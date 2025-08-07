/*
 * cmd_decode.c
 *
 *  Created on: 14.01.2020
 *      Author: pvvx
 */
#include "common.h"
#include "cmd_cfg.h"
#include "i2c_dev.h"

#define INT_DEV_ID  0x1024  // DevID = 0x1024
#define INT_DEV_VER 0x0007  // Ver 1.2.3.4 = 0x1234

//blk_rx_pkt_t read_pkt; // приемный буфер
blk_tx_pkt_t send_pkt; // буфер отправки

/*******************************************************************************
 *******************************************************************************/
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

/* delay for Conversion Ready INA228 */
// Sample time:    50,  84, 150, 280,  540, 1052  2074  4120 us
// RDY 2 channel: 188, 248, 379, 640, 1160, 1400, 2431, 8326 us
const uint16_t tab_time_us[8] = {
    100, // 50 us not work!
    84,
    150,
    280,
    540,
    1052,
    2074,
    4120
};

dev_ina228_cfg_t cfg_ina228;

int ina228_start(dev_ina228_cfg_t * cfg) {
  int i;
  uint16_t reg;

  i2c_dev.timer_flg = 0;

  // задать конфигурацию INA228
  cfg_i2c.init[0].dev_addr = I2C_ADDR_INA228;
  cfg_i2c.init[0].reg_addr = INA228_RA_CONFIG;
  cfg_i2c.init[0].data = 3<<14;  // System Reset sets registers to default values

  if(cfg->sh) // Shunt 0 = 40.96 mV, 1 = 163.84 mV
    reg = 1<<4; // Shunt full scale range 40.96 mV
  else
    reg = 0; // Shunt full scale range 163.84 mV

  cfg_i2c.init[1].dev_addr = I2C_ADDR_INA228;
  cfg_i2c.init[1].reg_addr = INA228_RA_CONFIG;
  cfg_i2c.init[1].data = reg;  // Shunt full scale range +-40.96/163.84 mV

  cfg_i2c.init[2].dev_addr = I2C_ADDR_INA228;
  cfg_i2c.init[2].reg_addr = INA228_RA_DIAG_ALRT;
  cfg_i2c.init[2].data = 1<<14;  // Alert pin to be asserted when the Conversion Ready

  cfg_i2c.rd[0].dev_addr = I2C_ADDR_INA228;
  cfg_i2c.rd[0].reg_addr = INA228_RA_VBUS;
  cfg_i2c.rd[1].dev_addr = I2C_ADDR_INA228;
  cfg_i2c.rd[1].reg_addr = INA228_RA_VSHUNT;
  cfg_i2c.rd[2].dev_addr = 0;

  // INA228_RA_ADC_CONFIG
  reg = 0;
  if(cfg->chnl == 3) {
    reg = 0x0b<<12; // Continuous Shunt + Vbus
    i2c_dev.i2c_rd_24bit = 2;
  } else  if(cfg->chnl == 2) {
    cfg_i2c.rd[1].reg_addr = INA228_RA_VBUS;
    reg = 0x09<<12; // Continuous Vbus only
    i2c_dev.i2c_rd_24bit = 1;
  } else {
    cfg_i2c.rd[0].reg_addr = INA228_RA_VSHUNT;
    reg = 0x0a<<12; // Continuous Shunt only
    i2c_dev.i2c_rd_24bit = 1;
  }
  // частота
  for(i = 0; i < 7; i++) {
    if(cfg->smpr >= tab_freg[i])
      break;
  }
  reg |= (i<<3)|(i<<6)|(i<<9);

  cfg_i2c.init[3].dev_addr = I2C_ADDR_INA228;
  cfg_i2c.init[3].reg_addr = INA228_RA_ADC_CONFIG;
  cfg_i2c.init[3].data = reg;  // Continuous shunt voltage only


  cfg_i2c.clk_khz = DEF_I2C_CLK_KHZ; // | 0x8000;
  cfg_i2c.multiplier = 0;
  cfg_i2c.time = tab_time_us[i]; // us

  cfg_i2c.pktcnt = 20;  // 20*3+2 = 62 bytes (+2 head)

  return I2CDevStart();
}

int ina228_stop(void) {
  cfg_i2c.pktcnt = 0;
  //cfg_i2c.clk_khz = 400 | 0x8000;
  return I2CDevStart();
}

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
        //PRINT("CMD_DEV_CFG\n");
				if (pbufi->head.size) {
				  i2c_dev.timer_flg = 0;
#if 0 // (USE_ADC_DEV)
					if(!pbufi->data.uc[0]) {
						ADC_Stop();
					}
#endif // USE_ADC_DEV
					memcpy(&cfg_i2c, &pbufi->data.ci2c,
							(pbufi->head.size > sizeof(cfg_i2c)-2)? sizeof(cfg_i2c)-2 : pbufi->head.size);
					if (!I2CDevStart()) {
						pbufo->head.cmd |= CMD_ERR_FLG; // Error cmd
						txlen = 0 + sizeof(blk_head_t);
						break;
					}
				}
				memcpy(&pbufo->data, &cfg_i2c, sizeof(cfg_i2c)-2);
				txlen = sizeof(cfg_i2c)-2 + sizeof(blk_head_t);
				break;
			case CMD_DEV_SCF: // Store CFG/ini in Flash
				if(pbufi->head.size < sizeof(dev_scf_t)) {
					pbufo->head.cmd |= CMD_ERR_FLG; // Error cmd
					txlen = 0 + sizeof(blk_head_t);
					break;
				}
				pbufo->data.ud[0] = 0;
#if	(USE_I2C_DEV)
				if(pbufi->data.scf.i2c) {
					//pbufo->data.scf.i2c = flash_write_cfg(&cfg_i2c, EEP_ID_I2C_CFG, sizeof(cfg_i2c));
				  if(I2CDevWriteSetings() == 0)
				    pbufo->data.scf.i2c = 1;
				}
#endif
#if	0 // (USE_ADC_DEV)
				if(pbufi->data.scf.adc)
					pbufo->data.scf.adc = flash_write_cfg(&cfg_adc, EEP_ID_ADC_CFG, sizeof(cfg_adc));
#endif
				txlen = sizeof(dev_scf_t) + sizeof(blk_head_t);
				break;
#if USE_I2C_24BIT
      case CMD_DEV_ECAD: // Get/Set CFG/ini ADC & Start measure
        if (pbufi->head.size) {
          memcpy(&cfg_ina228, &pbufi->data.ina228,
            (pbufi->head.size > sizeof(cfg_ina228))? sizeof(cfg_ina228) : pbufi->head.size);
          if(pbufi->data.ina228.enable) {
            ina228_start(&cfg_ina228);
            //cfg_adc.chnl = cfg_ina.chnls;
            //cfg_adc.smpr = cfg_ina.freq;
          } else
            ina228_stop();
        }
        memcpy(&pbufo->data, &cfg_ina228, sizeof(cfg_ina228));
        txlen = sizeof(cfg_ina228) + sizeof(blk_head_t);
        break;
#endif
			//-------
			case CMD_DEV_GRG: // Get reg
				tmp = i2c_dev.timer_flg;
				i2c_dev.timer_flg = 0;
				//I2CBusTrEnd();
				if (!I2CBusReadWord(pbufi->data.reg.dev_addr, pbufi->data.reg.reg_addr,
					(uint16_t *)&pbufo->data.reg.data)) {
					pbufo->data.ui[0] = pbufi->data.ui[0];
					txlen = sizeof(reg_wr_t) + sizeof(blk_head_t);
				} else {
					pbufo->head.cmd |= CMD_ERR_FLG; // Error cmd
					txlen = 0 + sizeof(blk_head_t);
				}
				i2c_dev.timer_flg = tmp;
				break;
			case CMD_DEV_SRG: // Set reg
				tmp = i2c_dev.timer_flg;
				i2c_dev.timer_flg = 0;
				//I2CBusTrEnd();
				if (!I2CBusWriteWord(pbufi->data.reg.dev_addr, pbufi->data.reg.reg_addr,
						pbufi->data.reg.data)) {
					pbufo->data.reg = pbufi->data.reg;
					txlen = sizeof(reg_wr_t) + sizeof(blk_head_t);
				} else {
					pbufo->head.cmd |= CMD_ERR_FLG; // Error cmd
					txlen = 0 + sizeof(blk_head_t);
				};
				i2c_dev.timer_flg = tmp;
				break;
#endif // USE_I2C_DEV
#if 0 // (USE_ADC_DEV)
			case CMD_DEV_CAD: // Get/Set CFG/ini ADC & Start measure
				if (pbufi->head.size) {
					memcpy(&cfg_adc, &pbufi->data.cadc,
						(pbufi->head.size > sizeof(cfg_adc))? sizeof(cfg_adc) : pbufi->head.size);
					if(pbufi->data.cadc.pktcnt) {
					  status.all_read_count = 0;
						status.all_overflow_cnt = 0;
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
			  // PRINT("CMD_DEV_STA\n");
        pbufo->data.sta.rd_cnt = status.all_read_count;
        pbufo->data.sta.to_cnt = status.all_overflow_cnt;
				txlen = sizeof(blk_head_t) + sizeof(dev_sta_t);
				break;

			case CMD_I2C_PWR:
			  if(pbufi->head.size) {
			    if(pbufi->data.uc[0])
			      I2CDevWakeUp();
			    else {
#ifdef I2C_DEV_POWER
			      if((R32_PB_OUT &= I2C_DEV_POWER) == 0)
#endif
			        I2CDevWakeUp();
			      I2CDevSleep();
			    }
			  }
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
				// test_function();
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
