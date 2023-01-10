#include "main_config.h"
#include "usbh_core.h"
#include "board.h"
#include "i2c_dev.h"
#include "usb_buffer.h"


extern void cdc_acm_init(void);
extern void usb_dc_send_from_ringbuffer(Ring_Buffer_Type *rb);

int main(void)
{
    board_init();

    cdc_acm_init(); // BL702 USB2.0FS max output 1000 kbytes/s

    usb_buffers_init(); // init usb ring buffers

    while (1) {
#if USE_I2C_DEV
    	task_I2C();
#endif
    	usb_dc_send_from_ringbuffer(&usb_tx_rb);
    }
}
