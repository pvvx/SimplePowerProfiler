/*
 * adc_dma.h
 *
 *  Created on: 17 нояб. 2022 г.
 *      Author: pvvx
 */

#ifndef ADC_DMA_H_
#define ADC_DMA_H_
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "bflb_adc.h"
#include "ring_buffer.h"
#include "cmd_cfg.h"

#define ADC_DATA_BLK_CNT	(63-2) // (127-2) // (31-2)

extern dev_adc_cfg_t cfg_adc;

void adc_dma_init(dev_adc_cfg_t *cadc);

void ADC_Start(dev_adc_cfg_t *cadc);
void ADC_Stop(void);

#endif /* ADC_DMA_H_ */
