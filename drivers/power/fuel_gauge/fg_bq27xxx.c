/* Author : Silicon Signals
*/

#include <common.h>
#include <errno.h>
#include <power/bq27xxx_fg.h>
#include <i2c.h>

#define CONFIG_ENABLE 0

BQ27xxx_device default_battery_config = {
	.design_capacity	= 3200, /*mAh*/
	.design_energy		= 11800,	/*design Capacity * 3.8f for G1B*/
	.taper_rate		= 200,	/*mA*/
	.terminate_voltage	= 3200,	/*mV*/
	.wait_time		= 10	/*mSec*/
};

struct udevice *fg_i2c_dev = NULL;

static inline int BQ27xxx_write(uint8_t fg_reg_add, uint16_t fg_reg_wr_ctrl_cmd, uint8_t len) {

	int retval = 0;

	uchar fg_ctrl_cmd[2] = {(fg_reg_wr_ctrl_cmd & 0xff), (fg_reg_wr_ctrl_cmd >> 8)};
	retval = dm_i2c_write(fg_i2c_dev, fg_reg_add, (const uchar *)&fg_ctrl_cmd, len);
	
	if(retval) 
	{
		printf("Fual-Gauge: Error -> dm_i2c_write failed @ 0x%04x! %d\n", fg_reg_wr_ctrl_cmd, retval);
		return retval;
	}
	mdelay(50);
	return 0;
}

/*BQ27XX_readWord()  :Follow the "Standard Commands" to read battery information
 * i.e.  >> Voltage() @ 0x04 
 *	    fg_reg_add	= 0x04
 *          len 	= 2 
 *	    *fg_data	= pass the address of variable
 *	    return 0 on sucess and -1 on fail
 *       >> Temperature() @ 0x02 
 * more details check : https://github.com/sparkfun/SparkFun_BQ27441_Arduino_Library/blob/master/src/BQ27441_Definitions.h
 */
static inline int BQ27xxx_readWord(uint8_t fg_reg_add, uint8_t len){

	int ret = 0;
	uchar fg_buf_data[2] = {0, 0};

	ret =  dm_i2c_read(fg_i2c_dev, fg_reg_add, (const uchar *)&fg_buf_data, len);
	if(ret < 0){		
		return ret;
	}
	mdelay(50);
	return (((uint16_t)fg_buf_data[1] << 8) | fg_buf_data[0]);
}

static inline int BQ27xxx_readControlWord(uint16_t ctrl_cmd) {

	int retval = 0;
	uchar fg_ctrl_cmd[2] = {(ctrl_cmd & 0xff), (ctrl_cmd >> 8)};
	uchar fg_data[2] = {0, 0};

	// Write Control Command of 2 Bytes @ 0x00 Address
	retval = dm_i2c_write(fg_i2c_dev, 0x00, (const uchar *)&fg_ctrl_cmd, 2); //Write Word
	mdelay(50);
	if(retval <0){
		return retval;
	}

	// read 2 bytes of data
	retval =  dm_i2c_read(fg_i2c_dev, 0x00, (const uchar *)&fg_data, 2);
	if(retval <0){
		return retval;
	}
	mdelay(50);	
	return ((uint16_t)fg_data[1] << 8) | fg_data[0];
}

static inline int BQ27xxx_extended_write_byte_word(u8 dataClassID, u8 offset, u16 data, u8 len){

	int ret = 0;
	u8 old_checksum = 0;
	u8 new_checksum = 0;
	u8 temp_checksum = 0;
	u8 read_checksum = 0;
	u8 dataBlock = offset / 32;

	uchar fg_buf[2] = {0,0};
	uchar fg_data[2] = {(data>>8), (data & 0xff)};

	// Set Data Class
	ret = BQ27xxx_write(BQ27xxx_DATA_CLASS, dataClassID, 1);
	if(ret <0)
		return ret;
	mdelay(20);
	// Set Data Block
	ret = BQ27xxx_write(BQ27xxx_DATA_BLOCK, dataBlock, 1);
	if(ret <0)
		return ret;
	mdelay(20);
	// Get Old Checksum
	ret = dm_i2c_read(fg_i2c_dev, BQ27xxx_BLOCK_DATA_CHECKSUM, (const uchar *)&fg_buf, 1); 
	if(ret <0)
		return ret;
	old_checksum = fg_buf[0] & 0xff;

	// Get Old Data
	ret = dm_i2c_read(fg_i2c_dev, BQ27xxx_DATA + offset % 32, (const uchar *)&fg_buf, 2); 
	if(ret <0)
		return ret;

	// Send New Data
	ret = dm_i2c_write(fg_i2c_dev, BQ27xxx_DATA + offset % 32, (const uchar *)&fg_data, 2);
	if(ret <0)
		return ret;
	mdelay(20);
	// Compute Checksum
	temp_checksum = (255 - old_checksum - fg_buf[0] - fg_buf[1]) % 256;
	new_checksum  =  255 - ((temp_checksum + fg_data[0] + fg_data[1]) % 256);
	new_checksum &= 0xFF;

	// Send New Checksum
	ret = BQ27xxx_write(BQ27xxx_BLOCK_DATA_CHECKSUM, new_checksum, 1);
	if(ret <0)
		return ret;
	mdelay(20);
	// **** Re-verification for write succesfull ****
	// Set Data Class
	ret = BQ27xxx_write(BQ27xxx_DATA_CLASS, dataClassID, 1);
	if(ret <0)
		return ret;

	// Set Data Block
	ret = BQ27xxx_write(BQ27xxx_DATA_BLOCK, dataBlock, 1);
	if(ret <0)
		return ret;

	// Get  Checksum
	 fg_buf[0] = fg_buf[1] =0;
	ret = dm_i2c_read(fg_i2c_dev, BQ27xxx_BLOCK_DATA_CHECKSUM, (const uchar *)&fg_buf, 1);
	if(ret <0)
		return ret;
	read_checksum = fg_buf[0] & 0xff;

	if(read_checksum != new_checksum){
		printf("Fual-Gauge: Fail to update parameter --> %d\n", data);
	}

	return 0;
}

static inline int config_mode_start(void){
	int ret;
	int control_status;
	int flags_lsb;
	int itpor;
	uint16_t timeout;	

	/*config mode check*/
	ret = BQ27xxx_readWord(BQ27xxx_FLAGS,2);
	if(ret < 0)
		return ret;	

	itpor = ret & BQ27xxx_FLAGS_ITPOR;	
	
	if(itpor ==0){
		printf("Fual-Gauge: No need to update config parameters \n");
		return -1;
	}

	flags_lsb = (ret & 0xff);

	if(flags_lsb & BQ27xxx_FLAGS_CFGUPMODE) {
		printf("Fual-Gauge: Already in config mode\n");
		return 0;
	}

	// unseal the fg for data access if needed
	ret = BQ27xxx_readControlWord(BQ27xxx_CONTROL_STATUS);
	if(ret <0)
		return ret;

	control_status = ret;

	if(control_status & BQ27xxx_STATUS_SS){
		ret = BQ27xxx_write(BQ27xxx_CONTROL, BQ27xxx_UNSEAL_KEY, 2);
		if(ret <0)
			return ret;
		ret = BQ27xxx_write(BQ27xxx_CONTROL, BQ27xxx_UNSEAL_KEY, 2);
		if(ret <0)
			return ret;	
	}
	else{
		//printf("Fual-Gauge: Already unsealed\n");
	}
	mdelay(20);

	ret = BQ27xxx_readControlWord(BQ27xxx_CONTROL_STATUS);
	if(ret <0)
		return ret;

	// set fuel gauge in configmode
	ret = BQ27xxx_write(BQ27xxx_CONTROL, BQ27xxx_SET_CFGUPDATE, 2);
	if(ret <0){
		return ret;
	}

	// wait for config mode set
	timeout = 50; flags_lsb = 0;
	while(!flags_lsb)
	{
		ret = BQ27xxx_readWord(BQ27xxx_FLAGS,2);
		if(ret <0)
			return ret;

		flags_lsb = (ret & BQ27xxx_FLAGS_CFGUPMODE);

		if(timeout)
		{
			timeout--; mdelay(100);
		}	
		else
		{
			printf("Fual-Gauge: Unable to enable CFGUPDATE mode\n");
			return -1; //Timeout Occure
		}
	}

	// Enable Block Mode
	ret = BQ27xxx_write(BQ27xxx_CONTROL, 0x00, 1);
	if(ret <0){
		printf("Fual-Gauge: Unable to enable block mode, ret %d\n", ret);
		return ret;
	}

	return 0;	
}

static inline int config_mode_stop(void){
	int ret;
	int flags_lsb;
	uint16_t timeout;

	ret = BQ27xxx_readWord(BQ27xxx_FLAGS,2);
	if(ret <0)
		return ret;

	if(ret & BQ27xxx_FLAGS_CFGUPMODE){

		ret = BQ27xxx_write(BQ27xxx_CONTROL, BQ27xxx_SOFT_RESET, 2);
		if(ret <0)
			return ret;

		// Wait for config mode to exit
		flags_lsb = BQ27xxx_FLAGS_CFGUPMODE;
		timeout = 5;
		while(flags_lsb){
			ret = BQ27xxx_readWord(BQ27xxx_FLAGS,2);
			if(ret <0)
				return ret;

			flags_lsb = (ret & BQ27xxx_FLAGS_CFGUPMODE);
			if(timeout){
				timeout--; mdelay(1000);
			}else{
				return -1; flags_lsb =0;
			}
		}		
	}

	// seal the fuel gauge
	ret = BQ27xxx_write(BQ27xxx_CONTROL, BQ27xxx_SEALED_KEY, 2);
	if(ret <0)
		return ret;

	return 0;
}

static inline int is_fg_available(void){
	int ret=0; 
	
	ret = BQ27xxx_readWord(BQ27xxx_FLAGS,2);
	if(ret <0){
		printf("Fual-Gauge: Enable to read BQ27441_FLAGS\n");
		return ret;
	}
	
	return ret;	
}

static inline int configure(BQ27xxx_device *bat){

	int ret;

	ret = is_fg_available();
	if(ret <0){
		printf("Fual-Gauge: Failed to initialize\n");
		return ret;
	}
	
	ret = config_mode_start();	
	if(ret < 0){		
		printf("Fual-Gauge: Fail to enter config mode \n");
		return ret;
	}

	ret = BQ27xxx_extended_write_byte_word(BQ27xxx_ID_STATE, BQ27xxx_DCAP, bat->design_capacity, 2);
	if(ret <0){		
		printf("Fual-Gauge: Fail to set DESIGN CAPACITY \n");
		return ret;
	}

	ret = BQ27xxx_extended_write_byte_word(BQ27xxx_ID_STATE, BQ27xxx_DESIGN_ENERGY, bat->design_energy, 2);
	if(ret <0){
		printf("Fual-Gauge: Fail to set DESIGN ENERGY \n");
		return ret;
	}

	ret = BQ27xxx_extended_write_byte_word(BQ27xxx_ID_STATE, BQ27xxx_TERMINATE_VOLTAGE, bat->terminate_voltage, 2);
	if(ret <0){
		printf("Fual-Gauge: Fail to set TERMINATE VOLTAGE \n");
		return ret;	
	}

	ret = BQ27xxx_extended_write_byte_word(BQ27xxx_ID_STATE, BQ27xxx_TAPER_RATE, bat->taper_rate, 2);
	if(ret <0){
		printf("Fual-Gauge: Fail to set TAPER RATE \n");
		return ret;
	}

	ret = config_mode_stop();
	if(ret <0)
		printf("Fual-Gauge: Fail to exit config mode \n");

	return 0;	
}

static inline int battery_crnt_n_status(int *pCurrentData, u8 *pBatStatus, u8 *pBatLevel, int status_flags){

	int bitCheck =0;

	/*sign value conversion*/	
	if(pCurrentData != NULL)
		*pCurrentData = (int)((s16)(*pCurrentData)); // * 1000; //uA
	
	bitCheck = (status_flags & BQ27XXX_FLAG_FC);
	/*battery status*/
	if(pBatStatus != NULL) {

		if(*pCurrentData >0)
			*pBatStatus = POWER_SUPPLY_STATUS_CHARGING;
		else if(*pCurrentData <0)		
			*pBatStatus = POWER_SUPPLY_STATUS_DISCHARGING;
		else {
			if(bitCheck){
				*pBatStatus = POWER_SUPPLY_STATUS_FULL;
			}
			else{
				*pBatStatus = POWER_SUPPLY_STATUS_NOT_CHARGING;
			}
		}
	}

	/*battery level*/
	if(pBatLevel != NULL) {
		if(status_flags & BQ27XXX_FLAG_FC)
			*pBatLevel = POWER_SUPPLY_CAPACITY_LEVEL_FULL;
		else if(status_flags & BQ27XXX_FLAG_SOC1)
			*pBatLevel = POWER_SUPPLY_CAPACITY_LEVEL_LOW;
		else if(status_flags & BQ27XXX_FLAG_SOCF)
			*pBatLevel = POWER_SUPPLY_CAPACITY_LEVEL_CRITICAL;
		else
			*pBatLevel = POWER_SUPPLY_CAPACITY_LEVEL_NORMAL;
	}

	return 0;

}

 int power_read_battery_property(bq27xxx_battery *bat) {

	int ret;
	int is_battery_connect =-1;
	int flags =0;
	int curr=0;
	u8 bat_status=0;
	u8 bat_level =0;


	ret= BQ27xxx_readControlWord(BQ27xxx_DEVICE_TYPE);
	if(ret <0)
		return ret;
	bat->chip_id = ret;


	ret= BQ27xxx_readWord(BQ27xxx_FLAGS, 2);
	if(ret <0)
		return ret;
	
	is_battery_connect = (is_battery_connect & BQ27XXX_FLAG_BAT_DET);
	if(is_battery_connect ==0)
		return -1;

	flags =ret;
	bat->flags =flags;

	ret= BQ27xxx_readWord(BQ27xxx_DESIGN_CAPACITY, 2);	
	if(ret <0)
		ret =0;
	bat->d_cap = ret;


	ret= BQ27xxx_readWord(BQ27xxx_STATE_OF_CHARGE, 2);
	if(ret <0)
		ret =0;
	bat->soc = ret;

	ret= BQ27xxx_readWord(BQ27xxx_VOLTAGE_NOW, 2);
	if(ret <0)
		ret =0;
	bat->volt_now = ret;

	ret= BQ27xxx_readWord(BQ27xxx_FULL_CHARGE_CAPACITY, 2);
	if(ret <0)
		ret =0;
	bat->full_charge_capacity =ret;

	ret= BQ27xxx_readWord(BQ27xxx_NOMINAL_AVG_CURRENT , 2);
	if(ret <0)
		ret =0;
	bat->nominal_available_capacity =ret;

	ret = BQ27xxx_readWord(BQ27xxx_AVERAGE_CURRENT, 2);
	if(ret <0)
		ret =0;
	curr =ret;
	// printk("Current Now : %d\n",curr);
	if(flags){
		battery_crnt_n_status(&curr, &bat_status, &bat_level, flags);

		bat->current_now	= curr;
		bat->supply_status	= bat_status;
		bat->capacity_level	= bat_level;
	}

	return 0;
}

int power_fg_init_update(unsigned char bus, struct udevice *i2c_dev)
{
	BQ27xxx_device  bat_config;
	bq27xxx_battery bat_crnt_status;
	fg_i2c_dev = i2c_dev;

	// Reset data of battery data structure 
	memset(&bat_crnt_status, 0, sizeof(bat_crnt_status));

	// copy default battery config parameter
	memcpy(&bat_config, &default_battery_config, sizeof(bq27xxx_battery));

	/*update & write battery parameters to fuel gauge*/
	configure(&bat_config);

	if (power_read_battery_property(&bat_crnt_status) < 0) {
		printf("Fual-Gauge: FUEL GAUGE NOT CONNECTED \n");
		return -ENODEV;
	}

	//printf("################################# DATA FROM DRIVER ####################################\n");
	//printf("Fual-Gauge: FUEL GAUGE CHIP ID      	>> 0x%04x \n", bat_crnt_status.chip_id);
	//printf("Fual-Gauge: BATTERY DESIGN CAPACITY 	>> %d mAh \n", bat_crnt_status.d_cap);
	//printf("Fual-Gauge: BATTERY STATE OF CHAREG 	>> %d %%  \n", bat_crnt_status.soc);
	//printf("Fual-Gauge: BATTERY VOLTAGE NOW	 	>> %d mV  \n", bat_crnt_status.volt_now);
	//printf("Fual-Gauge: BATTERY CURRENT NOW     	>> %d mA  \n", bat_crnt_status.current_now);	
	//printf("Fual-Gauge: BATTERY FULL CHARGE CAP 	>> %d mAh \n", bat_crnt_status.full_charge_capacity);	
	//printf("Fual-Gauge: BATTERY NOMINAL AVAILABLE CAP	>> %d mAh \n", bat_crnt_status.nominal_available_capacity);
	//printf("Fual-Gauge: Flags				>> 0x%04x \n", bat_crnt_status.flags);	

	//printf("Fual-Gauge: POWER_SUPPLY_STATUS		>> ");

	// switch(bat_crnt_status.supply_status)
	// {
	// 	case POWER_SUPPLY_STATUS_CHARGING    : printf(" CHARGING\n");		break;
	// 	case POWER_SUPPLY_STATUS_DISCHARGING : printf(" DISCHARGING\n");       break;
	// 	case POWER_SUPPLY_STATUS_FULL	     : printf(" FULL\n");       	break;
	// 	case POWER_SUPPLY_STATUS_NOT_CHARGING: printf(" NOT_CHARGING\n");      break;
	// 	default: printf(" NO_BATTERY\n"); 	break;
	// }
	
	//printf("Fual-Gauge: POWER_SUPPLY_CAPACITY_LEVEL    >> ");


	// switch(bat_crnt_status.capacity_level)
    //     {
    //            case POWER_SUPPLY_CAPACITY_LEVEL_FULL    : printf(" FULL\n");           break;
    //            case POWER_SUPPLY_CAPACITY_LEVEL_LOW	: printf(" LOW\n");       break;
    //            case POWER_SUPPLY_CAPACITY_LEVEL_CRITICAL: printf(" CRITICAL\n");               break;
    //            case POWER_SUPPLY_CAPACITY_LEVEL_NORMAL  : printf(" NORMAL\n");      break;
    //            default: printf(" NO_BATTERY\n");       break;
    //     }

	//printf("############################### END DATA FROM DRIVER ####################################\n");

	return 0;
}

