/********************************** (C) COPYRIGHT *******************************
 * File Name          : APP_USB.h
 * Author             : WCH
 * Version            : V1.1
 * Date               : 2022/01/19
 * Description        :
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

#ifndef _APP_USB_H
#define _APP_USBE_H

#ifdef __cplusplus
extern "C" {
#endif

#define USE_MAC_SER         1
#define USE_SET_LINE_CODING 0

#if  DEBUG_USB
extern uint32_t debug_usb;
#endif

#define MAX_PACKET_SIZE 64

#define MAX_TX_PACKET_SIZE    64

extern int old_send_len;

extern uint8_t EP0_Databuf[MAX_PACKET_SIZE + MAX_PACKET_SIZE + MAX_PACKET_SIZE];    //ep0(64)+ep4_out(64)+ep4_in(64)
extern uint8_t EP1_Databuf[MAX_PACKET_SIZE + MAX_PACKET_SIZE];    //ep1_out(64)+ep1_in(64)
//extern uint8_t EP2_Databuf[MAX_PACKET_SIZE + MAX_PACKET_SIZE];    //ep2_out(64)+ep2_in(64)
//extern uint8_t EP3_Databuf[MAX_PACKET_SIZE + MAX_PACKET_SIZE];    //ep3_out(64)+ep3_in(64)
//extern uint8_t EP2_Databuf[MAX_PACKET_SIZE + MAX_PACKET_SIZE];

void app_usb_init(void);

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* _BLE_usb_SERVICE_H */
