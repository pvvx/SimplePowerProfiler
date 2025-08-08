/*
 * usb_buffer.c
 *
 *  Created on: 9 янв. 2023 г.
 *      Author: pvvx
 */
#include "main_config.h"
#include "board.h"
#include "usb_buffer.h"
#include "cmd_cfg.h"

uint32_t blk_read_count, blk_overflow_cnt;

uint32_t data_buffer[DATA_BUF_CNT];
uint32_t data_buffer_wr_ptr, data_buffer_rd_ptr;

struct {
	uint16_t id;
	uint8_t data[DATA8_BLK_CNT];
} data_blk_out;

//uint8_t usb_rx_mem[USB_RX_RINGBUFFER_SIZE];
uint8_t usb_tx_mem[USB_TX_RINGBUFFER_SIZE];

//Ring_Buffer_Type usb_rx_rb;
Ring_Buffer_Type usb_tx_rb;

#if USE_LOCK_RNG
volatile uint32_t nesting = 0;

static void _ringbuffer_lock()
{
    __disable_irq();
    nesting++;
}

static void _ringbuffer_unlock()
{
    nesting--;
    if (nesting == 0) {
        __enable_irq();
    }
}
#endif


void task_data_out(void) {
	uint32_t prd = data_buffer_rd_ptr;
	uint32_t pwr = data_buffer_wr_ptr;
	uint32_t size;
	uint8_t *p = data_blk_out.data;
	if(pwr < prd) {
		size = DATA_BUF_CNT - prd + pwr;
		if(size >= DATA24_BLK_CNT) {
			size = DATA24_BLK_CNT;
			data_blk_out.id = (CMD_DEV_EADC << 8) + sizeof(data_blk_out.data);

			for(int i = prd; i < DATA_BUF_CNT && size; i++) {
//				*p++ = data_buffer[i];
				*p++ = data_buffer[i] >> 8;
				*p++ = data_buffer[i] >> 16;
				*p++ = data_buffer[i] >> 24;
				prd++;
				size--;
			}
			for(int i = 0; size; i++) {
//				*p++ = data_buffer[i];
				*p++ = data_buffer[i] >> 8;
				*p++ = data_buffer[i] >> 16;
				*p++ = data_buffer[i] >> 24;
				prd++;
				size--;
			}
			data_buffer_rd_ptr = prd & (DATA_BUF_CNT - 1);
		} else
			return;

	} else if (pwr > prd) {
		size = pwr - prd;
		if(size >= DATA24_BLK_CNT) {
			size = DATA24_BLK_CNT;
			data_blk_out.id = (CMD_DEV_EADC << 8) + sizeof(data_blk_out.data);
			for(int i = 0; size; i++) {
//				*p++ = data_buffer[prd];
				*p++ = data_buffer[prd] >> 8;
				*p++ = data_buffer[prd] >> 16;
				*p++ = data_buffer[prd] >> 24;
				prd++;
				size--;
			}
			data_buffer_rd_ptr = prd & (DATA_BUF_CNT - 1);
		} else
			return;
	} else
		return;

	if(sizeof(data_blk_out) == Ring_Buffer_Write(&usb_tx_rb, (uint8_t *)&data_blk_out, sizeof(data_blk_out))) {
		blk_read_count++;
	} else {
		printf("USB: buffer overflow!\r\n");
		blk_overflow_cnt++;
	}

}

/* init ring_buffer */
void usb_buffers_init(void) {
	memset(usb_tx_mem, 0, USB_TX_RINGBUFFER_SIZE);
	Ring_Buffer_Init(&usb_tx_rb, usb_tx_mem, USB_TX_RINGBUFFER_SIZE,
#if USE_LOCK_RNG
			_ringbuffer_lock, _ringbuffer_unlock);
#else
			NULL, NULL);
#endif
}


