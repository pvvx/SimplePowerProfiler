/*
 * i2cbus.c
 *
 *  Created on: 7 янв. 2023 г.
 *      Author: pvvx
 */
#include "main_config.h"
#if (USE_I2C_DEV)
#include "bl702_common.h"
#include "board.h"
#include "i2cbus.h"
#include "bflb_clock.h"
#include "hardware/i2c_reg.h"
#include "bflb_i2c.h"

struct bflb_device_s *i2c0;

int I2CBusTrBusy(uint32_t timeout_us) {
	if(!i2c0) return -1;
	uint32_t reg_base = i2c0->reg_base;
	uint32_t start_time = bflb_mtimer_get_time_us();
    while ((getreg32(reg_base + I2C_BUS_BUSY_OFFSET) & I2C_STS_I2C_BUS_BUSY)
    		|| !(getreg32(reg_base + I2C_INT_STS_OFFSET) & I2C_END_INT)) {
        if ((bflb_mtimer_get_time_us() - start_time) > timeout_us) {
/*
        	uint32_t regval = getreg32(reg_base + I2C_BUS_BUSY_OFFSET);
            regval |= I2C_CR_I2C_BUS_BUSY_CLR;
            putreg32(regval, reg_base + I2C_BUS_BUSY_OFFSET);
*/
            return -ETIMEDOUT;
        }
    }
    return 0;
}

int I2CBusTrEnd(void) {
	int ret = -1;
	if(!i2c0) return ret;
	uint32_t reg_base = i2c0->reg_base;
	ret = (getreg32(reg_base + I2C_INT_STS_OFFSET) & I2C_NAK_INT) == 0;
	uint32_t regval = getreg32(reg_base + I2C_CONFIG_OFFSET);
    regval &= ~I2C_CR_I2C_M_EN;
    putreg32(regval, reg_base + I2C_CONFIG_OFFSET);
    /* Clear I2C fifo */
    regval = getreg32(reg_base + I2C_FIFO_CONFIG_0_OFFSET);
    regval |= I2C_TX_FIFO_CLR;
    regval |= I2C_RX_FIFO_CLR;
    putreg32(regval, reg_base + I2C_FIFO_CONFIG_0_OFFSET);
    /* Clear I2C interrupt status */
    regval = getreg32(reg_base + I2C_INT_STS_OFFSET);
    regval |= I2C_CR_I2C_END_CLR;
    regval |= I2C_CR_I2C_NAK_CLR;
    regval |= I2C_CR_I2C_ARB_CLR;
    putreg32(regval, reg_base + I2C_INT_STS_OFFSET);
	return ret;
}


// Return NAK
int I2CBusWriteWord(unsigned char i2c_addr, unsigned char reg_addr, unsigned short reg_data)
{
	int ret = -1;
	if(!i2c0) return ret;

    uint32_t reg_base = i2c0->reg_base;

    uint32_t regval = getreg32(reg_base + I2C_CONFIG_OFFSET);

    regval &= ~I2C_CR_I2C_PKT_DIR; // Transfer direction of the packet: 0 - write

    regval |= I2C_CR_I2C_SUB_ADDR_EN;
    regval &= ~I2C_CR_I2C_SUB_ADDR_BC_MASK;
    //regval |= 0 << I2C_CR_I2C_SUB_ADDR_BC_SHIFT);

    regval &= ~I2C_CR_I2C_SLV_ADDR_MASK;
    regval |= (i2c_addr << (I2C_CR_I2C_SLV_ADDR_SHIFT - 1)) & I2C_CR_I2C_SLV_ADDR_MASK;

    regval &= ~I2C_CR_I2C_PKT_LEN_MASK;
    regval |= 1 << I2C_CR_I2C_PKT_LEN_SHIFT;

    putreg32(reg_addr, reg_base + I2C_SUB_ADDR_OFFSET);
    uint32_t tmp = (reg_data << 8) |  (reg_data >> 8);
    putreg32(tmp, reg_base + I2C_FIFO_WDATA_OFFSET);
    regval |= I2C_CR_I2C_M_EN; // Enable signal of I2C Master function
    putreg32(regval, reg_base + I2C_CONFIG_OFFSET);

    I2CBusTrBusy(4000); // 4 ms
    return I2CBusTrEnd();
}

int I2CBusReadWord(unsigned char i2c_addr, unsigned char reg_addr, void *preg_data)
{
	int ret = -1;
	if(!i2c0) return ret;

	uint8_t * p = (uint8_t *) preg_data;

    uint32_t reg_base = i2c0->reg_base;

    uint32_t regval = getreg32(reg_base + I2C_CONFIG_OFFSET);

    regval |= I2C_CR_I2C_PKT_DIR; // Transfer direction of the packet: 1 - read

    regval |= I2C_CR_I2C_SUB_ADDR_EN;
    regval &= ~I2C_CR_I2C_SUB_ADDR_BC_MASK;
    //regval |= 0 << I2C_CR_I2C_SUB_ADDR_BC_SHIFT);

    regval &= ~I2C_CR_I2C_SLV_ADDR_MASK;
    regval |= (i2c_addr << (I2C_CR_I2C_SLV_ADDR_SHIFT - 1)) & I2C_CR_I2C_SLV_ADDR_MASK;

    regval &= ~I2C_CR_I2C_PKT_LEN_MASK;
    regval |= 1 << I2C_CR_I2C_PKT_LEN_SHIFT;

    putreg32(reg_addr, reg_base + I2C_SUB_ADDR_OFFSET);

    regval |= I2C_CR_I2C_M_EN;
    putreg32(regval, reg_base + I2C_CONFIG_OFFSET);

    I2CBusTrBusy(4000); // 4 ms

    regval = getreg32(reg_base + I2C_FIFO_RDATA_OFFSET);
    p[0] = (uint8_t)(regval >> 8);
    p[1] = (uint8_t)regval;

	return I2CBusTrEnd();
}

void I2CBusDeInit(void) {
	if(!i2c0) return;
	I2CBusTrEnd();
	uint32_t reg_base = i2c0->reg_base;
	uint32_t regval = getreg32(reg_base + I2C_INT_STS_OFFSET);

    regval &= ~(I2C_CR_I2C_END_MASK |
               I2C_CR_I2C_TXF_MASK |
               I2C_CR_I2C_RXF_MASK |
               I2C_CR_I2C_NAK_MASK |
               I2C_CR_I2C_ARB_MASK |
               I2C_CR_I2C_FER_MASK);

    putreg32(regval, reg_base + I2C_INT_STS_OFFSET);
    // PERIPHERAL_CLOCK_I2C0_DISABLE();
}

static void _I2CBusInit(uint32_t freq, unsigned int clock_stretching) {
	uint32_t regval;
    uint32_t reg_base = i2c0->reg_base;
	printf("I2C: Set CLK %i\r\n", freq);
#if 0
	bflb_i2c_init(i2c0, freq);
#else
    regval = getreg32(reg_base + I2C_INT_STS_OFFSET);
    regval |= (I2C_CR_I2C_END_MASK |
               I2C_CR_I2C_TXF_MASK |
               I2C_CR_I2C_RXF_MASK |
               I2C_CR_I2C_NAK_MASK |
               I2C_CR_I2C_ARB_MASK |
               I2C_CR_I2C_FER_MASK);
    putreg32(regval, reg_base + I2C_INT_STS_OFFSET);

    uint32_t phase = bflb_clk_get_peripheral_clock(BFLB_DEVICE_TYPE_I2C, i2c0->idx) / (freq * 4) - 1;

    regval = phase << I2C_CR_I2C_PRD_S_PH_0_SHIFT;
    regval |= phase << I2C_CR_I2C_PRD_S_PH_1_SHIFT;
    regval |= phase << I2C_CR_I2C_PRD_S_PH_2_SHIFT;
    regval |= phase << I2C_CR_I2C_PRD_S_PH_3_SHIFT;

    putreg32(regval, reg_base + I2C_PRD_START_OFFSET);
    putreg32(regval, reg_base + I2C_PRD_STOP_OFFSET);
    putreg32(regval, reg_base + I2C_PRD_DATA_OFFSET);
#endif
    regval = getreg32(reg_base + I2C_CONFIG_OFFSET);
	if(clock_stretching)
		regval &= ~I2C_CR_I2C_SCL_SYNC_EN;
	else
		regval &= ~I2C_CR_I2C_SCL_SYNC_EN;
	putreg32(regval, reg_base + I2C_CONFIG_OFFSET);
}

void I2CBusInit(unsigned int clk_khz, unsigned int clock_stretching) {
#ifndef I2C_DEV_POWER
	board_i2c0_gpio_init();
#endif
	i2c0 = bflb_device_get_by_name("i2c0");
	if(i2c0) {
		// PERIPHERAL_CLOCK_I2C0_ENABLE();
		I2CBusTrEnd();
		_I2CBusInit(clk_khz * 1000, clock_stretching);
	}
}

#endif // USE_I2C_DEV
