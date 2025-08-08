/*
 * ina229.h
 *
 *  Created on: 3 дек. 2022 г.
 *      Author: pvvx
 */

#ifndef INA229_H_
#define INA229_H_

//#include "cmd_cfg.h"

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
#define INA229_RA_CONFIG 0x0

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
#define INA229_RA_ADC_CONFIG 0x01

/* DIAG_ALRT Register
 bit14: Setting this bit high configures the Alert pin to be asserted when
the Conversion Ready Flag (bit 1) is asserted, indicating that a
conversion cycle has completed.
0h = Disable conversion ready flag on ALERT pin
1h = Enables conversion ready flag on ALERT pin
 bit12: Alert Polarity bit sets the Alert pin polarity.
0h = Normal (Active-low, open-drain)
1h = Inverted (active-high, open-drain )  */
#define INA229_RA_DIAG_ALRT 0x0b // Diagnostic flags and Alert

#define INA229_RA_VSHUNT 0x04 // Shunt Voltage Measurement
#define INA229_RA_VBUS 	 0x05 // Bus Voltage Measurement
#define INA229_RA_MAN_ID 0x3e // Manufacturer ID = 0x5449
#define INA229_RA_DEV_ID 0x3f // Device ID = 0x2291


typedef struct {
	int inited;		// инициализирована
	int buf_enable;	// работает, набор данных в буфер
	int chnls;		// рабочие каналы: bit0 - Shunt, bit1 - Vbus
	int shunt; 		// шунт
	uint16_t freq; 	// текушая частота опроса SPI/I2C
	uint16_t r_shunt;
	uint16_t r_alert;
	uint16_t r_config;
} cfg_ina_t;

extern cfg_ina_t cfg_ina;

int ina229_init(void);
int ina229_start(dev_adc_cfg_t * cfg);
int ina229_stop(void);
void ina229_wakeup(void);
void ina229_sleep(void);

void task_INA229(void);

#endif /* INA229_H_ */
