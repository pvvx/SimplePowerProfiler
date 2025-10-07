/*
 * utils.c
 *
 *  Created on: Aug 6, 2025
 *      Author: pvvx
 */
#include "common.h"

void sleep_tick(uint32_t tick) {
    uint32_t tt = GetSysTickCnt();
    while(GetSysTickCnt() - tt < tick) ;
}
