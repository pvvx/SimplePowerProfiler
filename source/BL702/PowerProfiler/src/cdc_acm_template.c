#include "main_config.h"
#include "usbd_core.h"
#include "usbd_cdc.h"
#include "bflb_adc.h"
#include "adc_dma.h"
#include "i2c_dev.h"
#include "cmd_cfg.h"
#include "usb_buffer.h"

#define cdc_printf(...) printf(__VA_ARGS__)

/*!< endpoint address */
#define CDC_IN_EP  0x81
#define CDC_OUT_EP 0x02
#define CDC_INT_EP 0x83

#define USBD_VID           0xFFFF
#define USBD_PID           0xFFFF
#define USBD_MAX_POWER     100
#define USBD_LANGID_STRING 1033
#define USBD_RXBUFSIZE		2048
#define USBD_TXBUFSIZE		2048

/*!< config descriptor size */
#define USB_CONFIG_SIZE (9 + CDC_ACM_DESCRIPTOR_LEN)

/*!< global descriptor */
static const uint8_t cdc_descriptor[] = {
    USB_DEVICE_DESCRIPTOR_INIT(USB_2_0, 0xEF, 0x02, 0x01, USBD_VID, USBD_PID, 0x0100, 0x01),
    USB_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, 0x02, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
    CDC_ACM_DESCRIPTOR_INIT(0x00, CDC_INT_EP, CDC_OUT_EP, CDC_IN_EP, 0x02),
    ///////////////////////////////////////
    /// string0 descriptor
    ///////////////////////////////////////
    USB_LANGID_INIT(USBD_LANGID_STRING),
    ///////////////////////////////////////
    /// string1 descriptor
    ///////////////////////////////////////
    0x14,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    'C', 0x00,                  /* wcChar0 */
    'h', 0x00,                  /* wcChar1 */
    'e', 0x00,                  /* wcChar2 */
    'r', 0x00,                  /* wcChar3 */
    'r', 0x00,                  /* wcChar4 */
    'y', 0x00,                  /* wcChar5 */
    'U', 0x00,                  /* wcChar6 */
    'S', 0x00,                  /* wcChar7 */
    'B', 0x00,                  /* wcChar8 */
    ///////////////////////////////////////
    /// string2 descriptor
    ///////////////////////////////////////
    0x26,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    'C', 0x00,                  /* wcChar0 */
    'h', 0x00,                  /* wcChar1 */
    'e', 0x00,                  /* wcChar2 */
    'r', 0x00,                  /* wcChar3 */
    'r', 0x00,                  /* wcChar4 */
    'y', 0x00,                  /* wcChar5 */
    'U', 0x00,                  /* wcChar6 */
    'S', 0x00,                  /* wcChar7 */
    'B', 0x00,                  /* wcChar8 */
    ' ', 0x00,                  /* wcChar9 */
    'C', 0x00,                  /* wcChar10 */
    'D', 0x00,                  /* wcChar11 */
    'C', 0x00,                  /* wcChar12 */
    ' ', 0x00,                  /* wcChar13 */
    'D', 0x00,                  /* wcChar14 */
    'E', 0x00,                  /* wcChar15 */
    'M', 0x00,                  /* wcChar16 */
    'O', 0x00,                  /* wcChar17 */
    ///////////////////////////////////////
    /// string3 descriptor
    ///////////////////////////////////////
    0x16,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    '2', 0x00,                  /* wcChar0 */
    '0', 0x00,                  /* wcChar1 */
    '2', 0x00,                  /* wcChar2 */
    '2', 0x00,                  /* wcChar3 */
    '1', 0x00,                  /* wcChar4 */
    '2', 0x00,                  /* wcChar5 */
    '3', 0x00,                  /* wcChar6 */
    '4', 0x00,                  /* wcChar7 */
    '5', 0x00,                  /* wcChar8 */
    '6', 0x00,                  /* wcChar9 */
#ifdef CONFIG_USB_HS
    ///////////////////////////////////////
    /// device qualifier descriptor
    ///////////////////////////////////////
    0x0a,
    USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER,
    0x00,
    0x02,
    0x02,
    0x02,
    0x01,
    0x40,
    0x01,
    0x00,
#endif
    0x00
};

USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t read_buffer[USBD_RXBUFSIZE];
USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t write_buffer[USBD_TXBUFSIZE];

volatile bool ep_tx_busy_flag = false;

#ifdef CONFIG_USB_HS
#define CDC_MAX_MPS 512
#else
#define CDC_MAX_MPS 64
#endif

void usbd_configure_done_callback(void)
{
    /* setup first out ep read transfer */
    usbd_ep_start_read(CDC_OUT_EP, read_buffer, USBD_RXBUFSIZE);
}

void usbd_cdc_acm_bulk_out(uint8_t ep, uint32_t nbytes)
{
	USB_LOG_DBG("actual out len:%u\r\n", (unsigned int)nbytes);
	blk_rx_pkt_t * pbufi = (blk_rx_pkt_t *) read_buffer;

	cdc_printf("RdBuf: ");
	for (int i = 0; i < nbytes; i++)
		cdc_printf("%02x ", read_buffer[i]);
	cdc_printf("\r\n");

	if ((nbytes >= sizeof(blk_head_t))
		&& (nbytes >= pbufi->head.size + sizeof(blk_head_t))) {
		if(cmd_decode(&send_pkt, pbufi, nbytes)) {
			Ring_Buffer_Write(&usb_tx_rb, (uint8_t *)&send_pkt, send_pkt.head.size + sizeof(blk_head_t));
			cdc_printf("WrBuf: ");
			uint8_t * p = (uint8_t *)&send_pkt;
			for (int i = 0; i < send_pkt.head.size + sizeof(blk_head_t); i++)
				cdc_printf("%02x ", p[i]);
			cdc_printf("\r\n");
		}
	}
    /* setup next out ep read transfer */
    usbd_ep_start_read(CDC_OUT_EP, read_buffer, USBD_RXBUFSIZE);
}

// конец передачи
void usbd_cdc_acm_bulk_in(uint8_t ep, uint32_t nbytes)
{
	USB_LOG_DBG("actual in len:%u\r\n", (unsigned int)nbytes);

    if ((nbytes % CDC_MAX_MPS) == 0 && nbytes) {
        /* send zlp */
        usbd_ep_start_write(CDC_IN_EP, NULL, 0);
    } else {
        ep_tx_busy_flag = false;
    }
}

/*!< endpoint call back */
struct usbd_endpoint cdc_out_ep = {
    .ep_addr = CDC_OUT_EP,
    .ep_cb = usbd_cdc_acm_bulk_out
};

struct usbd_endpoint cdc_in_ep = {
    .ep_addr = CDC_IN_EP,
    .ep_cb = usbd_cdc_acm_bulk_in
};

struct usbd_interface intf0;
struct usbd_interface intf1;

/* function ------------------------------------------------------------------*/
void cdc_acm_init(void)
{
    const uint8_t data[10] = { 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30 };

    memcpy(&write_buffer[0], data, 10);
    memset(&write_buffer[10], 'a', USBD_TXBUFSIZE-10);

    usbd_desc_register(cdc_descriptor);
    usbd_add_interface(usbd_cdc_acm_init_intf(&intf0));
    usbd_add_interface(usbd_cdc_acm_init_intf(&intf1));
    usbd_add_endpoint(&cdc_out_ep);
    usbd_add_endpoint(&cdc_in_ep);
    usbd_initialize();
}

volatile uint8_t dtr_enable = 0;

void usbd_cdc_acm_set_dtr(uint8_t intf, bool dtr)
{
	if(dtr) {
		if(!dtr_enable) {
#if USE_I2C_DEV
			I2CDevWakeUp();
#endif
		}
	} else {
		if(dtr_enable) {
#if USE_ADC_DEV
			ADC_Stop();
#endif
#if USE_I2C_DEV
			I2CDevSleep();
#endif
		}
	}
	dtr_enable = dtr;
}

volatile uint8_t rts_enable = 0;

void usbd_cdc_acm_set_rts(uint8_t intf, bool rts)
{
	rts_enable = rts;
}

void cdc_acm_data_send_with_dtr_test(void)
{
    if (dtr_enable) {
        ep_tx_busy_flag = true;
        usbd_ep_start_write(CDC_IN_EP, write_buffer, USBD_TXBUFSIZE-1);
        while (ep_tx_busy_flag) {
        }
    }
}

void usb_dc_send_from_ringbuffer(Ring_Buffer_Type *rb)
{
//    if (dtr_enable) {
    	while (ep_tx_busy_flag)
    		;
    	ep_tx_busy_flag = true;
    	size_t len = Ring_Buffer_Read(&usb_tx_rb, write_buffer, USBD_TXBUFSIZE - 1);
    	if(len) {
    		usbd_ep_start_write(CDC_IN_EP, write_buffer, len);
    		USB_LOG_DBG("Write %u bytes\r\n", len);
    	} else
    		ep_tx_busy_flag = false;
//    }
}

