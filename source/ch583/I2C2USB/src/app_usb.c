/********************************** (C) COPYRIGHT *******************************
 * File Name          : app_usb.c
 * Author             : WCH
 * Version            : V1.1
 * Date               : 2022/01/19
 * Description        :
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include "common.h"
#include "app_drv_fifo.h"
#include "app_usb.h"
#include "cmd_cfg.h"

#if DEBUG_USB
uint32_t debug_usb;
#define DEBUG_USB_OUT(a) debug_usb = a
#else
#define DEBUG_USB_OUT(a)
#endif

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */
struct {
    const uint8_t *pDescr;
    uint16_t SetupReqLen;
    uint8_t DevConfig;
    uint8_t SetupReqCode;
} usb_data;

#define DevEP0SIZE  64

//  3.1 Requests---Abstract Control Model
#define DEF_SEND_ENCAPSULATED_COMMAND  0x00
#define DEF_GET_ENCAPSULATED_RESPONSE  0x01
#define DEF_SET_COMM_FEATURE           0x02
#define DEF_GET_COMM_FEATURE           0x03
#define DEF_CLEAR_COMM_FEATURE         0x04
#define DEF_SET_LINE_CODING            0x20   // Configures DTE rate, stop-bits, parity, and number-of-character
#define DEF_GET_LINE_CODING            0x21   // This request allows the host to find out the currently configured line coding.
//#define DEF_SET_CTL_LINE_STE         0X22   // This request generates RS-232/V.24 style control signals.
#define DEF_SET_CONTROL_LINE_STATE     0x22
#define DEF_SEND_BREAK                 0x23

#if USE_SET_LINE_CODING
//Line Code
 typedef struct __PACKED _LINE_CODE
{
  UINT32  BaudRate;   /* 波特率 */
  UINT8 StopBits;   /* 停止位计数，0：1停止位，1：1.5停止位，2：2停止位 */
  UINT8 ParityType;   /* 校验位，0：None，1：Odd，2：Even，3：Mark，4：Space */
  UINT8 DataBits;   /* 数据位计数：5，6，7，8，16 */
}LINE_CODE, *PLINE_CODE;

LINE_CODE Uart0Para;
#endif

// Device Descriptor
const uint8_t MyDevDescr[] = {
        0x12, // bLength
        0x01, // bDescriptorType: Device Descriptor
        0x00,0x02, // bcdUSB : USB Version 2.00
        0x02, // bDeviceClass : Communications and CDC Control
        0x00, // bDeviceSubClass
        0x00, // bDeviceProtocol : (No class specific protocol required
        MAX_PACKET_SIZE, // bMaxPacketSize0 : 64 bytes
        0x86, 0x1a, // idVendor : 0x1A86
        0x40, 0x80, // idProduct : 0x8040
        0x00, 0x30, // bcdDevice : 0x3000
        0x01, // iManufacturer: Индексное значение строкового дескриптора производителя
        0x02, // iProduct: Индексное значение дескриптора строки продукта
        0x03, // iSerialNumber: Индексное значение строкового дескриптора серийного номера
        0x01  // bNumConfigurations: 1 Количество возможных конфигураций
};
// Configuration Descriptor
const uint8_t MyCfgDescr[] = {
        // Configuration Descriptor
        0x09, // bLength : 9 bytes
        0x02, // bDescriptorType: Configuration Descriptor
        0x43+8,0x00, // wTotalLength
        0x02, // bNumInterfaces: 2 Interfaces
        0x01, // bConfigurationValue : Configuration 1
        0x00, // iConfiguration : No String Descriptor
        0x80, /* bmAttributes : 0x80
         D7: Reserved, set 1     : 0x01
         D6: Self Powered        : 0x00 (no)
         D5: Remote Wakeup       : 0x00 (no)
         D4..0: Reserved, set 0  : 0x00*/
        0x30, // MaxPower : 96 mA

        // ------------------- IAD Descriptor --------------------
        0x08, // bLength                  : 0x08 (8 bytes)
        0x0b, // bDescriptorType          : 0x0B
        0x00, // bFirstInterface          : 0x00
        0x02, // bInterfaceCount          : 0x02
        0x02, // bFunctionClass           : 0x02 (Communications and CDC Control)
        0x02, // bFunctionSubClass        : 0x02
        0x01, // bFunctionProtocol        : 0x01
        0x00, // iFunction                : 0x00 (No String Descriptor)

        // Interface Descriptor 0 (интерфейс CDC) дескриптор
        0x09, // bLength
        0x04, // bDescriptorType : Interface Descriptor
        0x00, // bInterfaceNumber
        0x00, // bAlternateSetting
        0x01, // bNumEndpoints : 1
        0x02, // bInterfaceClass : Communications and CDC Control
        0x02, // bInterfaceSubClass: Abstract Control Model
        0x01, // bInterfaceProtocol: AT Commands defined by ITU-T V.250 etc
        0x00, // iInterface : No String Descriptor

        // CDC Interface Descriptor
        0x05, // bFunctionLength
        0x24, // bDescriptorType : Interface
        0x00, // bDescriptorSubType : Header Functional Descriptor
        0x10,0x01, // bcdCDC: CDC Version 1.10

        // CDC Interface Descriptor
        0x05, // bFunctionLength
        0x24, // bDescriptorType : Interface
        0x01, // bDescriptorSubType : Call Management Functional Descriptor
        0x00, /* bmCapabilities : 0x01
           D7..2: 0x00 (Reserved)
           D1   : 0x00 (sends/receives call management information only over the Communication Class interface)
           D0   : 0x01 (handles call management itself)*/
        0x01, // bDataInterface

        // CDC Interface Descriptor
        0x04, // bFunctionLength
        0x24, // bDescriptorType : Interface
        0x02, // bDescriptorSubType : Abstract Control Management Functional Descriptor
        0x02, /* bmCapabilities : 0x02
           D7..4: 0x00 (Reserved)
           D3   : 0x00 (not supports the notification Network_Connection)
           D2   : 0x00 (not supports the request Send_Break)
           D1   : 0x01 (supports the request combination of Set_Line_Coding, Set_Control_Line_State, Get_Line_Coding, and the notification Serial_State)
           D0   : 0x00 (not supports the request combination of Set_Comm_Feature, Clear_Comm_Feature, and Get_Comm_Feature) */

        // CDC Interface Descriptor
        0x05, // bFunctionLength
        0x24, // bDescriptorType : Interface
        0x06, // bDescriptorSubType : Union Functional Descriptor
        0x00, // bControlInterface
        0x01, // bSubordinateInterface[0]

        //  Endpoint Descriptor Дескриптор конечной точки загрузки прерывания
        0x07, // bLength
        0x05, // bDescriptorType : Endpoint Descriptor
        0x84, // bEndpointAddress : Direction=IN EndpointID=4
        0x03, // bmAttributes : TransferType=Interrupt
        0x08,0x00, // wMaxPacketSize : 8 bytes
        0x0a, // bInterval : 10 ms

        // Interface Descriptor 1 (интерфейс данных) дескриптор
        0x09, // bLength
        0x04, // bDescriptorType : Interface Descriptor
        0x01, // bInterfaceNumber
        0x00, // bAlternateSetting
        0x02, // bNumEndpoints : 2 Endpoints
        0x0a, // bInterfaceClass : CDC-Data
        0x00, // bInterfaceSubClass
        0x00, // bInterfaceProtocol
        0x00, // iInterface : No String Descriptor

        // Endpoint Descriptor Дескрипторs конечной точки
        0x07, // bLength
        0x05, // bDescriptorType : Endpoint Descriptor
        0x01, // bEndpointAddress : Direction=OUT EndpointID=1
        0x02, // bmAttributes : TransferType=Bulk
        MAX_PACKET_SIZE,0x00, // wMaxPacketSize : 64 bytes
        0x00, // bInterval : ignored

        // Endpoint Descriptor  Дескрипторs конечной точки
        0x07, // bLength
        0x05, // bDescriptorType : Endpoint Descriptor
        0x81, // bEndpointAddress : Direction=IN EndpointID=1
        0x02, // bmAttributes : TransferType=Bulk
        MAX_PACKET_SIZE,0x00, // wMaxPacketSize : 64 bytes
        0x00, // bInterval : ignored

};
const uint8_t MyLangDescr[] = { 0x04, 0x03, 0x09, 0x04 };
const uint8_t MyManuInfo[] = { 0x0E, 0x03, 'w', 0, 'c', 0, 'h', 0, '.', 0, 'c', 0, 'n', 0 };
const uint8_t MyProdInfo[] = { 0x0C, 0x03, 'C', 0, 'H', 0, '5', 0, '8', 0, 'x', 0 };
#if USE_MAC_SER
uint8_t MySerInfo[] = { 0x0E, 0x03, '1', 0, '2', 0, '3', 0, '4', 0, '5', 0, '6', 0 };
#else
const uint8_t MySerInfo[] = { 0x0E, 0x03, '1', 0, '2', 0, '3', 0, '4', 0, '5', 0, '6', 0 };
#endif

const uint8_t Return1[2] = {0x31,0x00};
const uint8_t Return2[2] = {0xC3,0x00};
const uint8_t Return3[2] = {0x9F,0xEE};

/*********************************************************************
 * LOCAL VARIABLES
 */

/******** 用户自定义分配端点RAM ****************************************/
__attribute__((aligned(4)))  uint8_t EP0_Databuf[MAX_PACKET_SIZE + MAX_PACKET_SIZE + MAX_PACKET_SIZE];    //ep0(64)+ep4_out(64)+ep4_in(64)
__attribute__((aligned(4)))  uint8_t EP1_Databuf[MAX_PACKET_SIZE + MAX_PACKET_SIZE];    //ep1_out(64)+ep1_in(64)
//__attribute__((aligned(4)))  uint8_t EP2_Databuf[MAX_PACKET_SIZE + MAX_PACKET_SIZE];    //ep2_out(64)+ep2_in(64)
//__attribute__((aligned(4)))  uint8_t EP3_Databuf[MAX_PACKET_SIZE + MAX_PACKET_SIZE];    //ep3_out(64)+ep3_in(64)

#define pSetupRequest          ((PUSB_SETUP_REQ)EP0_Databuf)

volatile uint8_t app_cmd_len;
uint8_t app_cmd_buf[APP_RX_BUFFER_LENGTH];

/*********************************************************************
 * PUBLIC FUNCTIONS
 */
#if USE_MAC_SER
// "650d8172ab3c"
static void mak_to_ser(void) {
    static const char hex[] = "0123456789abcdef";
    uint8_t * p = MySerInfo;
    // GET_UNIQUE_ID(buf); -> "650d8172ab3c"
    FLASH_EEPROM_CMD( CMD_GET_ROM_INFO, ROM_CFG_MAC_ADDR, EP0_Databuf, 0 );
    *p++ = 0x0E;
    *p++ = 0x03;
    for(int i = 0; i < 3; i++)
    {
        *p++ = hex[EP0_Databuf[i] >> 4];
        *p++ = 0;
        *p++ = hex[EP0_Databuf[i] & 0x0f];
        *p++ = 0;
    }
}
#endif
/*********************************************************************
 * @fn      app_usb_init
 *
 * @brief   初始化usb
 *
 * @return  none
 */
void app_usb_init()
{
    R8_USB_CTRL = 0x00;
#if USE_MAC_SER
    mak_to_ser();
#endif
    R8_UEP4_1_MOD = RB_UEP4_RX_EN | RB_UEP4_TX_EN | RB_UEP1_RX_EN | RB_UEP1_TX_EN; // 端点4 OUT+IN,端点1 OUT+IN
//    R8_UEP2_3_MOD = 0; //RB_UEP2_RX_EN | RB_UEP2_TX_EN | RB_UEP3_RX_EN | RB_UEP3_TX_EN; // 端点2 OUT+IN,端点3 OUT+IN


    R16_UEP0_DMA = (UINT16)(UINT32)&EP0_Databuf[0];
    R16_UEP1_DMA = (UINT16)(UINT32)&EP1_Databuf[0];
//    R16_UEP2_DMA = (UINT16)(UINT32)&EP2_Databuf[0];
//    R16_UEP3_DMA = (UINT16)(UINT32)&EP3_Databuf[0];

    /* 端点0状态：OUT--ACK IN--NAK */
    R8_UEP0_CTRL = UEP_R_RES_NAK | UEP_T_RES_NAK;

    /* 端点1状态：OUT--ACK IN--NAK 自动翻转 */
    R8_UEP1_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK | RB_UEP_AUTO_TOG;

    /* 端点2状态：OUT--ACK IN--NAK 自动翻转 */
//    R8_UEP2_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK | RB_UEP_AUTO_TOG;

    /* 端点3状态：IN--NAK 自动翻转 */
//    R8_UEP3_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK | RB_UEP_AUTO_TOG;

    /* 端点4状态：IN--NAK 手动翻转 */
    R8_UEP4_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;

    R8_USB_DEV_AD = 0x00;
    R8_USB_CTRL = RB_UC_DEV_PU_EN | RB_UC_INT_BUSY | RB_UC_DMA_EN;
    R16_PIN_ANALOG_IE |= RB_PIN_USB_IE | RB_PIN_USB_DP_PU;
    R8_USB_INT_FG = 0xFF;
    R8_USB_INT_EN = RB_UIE_SUSPEND | RB_UIE_BUS_RST | RB_UIE_TRANSFER;

    //PFIC_EnableIRQ(USB_IRQn);

    R8_UDEV_CTRL = RB_UD_PD_DIS | RB_UD_PORT_EN;

    PFIC_EnableIRQ( USB_IRQn );
}

/*********************************************************************
 * @fn      DevEP1_OUT_Deal
 *
 * @brief   Endpoint 1 data processing
 *
 * @return  none
 */
void DevEP1_OUT_Deal( uint8_t l )
{ /* User customizable */
#if USE_TEST_ADC || USE_I2C_DEV
    if(app_cmd_len == 0 && l) {
        if(l > APP_RX_BUFFER_LENGTH)
            app_cmd_len = APP_RX_BUFFER_LENGTH;
        else
            app_cmd_len = l;
        memcpy(app_cmd_buf, EP1_Databuf, app_cmd_len);
        //tmos_set_event(ObserverTaskId, NEW_USBDATA_EVT);
    }
#else
    uint16_t len = l;
    if(app_drv_fifo_write(&app_tx_fifo, EP1_Databuf, (uint16_t *)&len) == APP_DRV_FIFO_RESULT_SUCCESS) {

    }
#endif
}

/*********************************************************************
 * @fn      DevEP2_OUT_Deal
 *
 * @brief   Endpoint 2 data processing
 *
 * @return  none
 */
void DevEP2_OUT_Deal( uint8_t l )
{ /* User customizable */
}

/*********************************************************************
 * @fn      DevEP3_OUT_Deal
 *
 * @brief   Endpoint 3 data processing
 *
 * @return  none
 */
void DevEP3_OUT_Deal( uint8_t l )
{ /* User customizable */
}

/*********************************************************************
 * @fn      DevEP4_OUT_Deal
 *
 * @brief   Endpoint 4 data processing
 *
 * @return  none
 */
void DevEP4_OUT_Deal( uint8_t l )
{ /* User customizable */
}

/*********************************************************************
 * @fn      USB_IRQHandler
 *
 * @brief   USB中断函数
 *
 * @return  none
 */
__INTERRUPT
__HIGH_CODE
void USB_IRQHandler( void )
{
  uint8_t len, chtype;
  uint8_t errflag = 0;

  uint8_t intflag = R8_USB_INT_FG;
  uint8_t intsta = R8_USB_INT_ST;

  if (intflag & RB_UIF_TRANSFER) {
      if (( intsta & MASK_UIS_TOKEN) != MASK_UIS_TOKEN)    // Non-idle
      {
          switch ( intsta & ( MASK_UIS_TOKEN | MASK_UIS_ENDP)) {
          // Analyze the operation token and endpoint number
          case UIS_TOKEN_IN | 1: // ep1
            R8_UEP1_CTRL = ( R8_UEP1_CTRL & (~MASK_UEP_T_RES)) | UEP_T_RES_NAK;
            break;
          case UIS_TOKEN_OUT | 1: // ep1
            if ( intsta & RB_UIS_TOG_OK) { // Out-of-sync packets will be discarded
              len = R8_USB_RX_LEN;
              DevEP1_OUT_Deal(len);
            }
            break;
          case UIS_TOKEN_IN: // ep0
              switch (usb_data.SetupReqCode) {
              case USB_GET_DESCRIPTOR:
                  len = usb_data.SetupReqLen >= DevEP0SIZE ? DevEP0SIZE : usb_data.SetupReqLen; // length of this transmission
                  memcpy( EP0_Databuf, usb_data.pDescr, len); /* Loading Upload Data */
                  usb_data.SetupReqLen -= len;
                  usb_data.pDescr += len;
                  R8_UEP0_T_LEN = len;
                  R8_UEP0_CTRL ^= RB_UEP_T_TOG; // Flip
                  break;
              case USB_SET_ADDRESS:
                  R8_USB_DEV_AD = ( R8_USB_DEV_AD & RB_UDA_GP_BIT) | usb_data.SetupReqLen;
                  R8_UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
                  break;
              default:
                  R8_UEP0_T_LEN = 0; // The status stage is completed and the interrupt is completed or the zero-length data packet is forced to upload to end the control transmission.
                  R8_UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
                  break;
              }
              break;
/*
          case UIS_TOKEN_OUT: // ep0
              len = R8_USB_RX_LEN;
              break;
*/

#if 0
          case UIS_TOKEN_OUT | 2: // ep2
              if ( intsta & RB_UIS_TOG_OK) { // Out-of-sync packets will be discarded
                  len = R8_USB_RX_LEN;
                  DevEP2_OUT_Deal(len);
              }
              break;

          case UIS_TOKEN_IN | 2: // ep2
              R8_UEP2_CTRL = ( R8_UEP2_CTRL & (~MASK_UEP_T_RES)) | UEP_T_RES_NAK;
              break;

          case UIS_TOKEN_OUT | 3: // ep3
              if ( intsta & RB_UIS_TOG_OK) { // Out-of-sync packets will be discarded
                  len = R8_USB_RX_LEN;
                  DevEP3_OUT_Deal(len);
              }
              break;

          case UIS_TOKEN_IN | 3: // ep3
              R8_UEP3_CTRL = ( R8_UEP3_CTRL & (~MASK_UEP_T_RES)) | UEP_T_RES_NAK;
              break;
#endif

          case UIS_TOKEN_OUT | 4: // ep4
              if ( intsta & RB_UIS_TOG_OK) {
                  R8_UEP4_CTRL ^= RB_UEP_R_TOG;
                  len = R8_USB_RX_LEN;
                  DevEP4_OUT_Deal(len);
              }
              break;

          case UIS_TOKEN_IN | 4: // ep4
              R8_UEP4_CTRL ^= RB_UEP_T_TOG;
              R8_UEP4_CTRL = ( R8_UEP4_CTRL & (~MASK_UEP_T_RES)) | UEP_T_RES_NAK;
              break;

          default:
              break;
          }
          R8_USB_INT_FG = RB_UIF_TRANSFER;
      }
      if ( intsta & RB_UIS_SETUP_ACT) { // Setup package processing
          usb_data.SetupReqLen = pSetupRequest->wLength;
          usb_data.SetupReqCode = pSetupRequest->bRequest;
          chtype = pSetupRequest->bRequestType;
          R8_UEP0_CTRL = RB_UEP_R_TOG | RB_UEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_NAK;
          len = 0;
          errflag = 0;
          if ((chtype & USB_REQ_TYP_MASK) != USB_REQ_TYP_STANDARD) {
              DEBUG_USB_OUT(0x1000000 | (EP0_Databuf[0] << 16) | (EP0_Databuf[1] << 8) | EP0_Databuf[2]);
              if(chtype == 0x21 // USB_REQ_TYP_CLASS
                  && usb_data.SetupReqCode == DEF_SET_CONTROL_LINE_STATE) { // DEF_SET_CONTROL_LINE_STATE:  /* SET_CONTROL_LINE_STATE */
                  DEBUG_USB_OUT(0x3000000 | (EP0_Databuf[0] << 16) | (EP0_Databuf[1] << 8) | EP0_Databuf[2]);
#if USE_TEST_ADC
                  AdcInit(EP0_Databuf[2] & (1<<0));   // bit0: DTR,  bit1: RTS
#endif

#if USE_I2C_DEV
                  app_cmd_buf[0] = 1;
                  app_cmd_buf[1] = CMD_I2C_PWR;
                  app_cmd_buf[2] = EP0_Databuf[2] & (1<<0);
                  app_cmd_len = 3;
#endif
              }
#if USE_SET_LINE_CODING
/*
              else if(chtype == 0x21
                  && usb_data.SetupReqCode == DEF_SET_LINE_CODING) {
                memcpy(&Uart0Para, &EP0_Databuf[2], sizeof(Uart0Para));
                @TODO
              }
*/
#endif
/*
              else if (chtype == 0xC0) { // vendor + IN
                  if (usb_data.SetupReqCode == DEF_VEN_GET_VER) { // 0x5F
                      pDescr = Return1;
                      len = sizeof(Return1);
                  } else if (usb_data.SetupReqCode == DEF_VEN_DEBUG_READ) { // 0x95
                      if ((pSetupRequest->wValue) == 0x18) {
                          usb_data.pDescr = Return2;
                          len = sizeof(Return2);
                      } else if ((pSetupRequest->wValue) == 0x06) {
                          usb_data.pDescr = Return3;
                          len = sizeof(Return3);
                      }
                  } else {
                      errflag = 0xFF;
                  }
                  memcpy(EP0_Databuf, usb_data.pDescr, len);
              } else {
                  len = 0;
              }
*/
          } else { /* Standard Request */
              switch (usb_data.SetupReqCode) {
              case USB_GET_DESCRIPTOR: {
                  switch ((( pSetupRequest->wValue) >> 8)) {
                  case USB_DESCR_TYP_DEVICE:
                      usb_data.pDescr = MyDevDescr;
                      len = sizeof(MyDevDescr);
                      break;

                  case USB_DESCR_TYP_CONFIG:
                      usb_data.pDescr = MyCfgDescr;
                      len = sizeof(MyCfgDescr);
                      break;

//                    case USB_DESCR_TYP_REPORT:
//                       break;

                  case USB_DESCR_TYP_STRING:
                      DEBUG_USB_OUT(0x2000000 | (EP0_Databuf[0] << 16) | (EP0_Databuf[1] << 8) | EP0_Databuf[2]);
                      switch (( pSetupRequest->wValue) & 0xff) {
                      case 0: // Lang
                          usb_data.pDescr = MyLangDescr;
                          len = MyLangDescr[0];
                          break;
                      case 1: // iManufacturer
                          usb_data.pDescr = MyManuInfo;
                          len = MyManuInfo[0];
                          break;
                      case 2: // MyProductInfo[0]; // iProduct
                          usb_data.pDescr = MyProdInfo;
                          len = MyProdInfo[0];
                          break;
                      case 3: // iSerialNumber
                          usb_data.pDescr = MySerInfo;
                          len = MySerInfo[0];
                          break;
                      default:
                          errflag = 0xFF;     // Unsupported string descriptor
                          break;
                      }
                      break;
                  default:
                      errflag = 0xff;
                      break;
                  }
                  if (usb_data.SetupReqLen > len)
                    usb_data.SetupReqLen = len; // actual total length to be uploaded
                  len = (usb_data.SetupReqLen >= DevEP0SIZE) ? DevEP0SIZE : usb_data.SetupReqLen;
                  memcpy(EP0_Databuf, usb_data.pDescr, len);
                  usb_data.pDescr += len;
              }
              break;

              case USB_SET_ADDRESS:
                  usb_data.SetupReqLen = ( pSetupRequest->wValue) & 0xff;
                  break;

              case USB_GET_CONFIGURATION:
                  EP0_Databuf[0] = usb_data.DevConfig;
                  if (usb_data.SetupReqLen > 1)
                      usb_data.SetupReqLen = 1;
                  break;

              case USB_SET_CONFIGURATION:
                  usb_data.DevConfig = ( pSetupRequest->wValue) & 0xff;
                  break;

              case USB_CLEAR_FEATURE:
                  if (( pSetupRequest->bRequestType & USB_REQ_RECIP_MASK)
                          == USB_REQ_RECIP_ENDP)    // 端点
                  {
                      switch (( pSetupRequest->wIndex) & 0xff) {
/*
                      case 0x82:
                          R8_UEP2_CTRL = ( R8_UEP2_CTRL
                                  & ~( RB_UEP_T_TOG | MASK_UEP_T_RES))
                                  | UEP_T_RES_NAK;
                          break;
                      case 0x02:
                          R8_UEP2_CTRL = ( R8_UEP2_CTRL
                                  & ~( RB_UEP_R_TOG | MASK_UEP_R_RES))
                                  | UEP_R_RES_ACK;
                          break;
*/
                      case 0x81:
                          R8_UEP1_CTRL = ( R8_UEP1_CTRL
                                  & ~( RB_UEP_T_TOG | MASK_UEP_T_RES))
                                  | UEP_T_RES_NAK;
                          break;
                      case 0x01:
                          R8_UEP1_CTRL = ( R8_UEP1_CTRL
                                  & ~( RB_UEP_R_TOG | MASK_UEP_R_RES))
                                  | UEP_R_RES_ACK;
                          break;
                      case 0x84:
                          R8_UEP4_CTRL = (R8_UEP4_CTRL
                              & (~ ( RB_UEP_T_TOG | MASK_UEP_T_RES )))
                              | UEP_T_RES_NAK;
                          break;
                      case 0x04:
                          R8_UEP4_CTRL = (R8_UEP4_CTRL
                              & (~ ( RB_UEP_R_TOG | MASK_UEP_R_RES )))
                              | UEP_R_RES_ACK;
                          break;
                      default:
                          errflag = 0xFF;              // Unsupported endpoint
                          break;
                      }
                  } else if (( pSetupRequest->bRequestType & USB_REQ_RECIP_MASK)
                      == USB_REQ_RECIP_DEVICE) {
                    R8_UEP1_CTRL = (R8_UEP1_CTRL
                        & (~ ( RB_UEP_T_TOG | MASK_UEP_T_RES ))) | UEP_T_RES_NAK;
//                  R8_UEP2_CTRL = (R8_UEP2_CTRL
//                      & (~ ( RB_UEP_T_TOG | MASK_UEP_T_RES ))) | UEP_T_RES_NAK;
//                  R8_UEP3_CTRL = (R8_UEP3_CTRL
//                      & (~ ( RB_UEP_T_TOG | MASK_UEP_T_RES ))) | UEP_T_RES_NAK;
                    R8_UEP4_CTRL = (R8_UEP4_CTRL
                        & (~ ( RB_UEP_T_TOG | MASK_UEP_T_RES ))) | UEP_T_RES_NAK;
                  } else
                      errflag = 0xFF;
                  break;

              case USB_GET_INTERFACE:
                  EP0_Databuf[0] = 0x00;
                  if (usb_data.SetupReqLen > 1)
                    usb_data.SetupReqLen = 1;
                  break;

              case USB_GET_STATUS:
                  EP0_Databuf[0] = 0x00;
                  EP0_Databuf[1] = 0x00;
                  if (usb_data.SetupReqLen > 2)
                    usb_data.SetupReqLen = 2;
                  break;

              default:
                  errflag = 0xff;
                  break;
              }
          }
          if (errflag == 0xff) { // Error or not supported
//                  SetupReqCode = 0xFF;
              R8_UEP0_CTRL = RB_UEP_R_TOG | RB_UEP_T_TOG | UEP_R_RES_STALL
                      | UEP_T_RES_STALL;    // STALL
          } else {
              if (chtype & 0x80) {    // Upload
                  len = (usb_data.SetupReqLen > DevEP0SIZE) ? DevEP0SIZE : usb_data.SetupReqLen;
                  usb_data.SetupReqLen -= len;
              } else
                  len = 0;        // Download
              R8_UEP0_T_LEN = len;
              R8_UEP0_CTRL = RB_UEP_R_TOG | RB_UEP_T_TOG | UEP_R_RES_ACK
                      | UEP_T_RES_ACK;    // default data packet is DATA1
          }
          R8_USB_INT_FG = RB_UIF_TRANSFER;
      }
  } else if (intflag & RB_UIF_BUS_RST) {   // USB bus reset
      R8_USB_DEV_AD = 0;
      R8_UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
      R8_UEP1_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK | RB_UEP_AUTO_TOG;
//    R8_UEP2_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK | RB_UEP_AUTO_TOG;
//    R8_UEP3_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK | RB_UEP_AUTO_TOG;
      R8_UEP4_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
      R8_USB_INT_FG = RB_UIF_BUS_RST;
  } else if (intflag & RB_UIF_SUSPEND) { // USB bus suspend/wake completed
      if ( R8_USB_MIS_ST & RB_UMS_SUSPEND) { // Suspend
      } else { // wake
      }
      R8_USB_INT_FG = RB_UIF_SUSPEND;
  } else {
      R8_USB_INT_FG = intflag;
  }
}


/*********************************************************************
*********************************************************************/
