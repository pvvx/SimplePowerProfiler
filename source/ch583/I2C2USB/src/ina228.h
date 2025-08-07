/*
 * ina228.h
 *
 *  Created on: 3 дек. 2022 г.
 *      Author: pvvx
 */

#ifndef INA228_H_
#define INA228_H_

//#include "cmd_cfg.h"

#define I2C_ADDR_INA228  0x80

/*CONFIG Register
 bit15:
 Reset Bit. Setting this bit to '1' generates a system reset that is the
same as power-on reset.
Resets all registers to default values.
0h = Normal Operation
1h = System Reset sets registers to default values
This bit self-clears.
 bit4: Shunt full scale range selection across IN+ and IN–.
0h = ±163.84 mV
1h = ± 40.96 mV */
#define INA228_RA_CONFIG 0x0

/* ADC_CONFIG Register
bit15-12: The user can set the MODE bits for continuous or triggered mode on
bus voltage, shunt voltage or temperature measurement.
0h = Shutdown
1h = Triggered bus voltage, single shot
2h = Triggered shunt voltage, single shot
3h = Triggered shunt voltage and bus voltage, single shot
4h = Triggered temperature, single shot
5h = Triggered temperature and bus voltage, single shot
6h = Triggered temperature and shunt voltage, single shot
7h = Triggered bus voltage, shunt voltage and temperature, single shot
8h = Shutdown
9h = Continuous bus voltage only
Ah = Continuous shunt voltage only
Bh = Continuous shunt and bus voltage
Ch = Continuous temperature only
Dh = Continuous bus voltage and temperature
Eh = Continuous temperature and shunt voltage
Fh = Continuous bus voltage, shunt voltage and temperature


bit9-11: Sets the conversion time of the bus voltage measurement:
0h = 50 μs
1h = 84 μs
2h = 150 μs
3h = 280 μs
4h = 540 μs
5h = 1052 μs
6h = 2074 μs
7h = 4120 μs

bit8-6: Sets the conversion time of the shunt voltage measurement:
0h = 50 μs
1h = 84 μs
2h = 150 μs
3h = 280 μs
4h = 540 μs
5h = 1052 μs
6h = 2074 μs
7h = 4120 μs

bit5-3: Sets the conversion time of the temperature measurement:
0h = 50 μs
1h = 84 μs
2h = 150 μs
3h = 280 μs
4h = 540 μs
5h = 1052 μs
6h = 2074 μs
7h = 4120 μs

 bit2-0: Selects ADC sample averaging count. The averaging setting applies
to all active inputs.
When >0h, the output registers are updated after the averaging has completed.
0h = 1
1h = 4
2h = 16
3h = 64
4h = 128
5h = 256
6h = 512
7h = 1024 */
#define INA228_RA_ADC_CONFIG 0x01

/* DIAG_ALRT Register
 bit14: Setting this bit high configures the Alert pin to be asserted when
the Conversion Ready Flag (bit 1) is asserted, indicating that a
conversion cycle has completed.
0h = Disable conversion ready flag on ALERT pin
1h = Enables conversion ready flag on ALERT pin
 bit12: Alert Polarity bit sets the Alert pin polarity.
0h = Normal (Active-low, open-drain)
1h = Inverted (active-high, open-drain )  */
#define INA228_RA_DIAG_ALRT 0x0b // Diagnostic flags and Alert

#define INA228_RA_VSHUNT 0x04 // Shunt Voltage Measurement
#define INA228_RA_VBUS 	 0x05 // Bus Voltage Measurement
#define INA228_RA_MAN_ID 0x3e // Manufacturer ID = 0x5449
#define INA228_RA_DEV_ID 0x3f // Device ID = 0x2291


typedef struct __attribute__((packed)) _dev_ina228_cfg_t {
	uint8_t enable;	// 1 = start, 0 = stop
	uint8_t chnl; 	// Channels: bit0 Vbus, bit1 Shunt
	uint16_t smpr; 	// Sample Rate (Hz)
	uint8_t  sh; 	// Shunt 0 = 40.96 mV, 1 = 163.84 mV
} dev_ina228_cfg_t; // [6]
extern dev_ina228_cfg_t cfg_ina228; // store in eep ?


int ina228_start(dev_ina228_cfg_t * cfg);
int ina228_stop(void);


#endif /* INA228_H_ */
