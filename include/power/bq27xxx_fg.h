/*
 * Copyright (C) 2013 Samsung Electronics
 * Piotr Wilczek <p.wilczek@samsung.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */
#include <linux/delay.h>

#ifndef __BQ27XXX_FG_H_
#define __BQ27XXX_FG_H_

/* BQ27441 REGISTERS */
#define BQ27XXX_FG_I2C_ADDR  (0x55)


/*STANDARD COMMAND REGISTER : TO READ OUT BATTERY DETAILS*/
#define BQ27xxx_CONTROL			(0x00)
#define BQ27xxx_DESIGN_CAPACITY		(0x3c) //DCAP
#define BQ27xxx_VOLTAGE_NOW		(0x04) //Volt
#define BQ27xxx_TEMPERATURE            	(0x02)
#define BQ27xxx_NOMINAL_AVG_CURRENT	(0x08) //NAC
#define BQ27xxx_FULL_CHARGE_CAPACITY	(0x0E) //FCC 
#define BQ27xxx_STATE_OF_CHARGE		(0x1c) //SOC
#define BQ27xxx_AVERAGE_CURRENT		(0x10) //AI
#define BQ27xxx_FLAGS			(0x06)

/*SUB CONTROL COMMAND : TO READ VENDOR , FG seal/unseal, config mode set ETC*/
#define BQ27xxx_CONTROL_STATUS         	(0x0000)
#define BQ27xxx_DEVICE_TYPE            	(0x0001)
#define BQ27xxx_UNSEAL_KEY             	(0x8000)
#define BQ27xxx_SET_CFGUPDATE          	(0x0013)

#define BQ27xxx_SOFT_RESET             	(0x0042)
#define BQ27xxx_SEALED_KEY		(0x0020)
/*BLCOK COMMAND */


/*FLAG BITS*/ 
#define BQ27XXX_FLAG_DSC		(1<<0) //if set, Discharging detected
#define BQ27XXX_FLAG_SOCF		(1<<1)
#define BQ27XXX_FLAG_SOC1		(1<<2)
#define BQ27XXX_FLAG_BAT_DET		(1<<3) //If set, Battery insertion detected
#define BQ27xxx_FLAGS_CFGUPMODE		(1<<4)
#define BQ27xxx_FLAGS_ITPOR		(1<<5) //If set, reload the configuration
#define BQ27XXX_FLAG_FC			(1<<9) //if set, Full-charge is detected


/*CONTROL STATUS BITS*/
#define BQ27xxx_STATUS_SS		(1<<13)

#define BQ27xxx_BLOCK_DATA_CHECKSUM	(0x60)
#define BQ27xxx_BLOCK_DATA_CONTROL	(0x61)
#define BQ27xxx_DATA_CLASS	  	(0x3E)
#define BQ27xxx_DATA_BLOCK		(0x3F)
#define BQ27xxx_DATA			(0x40)

#define BQ27xxx_ID_STATE 		(82)
#define BQ27xxx_DCAP		        (10) //Configures the design capacity of the connected battery
#define BQ27xxx_DESIGN_ENERGY          	(12) //Configures the design energy of the connected battery
#define BQ27xxx_TAPER_RATE             	(27) //Configures taper rate of connected battery
#define BQ27xxx_TERMINATE_VOLTAGE      	(16) //Configures the terminate voltage
#define BQ27xxx_V_CHG_TERM  

enum POWER_SUPPLY_STATUS{
	POWER_SUPPLY_STATUS_CHARGING =1,
	POWER_SUPPLY_STATUS_DISCHARGING,
	POWER_SUPPLY_STATUS_FULL,
	POWER_SUPPLY_STATUS_NOT_CHARGING
};

enum POWER_SUPPLY_CAPACITY_LEVEL{
	POWER_SUPPLY_CAPACITY_LEVEL_FULL =1,
	POWER_SUPPLY_CAPACITY_LEVEL_LOW,
	POWER_SUPPLY_CAPACITY_LEVEL_CRITICAL,
	POWER_SUPPLY_CAPACITY_LEVEL_NORMAL
};

typedef struct BQ27xxx_device_info{
         u16 design_capacity;
         u16 design_energy;
         u16 taper_rate;
         u16 terminate_voltage;
         u16 wait_time;
}BQ27xxx_device;

typedef struct BQ27XXX_BATTAERY{

	u16 chip_id;

	u16 volt_now;                   //mVolt
	int current_now;                //mA
	u16 soc;                        //%
	u16 full_charge_capacity;       //mAh
	u16 nominal_available_capacity; //mAh
	u16 d_cap;                      //mAh
	u16 flags;		

	u8 supply_status;
	u8 capacity_level;

}bq27xxx_battery;


int power_fg_init(unsigned char bus, struct udevice *i2c_dev );
int power_check_battery(bq27xxx_battery *); 
#endif /* __BQ27XXX_FG_H_ */

