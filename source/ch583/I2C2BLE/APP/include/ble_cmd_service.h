/**
 * File Name          : BLE_CMD_SERVICE.h
 */

#ifndef _BLE_CMD_SERVICE_H
#define _BLE_CMD_SERVICE_H

#ifdef __cplusplus
extern "C" {
#endif

//#include "att.h"
#include "stdint.h"

//#define BLE_CMD_RX_BUFF_SIZE    1

#define CMDPROFILE_SERV_UUID    0xfff0
#define CMDPROFILE_CHAR_RX_UUID    0xfff2
#define CMDPROFILE_CHAR_TX_UUID    0xfff1

/*********************************************************************
 * API FUNCTIONS
 */

/*
 * ble_cmd_AddService- Initializes the raw pass GATT Profile service by registering
 *          GATT attributes with the GATT server.
 *
 * @param   services - services to add. This is a bit map and can
 *                     contain more than one service.
 */

extern bStatus_t ble_cmd_add_service(void);

extern uint8_t ble_cmd_notify_is_ready(uint16_t connHandle);
extern uint16_t ble_cmd_notify(uint16_t connHandle);
/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* _BLE_CMD_SERVICE_H */
