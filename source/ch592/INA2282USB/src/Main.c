/*
 * File Name          : Main.c
 */

#include "common.h"
#include "app_drv_fifo.h"
#include "app_usb.h"
#include "cmd_cfg.h"
#include "i2c_dev.h"

int old_send_len;

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

void DebugInit( void )
{
  GPIOA_SetBits( GPIO_Pin_9 );
  GPIOA_ModeCfg( GPIO_Pin_8, GPIO_ModeIN_PU );
  GPIOA_ModeCfg( GPIO_Pin_9, GPIO_ModeOut_PP_5mA );
  UART1_DefInit();
}


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
  app_drv_fifo_init(&app_tx_fifo, app_tx_buffer, APP_TX_BUFFER_LENGTH);
#ifdef GPIO_TEST
  GPIOB_ModeCfg(GPIO_TEST, GPIO_ModeOut_PP_5mA);
#endif
  InitSysTickCnt();
#if USE_I2C_DEV
  I2CDevInit();
#endif
  app_usb_init();
  while(1)
  {
    if(app_cmd_len) {
      if(app_cmd_len > 1) {
#if  DEBUG_USB
        PRINT("cmd[%d]: %02x,%02x", app_cmd_len, app_cmd_buf[0], app_cmd_buf[1]);
        int len = 2;
        for(; len < app_cmd_len; len++)
          PRINT(",%02x", app_cmd_buf[len]);
        PRINT("\n");
#endif
        int len = cmd_decode(&send_pkt, (blk_rx_pkt_t *)app_cmd_buf, app_cmd_len);
        if(len) {
          if(app_drv_fifo_write(&app_tx_fifo, (uint8_t *)&send_pkt, (uint16_t *)&len) != APP_DRV_FIFO_RESULT_SUCCESS) {
            PRINT("CMD: Out buffer overflow!\r\n");
          }
        }
      }
      app_cmd_len = 0;
    };
#if USE_I2C_DEV
    I2CDevTask();
#endif
    USBSendData();
#if  DEBUG_USB
    if(debug_usb) {
      PRINT("usb: %02x\n", debug_usb);
      debug_usb = 0;
    }
#endif
  }
}


