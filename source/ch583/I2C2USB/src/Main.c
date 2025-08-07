/*
 * File Name          : Main.c
 */

#include "common.h"
#include "app_drv_fifo.h"
#include "app_usb.h"
#include "cmd_cfg.h"
#include "i2c_dev.h"

int old_send_len;

#if USE_TEST_ADC
void AdcInit(uint8_t flg);
uint8_t ADC_on;

#else

app_drv_fifo_t app_tx_fifo;
uint8_t app_tx_buffer[APP_TX_BUFFER_LENGTH];

/*********************************************************************
 * @fn      USBSendData
 *
 * @brief   Send data to the host
 *
 * @return  data send
 */
int USBSendData(void)
{
    if((R8_UEP1_CTRL & MASK_UEP_T_RES) == UEP_T_RES_ACK) {
        return 1;
    }
    int len = app_drv_fifo_length(&app_tx_fifo);
    if(len) {
        if(len > MAX_TX_PACKET_SIZE){
            len = MAX_TX_PACKET_SIZE;
        }
        if(app_drv_fifo_read(&app_tx_fifo, &EP1_Databuf[MAX_PACKET_SIZE], (uint16_t *)&len) == APP_DRV_FIFO_RESULT_SUCCESS) {
            old_send_len = len;
            // DevEP2_IN_Deal(len);
            R8_UEP1_T_LEN = (UINT8)len;
            PFIC_DisableIRQ(USB_IRQn);
            R8_UEP1_CTRL = (R8_UEP1_CTRL & (~MASK_UEP_T_RES)) | UEP_T_RES_ACK;
            PFIC_EnableIRQ(USB_IRQn);
        }
    } else if(old_send_len == MAX_TX_PACKET_SIZE) {
      old_send_len = 0;
      R8_UEP1_T_LEN = 0;
      PFIC_DisableIRQ(USB_IRQn);
      R8_UEP1_CTRL = (R8_UEP1_CTRL & (~MASK_UEP_T_RES)) | UEP_T_RES_ACK;
      PFIC_EnableIRQ(USB_IRQn);
    }
    return 0;
}
#endif

void DebugInit( void )
{
  GPIOA_SetBits( GPIO_Pin_9 );
  GPIOA_ModeCfg( GPIO_Pin_8, GPIO_ModeIN_PU );
  GPIOA_ModeCfg( GPIO_Pin_9, GPIO_ModeOut_PP_5mA );
  UART1_DefInit();
}

#if USE_TEST_ADC

#define ADC_BUF_LEN   512 // 256

uint16_t adcBuff[ADC_BUF_LEN];
int adc_idx_read;
#if  USE_TIMER_SPS
struct {
    uint32_t tim_count;
    uint32_t old_tim_count;
    uint32_t count_blk;
    uint32_t sps;
} adc_stat;
#endif

struct {
    uint32_t sps;
    uint8_t  freg;
    uint8_t  pga;
    uint8_t  diff;
} adc_cfg ={
           .sps = ADC_DEF_SPS,
           .freg = SampleFreq_8, //SampleFreq_4, SampleFreq_5_33
           .pga = ADC_PGA_0,
           .diff = 0
};

void AdcInit(uint8_t flg_enable) {
  //ADC_StopDMA();
  R8_ADC_CTRL_DMA = 0;
  GPIOA_ModeCfg(GPIO_Pin_4 | GPIO_Pin_12, GPIO_ModeIN_Floating);
  if(flg_enable) {
    if(adc_cfg.sps > ADC_MAX_SPS)
      adc_cfg.sps = ADC_MAX_SPS;
    else if(adc_cfg.sps < ADC_MIN_SPS)
      adc_cfg.sps = ADC_MIN_SPS;
    adc_cfg.freg &= 3;
    adc_cfg.pga &= 3;
    if(adc_cfg.diff)
      ADC_ExtDiffChSampInit(adc_cfg.freg, adc_cfg.pga);
    else
      ADC_ExtSingleChSampInit(adc_cfg.freg, adc_cfg.pga);

    //PRINT("RoughCalib_Value =%d \n", ADC_DataCalib_Rough());

    R8_ADC_CHANNEL = 0;

    R8_ADC_AUTO_CYCLE = 256 - (FREQ_SYS/16/adc_cfg.sps); // 181 -> 50kHz
    adc_idx_read = 0;
#if  USE_TIMER_SPS
    adc_stat.count_blk = 0;
    adc_stat.sps = 0;
#endif
    //ADC_DMACfg(ENABLE, (uint32_t)&adcBuff[0], (uint32_t)&adcBuff[ADC_BUF_LEN], ADC_Mode_LOOP);
    //ADC_StartAutoDMA();
    R16_ADC_DMA_BEG = (uint32_t)&adcBuff[0];
    R16_ADC_DMA_END = (uint32_t)&adcBuff[ADC_BUF_LEN];
    R8_ADC_CTRL_DMA = RB_ADC_DMA_LOOP | RB_ADC_DMA_ENABLE | RB_ADC_AUTO_EN; // | RB_ADC_CONT_EN;
  }
  ADC_on = flg_enable;
}

__attribute__((section(".highcode")))
void ADCSendToUSB(void) {
  // USB UEP1 ready ?
  if((R8_UEP1_CTRL & MASK_UEP_T_RES) == UEP_T_RES_ACK)
      return;
  // ADC On?
  //if((R8_ADC_CTRL_DMA & RB_ADC_AUTO_EN) == 0
//      || ADC_on == 0) {
  if(!ADC_on) {
    // ADC Off
    if(old_send_len == MAX_TX_PACKET_SIZE) {
      old_send_len = 0;
#if  USE_TIMER_SPS
      adc_stat.count_blk++;
#endif
      R8_UEP1_T_LEN = (UINT8)old_send_len;
      PFIC_DisableIRQ(USB_IRQn);
      R8_UEP1_CTRL = (R8_UEP1_CTRL & (~MASK_UEP_T_RES)) | UEP_T_RES_ACK;
      PFIC_EnableIRQ(USB_IRQn);
    }
    return;
  }
  int cur_idx = R16_ADC_DMA_NOW - R16_ADC_DMA_BEG;
  int len = cur_idx - adc_idx_read;
  len &= ~1;
  if(len == 0) {
    if(old_send_len == MAX_TX_PACKET_SIZE) {
      old_send_len = 0;
      R8_UEP1_T_LEN = (UINT8)old_send_len;
      PFIC_DisableIRQ(USB_IRQn);
      R8_UEP1_CTRL = (R8_UEP1_CTRL & (~MASK_UEP_T_RES)) | UEP_T_RES_ACK;
      PFIC_EnableIRQ(USB_IRQn);
    }
    return;
  }
  if(len < 0 )
    len += sizeof(adcBuff);
#if 0
  if(len < MAX_TX_PACKET_SIZE - 2)
    return;
#else
  if(len > MAX_TX_PACKET_SIZE - 2)
#endif
    len = MAX_TX_PACKET_SIZE - 2;
  uint8_t * ps = (uint8_t *)adcBuff + adc_idx_read;
  uint8_t * pd = &EP1_Databuf[MAX_PACKET_SIZE];
  old_send_len = len + 2;
  // send head
#if  USE_TIMER_SPS
  adc_stat.sps += len >> 1;
  adc_stat.count_blk++;
#endif
  *pd++ = (uint8_t)(len >> 1);
  *pd++ = 0x0a;
  // send data
  while(len--) {
    *pd++ = *ps++;
    adc_idx_read++;
    if(adc_idx_read >= ADC_BUF_LEN*2) {
      adc_idx_read = 0;
      ps = (uint8_t *)adcBuff;
    }
  };
  // DevEP1_IN_Deal(len);
  R8_UEP1_T_LEN = (UINT8)old_send_len;
  PFIC_DisableIRQ(USB_IRQn);
  R8_UEP1_CTRL = (R8_UEP1_CTRL & (~MASK_UEP_T_RES)) | UEP_T_RES_ACK;
  PFIC_EnableIRQ(USB_IRQn);
}

#if  USE_TIMER_SPS
/*********************************************************************
 * @fn      TMR0_IRQHandler
 *
 * @brief   TMR0 IRQ
 *
 * @return  none
 */
__INTERRUPT
__HIGH_CODE
void TMR0_IRQHandler(void)
{
    if(TMR0_GetITFlag(RB_TMR_IF_CYC_END))
    {
        TMR0_ClearITFlag(RB_TMR_IF_CYC_END);
        adc_stat.tim_count++;
    }
}
#endif // USE_TIMER_SPS
#endif // USE_TEST_ADC

int main()
{
#if(defined(DCDC_ENABLE)) && (DCDC_ENABLE == TRUE) // Run 375 ksps to USB - Average 12 mA, if DC-DC On: 10 mA
  PWR_DCDCCfg(ENABLE);
#endif
  SetSysClock( CLK_SOURCE_PLL_60MHz );
#ifdef DEBUG
  DebugInit();  //PA9
#endif

  PRINT("\nStart\n");
#if !USE_TEST_ADC
  app_drv_fifo_init(&app_tx_fifo, app_tx_buffer, APP_TX_BUFFER_LENGTH);
#endif
#ifdef GPIO_TEST
  GPIOB_ModeCfg(GPIO_TEST, GPIO_ModeOut_PP_5mA);
#endif
  InitSysTickCnt();
#if USE_I2C_DEV
  I2CDevInit();
#endif
  app_usb_init();

#if  USE_TIMER_SPS
  TMR0_TimerInit(FREQ_SYS);
  TMR0_ITCfg(ENABLE, TMR0_3_IT_CYC_END);
  PFIC_EnableIRQ(TMR0_IRQn);
#endif
#ifdef ADC_T_TEST
  ADC_InterTSSampInit();
  int adc_val = 0;
  for(int i = 0; i < 16; i++)
      adc_val += ADC_ExcutSingleConver();
  PRINT("Temp: %d\n", adc_to_temperature_celsius(adc_val>>16));
#endif
#if USE_TEST_ADC
  AdcInit(0);
#endif

  while(1)
  {
#if USE_I2C_DEV
    if(app_cmd_len) {
      if(app_cmd_len > 1) {
/*
        PRINT("cmd[%d]: %02x,%02x", app_cmd_len, app_cmd_buf[0], app_cmd_buf[1]);
        int len = 2;
        for(; len < app_cmd_len; len++)
          PRINT(",%02x", app_cmd_buf[len]);
        PRINT("\n");
*/
        int len = cmd_decode(&send_pkt, (blk_rx_pkt_t *)app_cmd_buf, app_cmd_len);
        if(len) {
          if(app_drv_fifo_write(&app_tx_fifo, (uint8_t *)&send_pkt, (uint16_t *)&len) == APP_DRV_FIFO_RESULT_SUCCESS) {
          }
        }
      }
      app_cmd_len = 0;
    };
    I2CDevTask();
    USBSendData();
#else
#if USE_TEST_ADC
    ADCSendToUSB();
    if(app_cmd_len) {
      PRINT("cmd: %02x\n", app_cmd_buf[0]);
      if(app_cmd_buf[0] == 0x55) {
        if(app_cmd_len > 4) {
           memcpy(&adc_cfg, &app_cmd_buf[1], (app_cmd_len > sizeof(adc_cfg))? sizeof(adc_cfg) : app_cmd_len - 1);
        }
        AdcInit(1);
      } else if(app_cmd_len > 1 && app_cmd_buf[0] == 0xAA) {
        R8_ADC_AUTO_CYCLE = app_cmd_buf[1];  // 231 -> 150ksps, 6 -> 15 ksps
      } else {
        AdcInit(0);
      }
      app_cmd_len = 0;
    }
#if  USE_TIMER_SPS
    if(adc_stat.tim_count != adc_stat.old_tim_count) {
      adc_stat.old_tim_count = adc_stat.tim_count;
      PRINT("%u sps, %u blk\n", adc_stat.sps, adc_stat.count_blk);
      adc_stat.count_blk = 0;
      adc_stat.sps = 0;
    }
#endif
#else
    USBSendData();
#endif
#endif
#if  DEBUG_USB
    if(debug_usb) {
      PRINT("usb: %02x\n", debug_usb);
      debug_usb = 0;
    }
#endif
  }
}


