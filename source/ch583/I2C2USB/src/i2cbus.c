/*
 * i2cbus.c
 *  Author: pvvx
 */
#include "common.h"
#if (USE_I2C_DEV)
#include "i2c_dev.h"

uint32_t i2c_wait_status(uint32_t mask) {
  uint32_t ret = -1; // timeout
  uint32_t tt = GetSysTickCnt(); // (uint32_t)SysTick->CNT;
  while(1) {
    ret = R16_I2C_STAR1;
    if(ret & mask) {
      (void)R16_I2C_STAR2; // read R16_I2C_STAR2 -> clear flags STOPF, ADD10, BTF, ADDR, SB
      //i2c_printf("I2C: Status wr %08x\n", ret);
      ret &= (RB_I2C_AF | RB_I2C_BERR | RB_I2C_ARLO | RB_I2C_OVR | RB_I2C_PECERR | RB_I2C_TIMEOUT | RB_I2C_SMBALERT);
      if(ret) {
        I2CBusInit();
//        i2c_printf("I2C: Error %04x\n", ret);
      }
      break;
    }
    if(GetSysTickCnt() - tt > FREQ_SYS/10000) {// 100 us
      ret = -1;
      I2CBusInit();
//      i2c_printf("I2C: Error timeout\n");
      break;
    }
  }
  return ret;
}

// Return NAK/ERR
int I2CBusWriteWord(unsigned char i2c_addr, unsigned char reg_addr, unsigned short reg_data)
{
  int ret = -1;
  if((R16_I2C_CTRL1 & RB_I2C_PE) == 0) return ret;
  R16_I2C_CTRL1 &= ~RB_I2C_STOP;
  R16_I2C_OADDR1 = I2C_AckAddr_7bit | i2c_addr;
  R16_I2C_CTRL1 |= RB_I2C_START;// | RB_I2C_ACK;
  ret = i2c_wait_status(RB_I2C_SB);
  if(!ret) {
    R16_I2C_DATAR = i2c_addr;
    ret = i2c_wait_status(RB_I2C_ADDR);
    if(!ret) {
      R16_I2C_DATAR = reg_addr;
      ret = i2c_wait_status(RB_I2C_TxE | RB_I2C_BTF);
      if(!ret) {
        R16_I2C_DATAR = reg_data >> 8;
        ret = i2c_wait_status(RB_I2C_TxE | RB_I2C_BTF);
        if(!ret) {
          R16_I2C_DATAR = reg_data;
          ret = i2c_wait_status(RB_I2C_TxE | RB_I2C_BTF);
        }
      }
    }
  }
  R16_I2C_CTRL1 |= RB_I2C_STOP;
  return ret;
}

int I2CBusReadWord(unsigned char i2c_addr, unsigned char reg_addr, void *preg_data)
{
  int ret = -1;
  uint8_t * pbuf = preg_data;
  if((R16_I2C_CTRL1 & RB_I2C_PE) == 0) return ret; // i2c off
  R16_I2C_CTRL1 &= ~RB_I2C_STOP;
  R16_I2C_OADDR1 = I2C_AckAddr_7bit | i2c_addr;
  R16_I2C_CTRL1 |= RB_I2C_START;
  ret = i2c_wait_status(RB_I2C_SB);
  if(!ret) {
    R16_I2C_DATAR = i2c_addr;
    ret = i2c_wait_status(RB_I2C_ADDR);
    if(!ret) {
      R16_I2C_DATAR = reg_addr;
      ret = i2c_wait_status(RB_I2C_TxE | RB_I2C_BTF);
      if(!ret) {
        R16_I2C_OADDR1 = I2C_AckAddr_7bit | i2c_addr | 1;
        R16_I2C_CTRL1 |= RB_I2C_ACK | RB_I2C_START;
        ret = i2c_wait_status(RB_I2C_SB);
        if(!ret) {
          R16_I2C_DATAR = i2c_addr | 1;
          ret = i2c_wait_status(RB_I2C_ADDR);
          if(!ret) {
            ret = i2c_wait_status(RB_I2C_RxNE);
            if(!ret) {
              R16_I2C_CTRL1 &= ~RB_I2C_ACK;
              pbuf[1] =  R16_I2C_DATAR;
              ret = i2c_wait_status(RB_I2C_RxNE);
              if(!ret) {
                pbuf[0] =  R16_I2C_DATAR;
              }
            }
          }
        }
      }
    }
  }
  R16_I2C_CTRL1 |= RB_I2C_STOP;
	return ret;
}

int I2CBusRead24bits(unsigned char i2c_addr, unsigned char reg_addr, void *preg_data)
{
  int ret = -1;
  uint8_t * pbuf = preg_data;
  if((R16_I2C_CTRL1 & RB_I2C_PE) == 0) return ret; // i2c off
  R16_I2C_CTRL1 &= ~RB_I2C_STOP;
  R16_I2C_OADDR1 = I2C_AckAddr_7bit | i2c_addr;
  R16_I2C_CTRL1 |= RB_I2C_START;
  ret = i2c_wait_status(RB_I2C_SB);
  if(!ret) {
    R16_I2C_DATAR = i2c_addr;
    ret = i2c_wait_status(RB_I2C_ADDR);
    if(!ret) {
      R16_I2C_DATAR = reg_addr;
      ret = i2c_wait_status(RB_I2C_TxE | RB_I2C_BTF);
      if(!ret) {
        R16_I2C_OADDR1 = I2C_AckAddr_7bit | i2c_addr | 1;
        R16_I2C_CTRL1 |= RB_I2C_ACK | RB_I2C_START;
        ret = i2c_wait_status(RB_I2C_SB);
        if(!ret) {
          R16_I2C_DATAR = i2c_addr | 1;
          ret = i2c_wait_status(RB_I2C_ADDR);
          if(!ret) {
            ret = i2c_wait_status(RB_I2C_RxNE);
            if(!ret) {
              pbuf[2] =  R16_I2C_DATAR;
              ret = i2c_wait_status(RB_I2C_RxNE);
              if(!ret) {
                R16_I2C_CTRL1 &= ~RB_I2C_ACK;
                pbuf[1] =  R16_I2C_DATAR;
                ret = i2c_wait_status(RB_I2C_RxNE);
                if(!ret) {
                  pbuf[0] =  R16_I2C_DATAR;
                }
              }
            }
          }
        }
      }
    }
  }
  R16_I2C_CTRL1 |= RB_I2C_STOP;
  return ret;
}

/* Universal I2C/SMBUS read-write transaction
 * wrlen = 1..127 ! */
int I2CBusUtr(void * outdata, i2c_utr_t *tr, unsigned int wrlen) {
  unsigned char * pwrdata = (unsigned char *) &tr->wrdata;
  unsigned char * poutdata = (unsigned char *) outdata;
  unsigned int cntstart = wrlen - (tr->mode & 0x7f);
  unsigned int rdlen = tr->rdlen & 0x7f;

  int ret = -1;
  if((R16_I2C_CTRL1 & RB_I2C_PE) == 0) return ret; // i2c off
  R16_I2C_CTRL1 &= ~RB_I2C_STOP;
  R16_I2C_OADDR1 = I2C_AckAddr_7bit | *pwrdata; // i2c addr
  R16_I2C_CTRL1 |= RB_I2C_START;
  ret = i2c_wait_status(RB_I2C_SB);
  if(!ret) {
    R16_I2C_DATAR = *pwrdata++; // i2c wr/rd addr
    ret = i2c_wait_status(RB_I2C_ADDR);
    while((!ret) && wrlen--) {
        // write data
      R16_I2C_DATAR = *pwrdata++; // wr data
      ret = i2c_wait_status(RB_I2C_TxE | RB_I2C_BTF);
      if((!ret) && wrlen == cntstart) { // + send start & id
        if(tr->mode & 0x80) {
          // generate stop
          R16_I2C_CTRL1 |= RB_I2C_STOP;
          ret = i2c_wait_status(RB_I2C_TxE | RB_I2C_BTF);
          if(ret)
            break;
        }
        // generate start
        R16_I2C_OADDR1 |= 1;
        R16_I2C_CTRL1 |= RB_I2C_ACK | RB_I2C_START;
        ret = i2c_wait_status(RB_I2C_SB);
        if(!ret)
          R16_I2C_DATAR = R16_I2C_OADDR1 & 0xff; // i2c rd addr
          ret = i2c_wait_status(RB_I2C_ADDR);
      }
    }
    while((!ret) && rdlen--) {
      if(rdlen == 1 && (tr->rdlen & 0x80) == 0)
        // NACK
        R16_I2C_CTRL1 &= ~RB_I2C_ACK;
      else {
        R16_I2C_CTRL1 |= RB_I2C_ACK;
      }
      ret = i2c_wait_status(RB_I2C_RxNE);
      if(!ret) {
        *poutdata++ = R16_I2C_DATAR;
        ret = i2c_wait_status(RB_I2C_RxNE);
      }
    }
  }
  R16_I2C_CTRL1 |= RB_I2C_STOP;
  return ret;
}

void I2CBusDeInit(void) {
  if(R16_I2C_CTRL1 & RB_I2C_PE) { // I2C peripheral enable ?
    R16_I2C_CTRL1 |= RB_I2C_SWRST;
    R16_I2C_CTRL1 &= ~RB_I2C_SWRST;
    R16_I2C_CTRL1 &= ~RB_I2C_PE;
  }
}

void I2CBusInit(void) {

  uint32_t clk_khz =  cfg_i2c.clk_khz; // clk | 0x8000 - stretch enable

  GPIOB_ModeCfg(I2C_DEV_SCL | I2C_DEV_SDA, GPIO_ModeIN_PU);

  R16_I2C_CTRL1 &= ~RB_I2C_PE;

  R16_I2C_CTRL1 |= RB_I2C_SWRST;

  R16_I2C_CTRL1 &= ~RB_I2C_SWRST;

  R16_I2C_CTRL2 = (R16_I2C_CTRL2 & ~RB_I2C_FREQ) | (FREQ_SYS / 1000000);

  R16_I2C_RTR = ((FREQ_SYS / 1000000)*300)/1000 + 1;

#if 0
  if(clk_khz < 400)
    R16_I2C_CKCFGR = I2C_DutyCycle_2 | (((FREQ_SYS / 1000) / ((clk_khz & 0x7fff) * 3)) & RB_I2C_CCR);
  else
    R16_I2C_CKCFGR = RB_I2C_F_S | I2C_DutyCycle_16_9 | (((FREQ_SYS / 1000) / ((clk_khz & 0x7fff) * 25)) & RB_I2C_CCR);
#else // super speed >= 1.3MHz
  R16_I2C_CKCFGR =  I2C_DutyCycle_2 | ((FREQ_SYS / 2000) / (clk_khz & 0x7fff));
#endif

  R16_I2C_CTRL1 |= RB_I2C_PE;

  if((clk_khz & 0x8000) == 0)
    R16_I2C_CTRL1 |= RB_I2C_NOSTRETCH;

  R16_I2C_CTRL1 |= RB_I2C_SMBUS | RB_I2C_SMBTYPE | RB_I2C_ACK;

  R16_I2C_OADDR1 = 0;
  R16_I2C_OADDR1 = I2C_AckAddr_7bit | 0x80;

  R16_I2C_CTRL2 = 0;//|= I2C_IT_BUF | I2C_IT_EVT | I2C_IT_ERR;
}

#endif // USE_I2C_DEV
