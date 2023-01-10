
/*
 * usb_buffer.h
 *
 *  Created on: 9 янв. 2023 г.
 *      Author: pvvx
 */

#ifndef USB_BUFFER_H_
#define USB_BUFFER_H_

#include "ring_buffer.h"

#define USE_LOCK_RNG 	0

//#define USB_RX_RINGBUFFER_SIZE (4 * 1024)
#define USB_TX_RINGBUFFER_SIZE (4 * 1024)

//extern uint8_t usb_rx_mem[USB_RX_RINGBUFFER_SIZE];
//extern uint8_t usb_tx_mem[USB_TX_RINGBUFFER_SIZE];

//extern Ring_Buffer_Type usb_rx_rb;
extern Ring_Buffer_Type usb_tx_rb;

void usb_buffers_init(void);

#endif /* USB_BUFFER_H_ */
