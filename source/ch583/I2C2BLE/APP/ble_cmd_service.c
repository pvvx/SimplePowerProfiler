/********************************** (C) COPYRIGHT *******************************
 * File Name          : ble_cmd_service.c
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
#include "cmd_cfg.h"
#include "ble_cmd_service.h"
#include "peripheral.h"
#include "app_drv_fifo.h"


/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

#define SERVAPP_NUM_ATTR_SUPPORTED    7

#define RAWPASS_TX_VALUE_HANDLE       2
#define RAWPASS_RX_VALUE_HANDLE       5
/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

// ble_usb GATT Profile Service UUID
const uint8_t ble_cmd_ServiceUUID[ATT_BT_UUID_SIZE] =
    {LO_UINT16(CMDPROFILE_SERV_UUID), HI_UINT16(CMDPROFILE_SERV_UUID)};

// Characteristic rx uuid
const uint8_t ble_cmd_RxCharUUID[ATT_BT_UUID_SIZE] =
    {LO_UINT16(CMDPROFILE_CHAR_RX_UUID), HI_UINT16(CMDPROFILE_CHAR_RX_UUID)};

// Characteristic tx uuid
const uint8_t ble_cmd_TxCharUUID[ATT_BT_UUID_SIZE] =
    {LO_UINT16(CMDPROFILE_CHAR_TX_UUID), HI_UINT16(CMDPROFILE_CHAR_TX_UUID)};

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */

/*********************************************************************
 * Profile Attributes - variables
 */

// Profile Service attribute
static const gattAttrType_t ble_cmd_Service = {ATT_BT_UUID_SIZE, ble_cmd_ServiceUUID};

// Profile Characteristic 1 Properties
static uint8_t ble_cmd_RxCharProps = GATT_PROP_WRITE_NO_RSP | GATT_PROP_WRITE;

// Characteristic 1 Value
//static uint8_t ble_cmd_RxCharValue[BLE_CMD_RX_BUFF_SIZE];
static uint8_t ble_cmd_RxCharValue;

// Profile Characteristic 2 Properties
static uint8_t ble_cmd_TxCharProps = GATT_PROP_NOTIFY; // | GATT_PROP_INDICATE;

// Characteristic 2 Value
static uint8_t ble_cmd_TxCharValue;

// Simple Profile Characteristic 2 User Description
static gattCharCfg_t ble_cmd_TxCCCD[PERIPHERAL_MAX_CONNECTION];

/*********************************************************************
 * Profile Attributes - Table
 */

static gattAttribute_t ble_cmd_ProfileAttrTbl[] = {
    // Simple Profile Service
    {
        {ATT_BT_UUID_SIZE, primaryServiceUUID}, /* type */
        GATT_PERMIT_READ,                       /* permissions */
        0,                                      /* handle */
        (uint8_t *)&ble_cmd_Service             /* pValue */
    },

    // Characteristic 2 Declaration
    {
        {ATT_BT_UUID_SIZE, characterUUID},
        GATT_PERMIT_READ,
        0,
        &ble_cmd_TxCharProps},

    // Characteristic Value 2
    {
        {ATT_BT_UUID_SIZE, ble_cmd_TxCharUUID},
        0,
        0,
        &ble_cmd_TxCharValue},

    // Characteristic 2 User Description
    {
        {ATT_BT_UUID_SIZE, clientCharCfgUUID},
        GATT_PERMIT_READ | GATT_PERMIT_WRITE,
        0,
        (uint8_t *)ble_cmd_TxCCCD},

    // Characteristic 1 Declaration
    {
        {ATT_BT_UUID_SIZE, characterUUID},
        GATT_PERMIT_READ,
        0,
        &ble_cmd_RxCharProps},

    // Characteristic Value 1
    {
        {ATT_BT_UUID_SIZE, ble_cmd_RxCharUUID},
        GATT_PERMIT_WRITE,
        0,
        &ble_cmd_RxCharValue},

};

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static bStatus_t ble_cmd_ReadAttrCB(uint16_t connHandle, gattAttribute_t *pAttr,
                                     uint8_t *pValue, uint16_t *pLen, uint16_t offset, uint16_t maxLen, uint8_t method);
static bStatus_t ble_cmd_WriteAttrCB(uint16_t connHandle, gattAttribute_t *pAttr,
                                      uint8_t *pValue, uint16_t len, uint16_t offset, uint8_t method);

static void ble_cmd_HandleConnStatusCB(uint16_t connHandle, uint8_t changeType);

/*********************************************************************
 * PROFILE CALLBACKS
 */
// Simple Profile Service Callbacks
gattServiceCBs_t ble_cmd_ProfileCBs = {
    ble_cmd_ReadAttrCB,  // Read callback function pointer
    ble_cmd_WriteAttrCB, // Write callback function pointer
    NULL                 // Authorization callback function pointer
};

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      ble_cmd_AddService
 *
 * @brief   Initializes the Simple Profile service by registering
 *          GATT attributes with the GATT server.
 *
 * @param   services - services to add. This is a bit map and can
 *                     contain more than one service.
 *
 * @return  Success or Failure
 */
bStatus_t ble_cmd_add_service(void)
{
    uint8_t status = SUCCESS;

    GATTServApp_InitCharCfg(INVALID_CONNHANDLE, ble_cmd_TxCCCD);
    // Register with Link DB to receive link status change callback
    linkDB_Register(ble_cmd_HandleConnStatusCB);

    //    ble_cmd_TxCCCD.connHandle = INVALID_CONNHANDLE;
    //    ble_cmd_TxCCCD.value = 0;
    // Register GATT attribute list and CBs with GATT Server App
    status = GATTServApp_RegisterService(ble_cmd_ProfileAttrTbl,
                                         GATT_NUM_ATTRS(ble_cmd_ProfileAttrTbl),
                                         GATT_MAX_ENCRYPT_KEY_SIZE,
                                         &ble_cmd_ProfileCBs);
    if(status != SUCCESS)
        PRINT("Add BLE CMD service failed!\n");

    return (status);
}

/*********************************************************************
 * @fn          ble_cmd_ReadAttrCB
 *
 * @brief       Read an attribute.
 *
 * @param       connHandle - connection message was received on
 * @param       pAttr - pointer to attribute
 * @param       pValue - pointer to data to be read
 * @param       pLen - length of data to be read
 * @param       offset - offset of the first octet to be read
 * @param       maxLen - maximum length of data to be read
 *
 * @return      Success or Failure
 */
static bStatus_t ble_cmd_ReadAttrCB(uint16_t connHandle, gattAttribute_t *pAttr,
                                     uint8_t *pValue, uint16_t *pLen, uint16_t offset, uint16_t maxLen, uint8_t method)
{
    bStatus_t status = SUCCESS;
    PRINT("ReadAttrCB\n");

    // Make sure it's not a blob operation (no attributes in the profile are long)
    if(pAttr->type.len == ATT_BT_UUID_SIZE)
    {
        // 16-bit UUID
        uint16_t uuid = BUILD_UINT16(pAttr->type.uuid[0], pAttr->type.uuid[1]);
        if(uuid == GATT_CLIENT_CHAR_CFG_UUID)
        {
            *pLen = 2;
            tmos_memcpy(pValue, pAttr->pValue, 2);
        }
    }
    return (status);
}

/*********************************************************************
 * @fn      simpleProfile_WriteAttrCB
 *
 * @brief   Validate attribute data prior to a write operation
 *
 * @param   connHandle - connection message was received on
 * @param   pAttr - pointer to attribute
 * @param   pValue - pointer to data to be written
 * @param   len - length of data
 * @param   offset - offset of the first octet to be written
 *
 * @return  Success or Failure
 */

static bStatus_t ble_cmd_WriteAttrCB(uint16_t connHandle, gattAttribute_t *pAttr,
                                      uint8_t *pValue, uint16_t len, uint16_t offset, uint8_t method)
{
    bStatus_t status = SUCCESS;
    //uint8_t notifyApp = 0xFF;
    // If attribute permissions require authorization to write, return error
    if(gattPermitAuthorWrite(pAttr->permissions))
    {
        // Insufficient authorization
        return (ATT_ERR_INSUFFICIENT_AUTHOR);
    }

    if(pAttr->type.len == ATT_BT_UUID_SIZE)
    {
        // 16-bit UUID
        uint16_t uuid = BUILD_UINT16(pAttr->type.uuid[0], pAttr->type.uuid[1]);
        if(uuid == GATT_CLIENT_CHAR_CFG_UUID)
        {
            status = GATTServApp_ProcessCCCWriteReq(connHandle, pAttr, pValue, len,
                                                    offset, GATT_CLIENT_CFG_NOTIFY);
            if(status == SUCCESS) {
                if(pValue[0] | pValue[1]) {
                    if(!ble_enable) {
                        ble_enable = 1;
                        app_drv_fifo_flush(&app_tx_fifo);
#if 0 // USE_I2C_DEV
                        app_cmd_buf[0] = 1; // data size
                        app_cmd_buf[1] = CMD_I2C_PWR;
                        app_cmd_buf[2] = 1;
                        app_cmd_len = 3; // cmd size
                        tmos_set_event(Peripheral_TaskID, SBP_PROCESS_INPDATA_EVT);
#endif

                    }
                } else
                    ble_enable = 0;
            }
        }

        //  UUID
        if(pAttr->handle == ble_cmd_ProfileAttrTbl[RAWPASS_RX_VALUE_HANDLE].handle)
        {
            //PRINT("BLE RX DATA len:%d\n", len);
            if(app_cmd_len == 0 && len) {
                if(len > sizeof(app_cmd_buf))
                    app_cmd_len = sizeof(app_cmd_buf);
                else
                    app_cmd_len = len;
                memcpy(app_cmd_buf, pValue, app_cmd_len);

                tmos_set_event(Peripheral_TaskID, SBP_PROCESS_INPDATA_EVT);
            }
        }
    }
    return (status);
}

/*********************************************************************
 * @fn          ble_cmd_HandleConnStatusCB
 *
 * @brief       ble_usb link status change handler function.
 *
 * @param       connHandle - connection handle
 * @param       changeType - type of change
 *
 * @return      none
 */
static void ble_cmd_HandleConnStatusCB(uint16_t connHandle, uint8_t changeType)
{
    // Make sure this is not loopback connection
    if(connHandle != LOOPBACK_CONNHANDLE)
    {
        // Reset Client Char Config if connection has dropped
        if((changeType == LINKDB_STATUS_UPDATE_REMOVED) ||
           ((changeType == LINKDB_STATUS_UPDATE_STATEFLAGS) &&
            (!linkDB_Up(connHandle))))
        {
            //ble_cmd_TxCCCD[0].value = 0;
            GATTServApp_InitCharCfg(connHandle, ble_cmd_TxCCCD);
        }
    }
}

uint8_t ble_cmd_notify_is_ready(uint16_t connHandle)
{
    return (GATT_CLIENT_CFG_NOTIFY == GATTServApp_ReadCharCfg(connHandle, ble_cmd_TxCCCD));
}
/*********************************************************************
 * @fn          ble_cmd_notify
 *
 * @brief
 *
 * @param       connHandle - connection handle
 *
 * @return      Success or Failure
 */
uint16_t ble_cmd_notify(uint16_t connHandle){
    static attHandleValueInd_t noti;
    bStatus_t ret;
    uint16_t len = app_drv_fifo_length(&app_tx_fifo);
    if(len) {
        if(len > peripheralMTU - 3)
            len = peripheralMTU - 3;
        noti.len = len;
        noti.pValue = GATT_bm_alloc(connHandle, ATT_HANDLE_VALUE_NOTI, noti.len, NULL, 0);
        if(noti.pValue == NULL) {
            PRINT("Notify: Memory allocation error!\n");
            return 0;
        }
        app_drv_fifo_rd(&app_tx_fifo, noti.pValue, len);
        // Set the handle
        noti.handle = ble_cmd_ProfileAttrTbl[RAWPASS_TX_VALUE_HANDLE].handle;
        // Send the Indication
        ret = GATT_Indication(connHandle, &noti, FALSE, Peripheral_TaskID);
        //ret = GATT_Notification(connHandle, &noti, FALSE);
        GATT_bm_free((gattMsg_t *)&noti, ATT_HANDLE_VALUE_NOTI);
        if(ret != SUCCESS) {
            PRINT("Notify: Err %d!\n", ret);
            return 0;
        }
        app_drv_fifo_rd_del(&app_tx_fifo, len);
    }
    return len;
}
/*********************************************************************
*********************************************************************/
