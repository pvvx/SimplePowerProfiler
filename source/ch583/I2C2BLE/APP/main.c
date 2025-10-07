/********************************** (C) COPYRIGHT *******************************
 * File Name          : main.c
 * Author             : WCH
 * Version            : V1.1
 * Date               : 2020/08/06
 * Description        : 外设从机应用主函数及任务系统初始化
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

/******************************************************************************/
/* 头文件包含 */
#include "common.h"
#include "app_drv_fifo.h"
#include "app_usb.h"
#include "HAL.h"
#include "peripheral.h"
#include "i2c_dev.h"

int old_send_len;
volatile uint8_t ble_enable;
app_drv_fifo_t app_tx_fifo;
uint8_t app_tx_buffer[APP_TX_BUFFER_LENGTH]; // app_tx_fifo buffer: 2048 bytes

/*********************************************************************
 * GLOBAL TYPEDEFS
 */
__attribute__((aligned(4))) uint32_t MEM_BUF[BLE_MEMHEAP_SIZE / 4];

#if(defined(BLE_MAC)) && (BLE_MAC == TRUE)
const uint8_t MacAddr[6] = {0x84, 0xC2, 0xE4, 0x03, 0x02, 0x02};
#endif

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

/*********************************************************************
 * @fn      Main_Circulation
 *
 * @brief   主循环
 *
 * @return  none
 */
__HIGH_CODE
__attribute__((noinline))
void Main_Circulation()
{
    while(1)
    {
        TMOS_SystemProcess();
        I2CDevTask();
        if(!ble_enable)
            USBSendData();
    }
}

/*********************************************************************
 * @fn      DebugInit
 *
 * @brief
 *
 * @return  Debug Init
 */
void DebugInit( void )
{
  GPIOA_SetBits( GPIO_Pin_9 );
  GPIOA_ModeCfg( GPIO_Pin_8, GPIO_ModeIN_PU );
  GPIOA_ModeCfg( GPIO_Pin_9, GPIO_ModeOut_PP_5mA );
  UART1_DefInit();
}

/*********************************************************************
 * @fn      main
 *
 * @brief   主函数
 *
 * @return  none
 */
int main(void)
{
#if(defined(DCDC_ENABLE)) && (DCDC_ENABLE == TRUE)
    PWR_DCDCCfg(ENABLE);
#endif
    SetSysClock(CLK_SOURCE_PLL_60MHz);
#if(defined(HAL_SLEEP)) && (HAL_SLEEP == TRUE)
    GPIOA_ModeCfg(GPIO_Pin_All, GPIO_ModeIN_PU);
    GPIOB_ModeCfg(GPIO_Pin_All, GPIO_ModeIN_PU);
#endif
#ifdef DEBUG
    DebugInit();  //PA9
#endif
    app_drv_fifo_init(&app_tx_fifo, app_tx_buffer, APP_TX_BUFFER_LENGTH);
    PRINT("%s\n", VER_LIB);
    CH58X_BLEInit();
    HAL_Init();
    GAPRole_PeripheralInit();
    Peripheral_Init();
#if USE_I2C_DEV
    I2CDevInit();
#endif
    app_usb_init();
    Main_Circulation();
}

/******************************** endfile @ main ******************************/
