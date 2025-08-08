
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

#define DATA_BUF_CNT	1024 // 64, 128, 256, ..

#define DATA8_BLK_CNT   120 // 120  // 6 x N
#define DATA16_BLK_CNT (DATA8_BLK_CNT/2) // = 60
#define DATA24_BLK_CNT (DATA8_BLK_CNT/3) // = 40

extern uint32_t blk_read_count, blk_overflow_cnt;

extern uint32_t data_buffer[DATA_BUF_CNT];
extern uint32_t data_buffer_wr_ptr, data_buffer_rd_ptr;

void task_data_out(void);
void usb_buffers_init(void);

#endif /* USB_BUFFER_H_ */
