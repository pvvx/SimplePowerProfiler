/*
 * usb_buffer.c
 *
 *  Created on: 9 янв. 2023 г.
 *      Author: pvvx
 */
#include "main_config.h"
#include "usb_buffer.h"

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

void usb_buffers_init(void) {
	/* init mem for ring_buffer */
	//    memset(usb_rx_mem, 0, USB_RX_RINGBUFFER_SIZE);
	memset(usb_tx_mem, 0, USB_TX_RINGBUFFER_SIZE);
	/* init ring_buffer */
	//    Ring_Buffer_Init(&usb_rx_rb, usb_rx_mem, USB_RX_RINGBUFFER_SIZE, _ringbuffer_lock, _ringbuffer_unlock);

	#if USE_LOCK_RNG
	Ring_Buffer_Init(&usb_tx_rb, usb_tx_mem, USB_TX_RINGBUFFER_SIZE, _ringbuffer_lock, _ringbuffer_unlock);
	#else
	Ring_Buffer_Init(&usb_tx_rb, usb_tx_mem, USB_TX_RINGBUFFER_SIZE, NULL, NULL);
	#endif
}


