/***************************************************************************
 * File Name:		wlc.c
 * Description:		This main file contains all the most important
 *					functions generally used by a device driver the
 *					driver
 ***************************************************************************/

/***************************************************************************
 * Included files
 ***************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "stwlc38.h"

#ifdef UBIN
#include "ubin_data.h"
#endif

#ifndef UBIN
//#include "nvm_data.h"
#include "STSW-WLC38RX-nvm_data.h"
#endif
/***************************************************************************
 * Macro definitions
 ***************************************************************************/
// #define DEBUG_I2C	/* Uncomment this line for debug purpose */
#define BUFF_SIZE		2048
#define IO_DELAY_MS		1000
#define SLAVE_ADDRESS	0x61

/***************************************************************************
 * Global variables
 ***************************************************************************/
I2C_HandleTypeDef *hi2c = NULL;
UART_HandleTypeDef *huart = NULL;

/***************************************************************************
 * Private variables
 ***************************************************************************/
static u8 i2cSequentialRxDone = 0;
static u8 i2cSequentialTxDone = 0;
static char buff[BUFF_SIZE];

/***************************************************************************
 * Function declarations
 ***************************************************************************/
#ifdef UBIN
int get_fw_ubin_file (char *name, u8 **data, int *size);
int parse_ubin_file(u8 *ubin_data, int ubin_size, struct firmware_file *fw_data);
#endif

/***************************************************************************
 * Struct Initializations
 ***************************************************************************/
#ifdef UBIN
	struct firmware_file fw_data;
#endif

/***************************************************************************
 * Function definitions
 ***************************************************************************/
void msleep(int msec)
{
	HAL_Delay(msec);
}

void pr_err(char *msg, ...)
{	
	va_list args;
	va_start(args, msg);
	sprintf(buff, "[ST-ERROR] ");
	vsprintf(buff + 11, msg, args);
	if (buff[strlen(buff)-1] == '\n')
		sprintf(buff + strlen(buff)-1, "\r\n");
	else
		sprintf(buff + strlen(buff), "\r\n");
	va_end(args);
	HAL_UART_Transmit(huart, (u8 *)buff, strlen(buff), IO_DELAY_MS);
}

void pr_info(char *msg, ...)
{	
	va_list args;
	va_start(args, msg);
	sprintf(buff, "[ST-INFO] ");
	vsprintf(buff + 10, msg, args);
	if (buff[strlen(buff)-1] == '\n')
		sprintf(buff + strlen(buff)-1, "\r\n");
	else
		sprintf(buff + strlen(buff), "\r\n");
	va_end(args);
	HAL_UART_Transmit(huart, (u8 *)buff, strlen(buff), IO_DELAY_MS);
}

void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
	i2cSequentialTxDone = 1;
}

void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
	i2cSequentialRxDone = 1;
}

void I2C_reset()
{
	HAL_I2C_DeInit(hi2c);
	HAL_Delay(20);
	HAL_I2C_Init(hi2c);
	HAL_Delay(20);
}

HAL_StatusTypeDef wlc_i2c_write(uint8_t* cmd, int cmd_length)
{
#ifdef DEBUG_I2C
	char str[BUFF_SIZE];
	sprintf(str, "[WR-W]: ");
	for(int i = 0; i < cmd_length; i++)
		sprintf(str + strlen(str), "%02X ", cmd[i]);
	
	pr_info(str);
#endif
	
	HAL_StatusTypeDef status = HAL_OK;	
	status = HAL_I2C_Master_Transmit(hi2c, SLAVE_ADDRESS << 1, cmd, cmd_length, IO_DELAY_MS);
	if(status == HAL_BUSY)
	{
		I2C_reset();
		status = HAL_I2C_Master_Transmit(hi2c, SLAVE_ADDRESS << 1, cmd, cmd_length, IO_DELAY_MS);
	}
			
	return status;
}

HAL_StatusTypeDef wlc_i2c_read(uint8_t* cmd, int cmd_length, uint8_t* read_data, int read_count)
{
#ifdef DEBUG_I2C
	char str[BUFF_SIZE];
	sprintf(str, "[WR-W]: ");
	for(int i = 0; i < cmd_length; i++)
		sprintf(str + strlen(str), "%02X ", cmd[i]);
#endif	
	HAL_StatusTypeDef status = HAL_OK;
	
		i2cSequentialTxDone = 0;
		i2cSequentialRxDone = 0;

		uint32_t startTick = HAL_GetTick();

		status = HAL_I2C_Master_Sequential_Transmit_IT(hi2c, SLAVE_ADDRESS << 1, cmd, cmd_length, I2C_FIRST_FRAME);
		if(status != HAL_OK){
			if(status == HAL_BUSY) {
					I2C_reset();
					status = HAL_I2C_Master_Sequential_Transmit_IT(hi2c, SLAVE_ADDRESS << 1, cmd, cmd_length, I2C_FIRST_FRAME);
					if(status != HAL_OK)
						return HAL_ERROR;
				}
			else
					return HAL_ERROR;
		}
		
		while(i2cSequentialTxDone == 0)
		{
			if((HAL_GetTick() - startTick) > IO_DELAY_MS)
			{
				return HAL_TIMEOUT;
			}
		}
		if(HAL_I2C_Master_Sequential_Receive_IT(hi2c, SLAVE_ADDRESS << 1, read_data, read_count, I2C_LAST_FRAME) != HAL_OK)
			return HAL_ERROR;

	
		while(i2cSequentialRxDone == 0)
		{
			if((HAL_GetTick() - startTick) > IO_DELAY_MS)
			{
				return HAL_TIMEOUT;
			}
		}
		status = HAL_OK;
#ifdef DEBUG_I2C
	 sprintf(str + strlen(str), "[WR-R]: ");
	 for(int i = 0; i < read_count; i++)
		 sprintf(str + strlen(str), "%02X ", read_data[i]);	
	pr_info(str);
#endif		
	return status;
}
/*** Low Level API for I2C/UART communication**/

static int hw_i2c_write(u32 addr, u8 *data, u32 data_length)
{
	u8 *cmd = (u8 *)malloc(sizeof(u8) * (5 + data_length));

	cmd[0] = OPCODE_WRITE;
	cmd[1] = (u8)((addr >> 24) & 0xFF);
	cmd[2] = (u8)((addr >> 16) & 0xFF);
	cmd[3] = (u8)((addr >> 8) & 0xFF);
	cmd[4] = (u8)((addr >> 0) & 0xFF);
	memcpy(&cmd[5], data, data_length);

	if ((wlc_i2c_write(cmd, (5 + data_length))) != OK) {
		pr_err("[WLC] Error in writing Hardware I2c!\n");
		free(cmd);
		return E_BUS_W;
	}

	free(cmd);
	return OK;
}

static int fw_i2c_write(u16 addr, u8 *data, u32 data_length)
{
	u8 *cmd = (u8 *)malloc(sizeof(u8) * (2 + data_length));

	cmd[0] = (u8)((addr >>  8) & 0xFF);
	cmd[1] = (u8)((addr >>  0) & 0xFF);
	memcpy(&cmd[2], data, data_length);

	if ((wlc_i2c_write(cmd, (2 + data_length))) != OK) {
		pr_err("[WLC] ERROR: in writing Hardware I2c!\n");
		free(cmd);
		return E_BUS_W;
	}

	free(cmd);
	return OK;
}

static int hw_i2c_read(u32 addr, u8 *read_buff, int read_count)
{
	u8 *cmd = (u8 *)malloc(sizeof(u8) * 5);

	cmd[0] = OPCODE_WRITE;
	cmd[1] = (u8)((addr >> 24) & 0xFF);
	cmd[2] = (u8)((addr >> 16) & 0xFF);
	cmd[3] = (u8)((addr >>  8) & 0xFF);
	cmd[4] = (u8)((addr >>  0) & 0xFF);

	if ((wlc_i2c_read(cmd, 5, read_buff, read_count)) != OK) {
		pr_err("[WLC] Error in writing Hardware I2c!\n");
		free(cmd);
		return E_BUS_WR;
	}

	free(cmd);
	return OK;
}

static int fw_i2c_read(u16 addr, u8 *read_buff,
		int read_count)
{
	u8 *cmd = (u8 *)malloc(sizeof(u8) * 2);

	cmd[0] = (u8)((addr >>  8) & 0xFF);
	cmd[1] = (u8)((addr >>  0) & 0xFF);

	if ((wlc_i2c_read(cmd, 2, read_buff, read_count)) != OK) {
		pr_err("[WLC] Error in writing Hardware I2c!\n");
		free(cmd);
		return E_BUS_WR;
	}

	free(cmd);
	return OK;
}

char *print_hex(char *label, u8 *buff, int count, char *result)
{
	int i, offset;

	offset = strlen(label);
	strlcpy(result, label, offset + 1); /* +1 for terminator char */

	for (i = 0; i < count; i++) {
		snprintf(&result[offset], 4, "%02X ", buff[i]);
		/* this append automatically a null terminator char */
		offset += 3;
	}
	return result;
}

static void system_reset()
{
	u8 cmd[6] = { 0x00 };
	u32 addr = HWREG_RST_ADDR;

	/* System reset */
	cmd[0] = OPCODE_WRITE;
	cmd[1] = (u8)((addr >> 24) & 0xFF);
	cmd[2] = (u8)((addr >> 16) & 0xFF);
	cmd[3] = (u8)((addr >> 8) & 0xFF);
	cmd[4] = (u8)((addr >> 0) & 0xFF);
	cmd[5] = 0x01;
	wlc_i2c_write(cmd, 6);
	msleep(AFTER_SYS_RESET_SLEEP_MS);

	/* I2C NACK handling after system reset*/
	I2C_reset();
}

static int get_wlc_chip_info(struct wlc_chip_info *info)
{
	u8 read_buff[14] = { 0x00 };

	if (fw_i2c_read(FWREG_CHIP_ID_ADDR, read_buff, 14) != OK) {
		pr_err("[WLC] Error while getting wlc_chip_info\n");
		return E_BUS_R;
	}

	info->chip_id = (u16)(read_buff[0] + (read_buff[1] << 8));
	info->chip_revision = read_buff[2];
	info->customer_id = read_buff[3];
	info->project_id = (u16)(read_buff[4] + (read_buff[5] << 8));
	info->nvm_patch_id = (u16)(read_buff[6] + (read_buff[7] << 8));
	info->ram_patch_id = (u16)(read_buff[8] + (read_buff[9] << 8));
	info->config_id = (u16)(read_buff[10] + (read_buff[11] << 8));
	info->pe_id = (u16)(read_buff[12] + (read_buff[13] << 8));

	if (hw_i2c_read(HWREG_HW_VER_ADDR, read_buff, 1) != OK) {
		pr_err("[WLC] Error while getting wlc_chip_info\n");
		return E_BUS_R;
	}

	info->cut_id = read_buff[0];

	pr_info("[WLC] ChipID: %04X Chip Revision: %02X CustomerID: %02X "
		"RomID: %04X NVMPatchID: %04X RAMPatchID: %04X CFG: %04X "
		"PE: %04X\n", info->chip_id, info->chip_revision,
		info->customer_id, info->project_id, info->nvm_patch_id,
		info->ram_patch_id, info->config_id, info->pe_id);

	return OK;
}

static int wlc_nvm_write_sector(const u8 *data, int data_length,
									int sector_index)
{
	int err = 0;
	int i = 0;
	int timeout = 1;
	u8 reg_value = (u8)sector_index;
	u8 write_buff[NVM_SECTOR_SIZE_BYTES];

	pr_info("[WLC] writing sector %02X\n", sector_index);
	if (data_length > NVM_SECTOR_SIZE_BYTES) {
		pr_err("[WLC] sector data bigger than 256 bytes\n");
		return E_INVALID_INPUT;
	}

	memset(write_buff, 0, NVM_SECTOR_SIZE_BYTES);
	memcpy(write_buff, data, data_length);

	err = fw_i2c_write(FWREG_NVM_SECTOR_INDEX_ADDR, &reg_value, 1);
	if (err != OK)
		return err;

	reg_value = 0x10;
	err = fw_i2c_write(FWREG_SYS_CMD_ADDR, &reg_value, 1);
	if (err != OK)
		return err;

	err = fw_i2c_write(FWREG_AUX_DATA_00_ADDR, write_buff, data_length);
	if (err != OK)
		return err;

	reg_value = 0x04;
	err = fw_i2c_write(FWREG_SYS_CMD_ADDR, &reg_value, 1);
	if (err != OK)
		return err;

	for (i = 0; i < 20; i++) {
		msleep(1);
		err = fw_i2c_read(FWREG_SYS_CMD_ADDR, &reg_value, 1);
		if (err != OK)
			return err;
		if ((reg_value & 0x04) == 0) {
			timeout = 0;
			break;
		}
	}

	reg_value = 0x20;
	if (fw_i2c_write(FWREG_SYS_CMD_ADDR, &reg_value, 1) != OK)
		pr_err("[WLC] Error power down the NVM\n");

	return timeout == 0 ? OK : E_TIMEOUT;
}

static int wlc_nvm_write_bulk(const u8 *data, int data_length,
								u8 sector_index)
{
	int err = 0;
	int remaining = data_length;
	int to_write_now = 0;
	int written_already = 0;
	while (remaining > 0) {
		to_write_now = remaining > NVM_SECTOR_SIZE_BYTES
						? NVM_SECTOR_SIZE_BYTES : remaining;
		err = wlc_nvm_write_sector(data + written_already,
									to_write_now, sector_index);
		if (err != OK)
			return err;
		remaining -= to_write_now;
		written_already += to_write_now;
		sector_index++;
	}

	return OK;
}

static int wlc_nvm_write()
{
	int err = 0;
	u8 reg_value = 0;

	/* Disable Tx pinging if detected Tx mode */
	err = fw_i2c_read(FWREG_OP_MODE_ADDR, &reg_value, 1);
	if (err != OK)
		return err;
	pr_info("[WLC] OP MODE %02X\n", reg_value);
	if (reg_value == FW_OP_MODE_TX) {
		reg_value = 0x02;
		err = fw_i2c_write(FWREG_TX_CMD_ADDR, &reg_value, 1);
		if (err != OK)
			return err;
	}
	msleep(GENERAL_SLEEP_MS);

	reg_value = 0x0B;
	err = hw_i2c_write(HWREG_TM_CONFIG_ADDR, &reg_value, 1);
	if (err != OK)
		return err;

	reg_value = 0x00;
	err = hw_i2c_write(HWREG_TM_CONFIG_ADDR, &reg_value, 1);
	if (err != OK)
		return err;

	/* FW system reset */
	reg_value = 0x40;
	err = fw_i2c_write(FWREG_SYS_CMD_ADDR, &reg_value, 1);
	if (err != OK)
		return err;
	msleep(AFTER_SYS_RESET_SLEEP_MS);

	/* DC mode checking */
	err = fw_i2c_read(FWREG_OP_MODE_ADDR, &reg_value, 1);
	if (err != OK)
		return err;
	pr_info("[WLC] OP MODE %02X\n", reg_value);
	if (reg_value != FW_OP_MODE_SA) {
		pr_err("[WLC] no DC power detected, nvm programming aborted\n");
		return E_UNEXPECTED_OP_MODE;
	}

	reg_value = 0xC5;
	err = fw_i2c_write(FWREG_NVM_PWD_ADDR, &reg_value, 1);
	if (err != OK)
		return err;

	pr_info("[WLC] RRAM Programming..\n");
	/* Patch writing */

#ifdef UBIN
	err = wlc_nvm_write_bulk(fw_data.fw_patch_data, fw_data.fw_patch_size,
								 NVM_PATCH_START_SECTOR_INDEX);
#else 
	err = wlc_nvm_write_bulk(nvm_patch_data, NVM_PATCH_SIZE,
								 NVM_PATCH_START_SECTOR_INDEX);
#endif 

	if (err != OK)
		return err;

	/* Cfg writing */
#ifdef UBIN
	err = wlc_nvm_write_bulk(fw_data.fw_config_data, fw_data.fw_config_size,
								 NVM_CFG_START_SECTOR_INDEX);
#else 
	err = wlc_nvm_write_bulk(nvm_cfg_data, NVM_CFG_SIZE,
								 NVM_CFG_START_SECTOR_INDEX);
#endif 

	if (err != OK)
		return err;

	system_reset();

	return OK;
}

int chip_info_show(char *buf)
{
	int count;
	char temp[100];
	u8 read_buff[14] = { 0x00 };
	u8 cmd[2] = { 0x00 };

	cmd[0] = (FWREG_CHIP_ID_ADDR & 0xFF00) >> 8;
	cmd[1] = (FWREG_CHIP_ID_ADDR & 0xFF);
	pr_info("[WLC] Chip Id Command: %02X %02X\n", cmd[0], cmd[1]);

	if (wlc_i2c_read(cmd, 2, read_buff, 14) != OK) {
		pr_err("[WLC] could not read the register\n");
		count = snprintf(buf, PAGE_SIZE, "CHIP INFO READ ERROR {%02X}\n",
						 E_BUS_WR);
		return count;
	}

	pr_info("[WLC] Chip Id : 0x%04X\n",
			(read_buff[0] | (read_buff[1] << 8)));
	pr_info("[WLC] Chip Revision : 0x%02X\n",
			read_buff[2]);
	pr_info("[WLC] Customer Id : 0x%02X\n",
			read_buff[3]);
	pr_info("[WLC] Rom Id : 0x%04X\n",
			(read_buff[4] + (read_buff[5] << 8)));
	pr_info("[WLC] NVM Patch Id : 0x%04X\n",
			(read_buff[6] + (read_buff[7] << 8)));
	pr_info("[WLC] RAM Patch Id : 0x%04X\n",
			(read_buff[8] + (read_buff[9] << 8)));
	pr_info("[WLC] Cfg Id : 0x%04X\n",
			(read_buff[10] + (read_buff[11] << 8)));
	pr_info("[WLC] PE Id : 0x%04X\n",
			(read_buff[12] + (read_buff[13] << 8)));

	count = snprintf(buf, PAGE_SIZE, "%s\n",
					 print_hex("Chip Info: ", read_buff, 3, temp));

	return count;
}

int nvm_program_show(char *buf)
{
	int err = 0;
	int count = 0;
	int config_id_mismatch = 0;
	int patch_id_mismatch = 0;
	struct wlc_chip_info chip_info;

#ifdef UBIN
	int original_size = ubin_size;
	u8 *original_data = ubin_data;

	err = parse_ubin_file(original_data, original_size, &fw_data);
	if (err != OK) {
		pr_err("[WLC] Failed parsing ubin file.........ERROR %08X\n", err);
		goto exit_0;
	}
#endif

	pr_info("[WLC] NVM Programming started\n");

	if (get_wlc_chip_info(&chip_info) != OK) {
		pr_err("[WLC] Error in reading wlc_chip_info\n");
		err = E_BUS_R;
		goto exit_0;
	}

	pr_info("[WLC] Chip Id: %02X\n", chip_info.chip_id);

#ifdef UBIN
	if (chip_info.chip_id != fw_data.chip_id) {
#else 
	if (chip_info.chip_id != NVM_TARGET_CHIP_ID) {
#endif 

		pr_info("[WLC] HW chip id mismatch with target chip id, "
			"NVM programming aborted\n");
		err = E_UNEXPECTED_CHIP_ID;
		goto exit_0;
	}

	/* Determine what has to be programmed depending on version ids */
	pr_info("[WLC] Cut Id: %02X\n", chip_info.cut_id);

#ifdef UBIN
	if (chip_info.cut_id != fw_data.chip_revision) {
#else 
	if (chip_info.cut_id != NVM_TARGET_CUT_ID) {
#endif 

		pr_info("[WLC] HW cut id mismatch with Target cut id, "
			"NVM programming aborted\n");
		err = E_UNEXPECTED_HW_REV;
		goto exit_0;
	}

#ifdef UBIN
	if (chip_info.config_id != fw_data.fw_config_version_id) {
		pr_info("[WLC] Config ID mismatch - running|header: [%04X|%04X]\n",
				chip_info.config_id, fw_data.fw_config_version_id);
#else 
	if (chip_info.config_id != NVM_CFG_VERSION_ID) {
		pr_info("[WLC] Config ID mismatch - running|header: [%04X|%04X]\n",
				chip_info.config_id, NVM_CFG_VERSION_ID);
#endif 

		config_id_mismatch = 1;
	}

#ifdef UBIN
	if (chip_info.nvm_patch_id != fw_data.fw_patch_version_id) {
		pr_info("[WLC] Patch ID mismatch - running|header: [%04X|%04X]\n",
				chip_info.nvm_patch_id, fw_data.fw_patch_version_id);
#else 
	if (chip_info.nvm_patch_id != NVM_PATCH_VERSION_ID) {
		pr_info("[WLC] Patch ID mismatch - running|header: [%04X|%04X]\n",
				chip_info.nvm_patch_id, NVM_PATCH_VERSION_ID);
#endif 

		patch_id_mismatch = 1;
	}

	if (config_id_mismatch == 0 && patch_id_mismatch == 0) {
		pr_info("[WLC] NVM programming is not required, both cfg and patch "
			"are up to date\n");
		err = OK;
		goto exit_0;
	}

	/*
	 * Program both cfg and patch in case one of the two needs
	 * to be programmed
	 */
	err = wlc_nvm_write();
	if (err != OK) {
		pr_err("[WLC] NVM programming failed\n");
		err = E_NVM_WRITE;
		goto exit_0;
	}


	pr_info("[WLC] NVM programming completed, now checking patch "
		"and cfg id\n");

	if (get_wlc_chip_info(&chip_info) != OK) {
		pr_err("[WLC] Error in reading wlc_chip_info\n");
		err = E_BUS_R;
		goto exit_0;
	}

#ifdef UBIN
	if ((chip_info.config_id == fw_data.fw_config_version_id) &&
		(chip_info.nvm_patch_id == fw_data.fw_patch_version_id)) {
#else 
	if ((chip_info.config_id == NVM_CFG_VERSION_ID) &&
		(chip_info.nvm_patch_id == NVM_PATCH_VERSION_ID)) {
#endif 

		pr_info("[WLC] NVM patch and cfg id is OK\n");
		pr_info("[WLC] NVM Programming is successful\n");
	} else {
		err = E_NVM_DATA_MISMATCH;

#ifdef UBIN
		if (chip_info.config_id != fw_data.fw_config_version_id) {
#else 
		if (chip_info.config_id != NVM_CFG_VERSION_ID) {
#endif 

			pr_err("[WLC] Config Id mismatch after NVM programming\n");
		}
		
#ifdef UBIN
		if (chip_info.nvm_patch_id != fw_data.fw_patch_version_id) {
#else 
		if (chip_info.nvm_patch_id != NVM_PATCH_VERSION_ID) {
#endif 

			pr_err("[WLC] Patch Id mismatch after NVM programming\n");
		}
		pr_info("[WLC] NVM Programming failed\n");
	}

exit_0:
	system_reset();
	pr_info("[WLC] NVM programming exited\n");
	count = snprintf(buf, PAGE_SIZE, "{ %08X }\n", err);
	return count;
}

#ifdef UBIN
int u8_to_u32_be(u8 *src, u32 *dst)
{
	*dst = (u32)(((src[0] & 0xFF) << 24) + ((src[1] & 0xFF) << 16) +
		((src[2] & 0xFF) << 8) + (src[3] & 0xFF));
	return OK;
}

int u8_to_u16_be(u8 *src, u16 *dst)

{

	*dst = (u16)(((src[0] & 0x00FF) << 8) + (src[1] & 0x00FF));

	return OK;
}

unsigned int calculate_crc(unsigned char *message, int size)
{
	int i, j;
	unsigned int byte, crc, mask;
	i = 0;
	crc = 0xFFFFFFFF;

	while (i < size) {
		byte = message[i];
		crc = crc ^ byte;
		for (j = 7; j >= 0; j--) {
			mask = -(crc & 1);
			crc = (crc >> 1) ^ (0xEDB88320 & mask);
		}
	i = i + 1;
	}
	return ~crc;
}


int parse_ubin_file(u8 *ubin_data, int ubin_size, struct firmware_file *fw_data)
{
   	int index =0;
  	u32 temp =0;
 	u16 u16_temp =0;
	u32 crc = 0;
 	int patch_data_found=0;
 	int config_data_found =0;
 	u8 u8_temp =0;

 	crc = calculate_crc(ubin_data + 4, ubin_size-4);

	if (crc == (u32)((ubin_data[0] << 24) + (ubin_data[1] << 16) + (ubin_data[2] << 8) + ubin_data[3]))
		pr_info("[WLC] CRC successful for Ubin file ...\n");
	else {
		pr_info("[WLC] CRC failed for Ubin file ...\n");
		return E_FILE_PARSE;
	}

   	index += 4;

	if(ubin_size <= (BIN_HEADER_SIZE + SECTION_HEADER_SIZE) || (ubin_data == NULL)) {
		pr_info("[WLC] Read only %d instead of %d... ERROR %08X\n", ubin_size, BIN_HEADER_SIZE, E_FILE_PARSE);
		return E_FILE_PARSE;
	}

    	u8_to_u32_be(&ubin_data[index], &temp);

    	if (temp != BIN_HEADER) {
    		pr_info("[WLC] Wrong Signature 0x%08X ... ERROR %08X\n", temp, E_FILE_PARSE );
		return E_FILE_PARSE;
	}

	pr_info("[WLC] BIN HEADER Signature is correct and matched 0x%08X ... ", temp );

	index +=5;

    	u16_temp = (ubin_data[index+1] <<8) + ubin_data[index];

    	if (u16_temp != CHIP_ID) {
    		pr_info("[WLC] Wrong Chip ID %04X ... ERROR %08X\n", u16_temp, E_FILE_PARSE );
		return E_FILE_PARSE;
	}

	pr_info("[WLC] Chip ID: %04X\n", u16_temp);
	fw_data->chip_id = u16_temp;

	index +=18;

	u8_temp = ubin_data[index];
	fw_data->chip_revision = u8_temp;
	pr_info("[WLC] CUT ID: %02X\n", u8_temp);

	index +=9;

    	while (index < ubin_size){
        		u8_to_u32_be(&ubin_data[index], &temp);

       		if (temp != SECTION_HEADER) {
       			pr_info("[WLC] Wrong Section Signature %08X ... ERROR %08X\n", temp, E_FILE_PARSE);
			return E_FILE_PARSE;
		}
		pr_info("[WLC]  SECTION_HEADER Signature is correct and matched 0x%08X ... ", temp );

        		index +=4;
       		u8_to_u16_be(&ubin_data[index], &u16_temp);

        		if(u16_temp == WLC_FW_PATCH) {
            		if (patch_data_found) {
            			pr_info("[WLC] Cannot have more than one patch  ... ERROR %08X\n", E_FILE_PARSE);
				return E_FILE_PARSE;
        			}

        			patch_data_found =1;

        			index +=2;
		u8_to_u16_be(&ubin_data[index], &u16_temp);
		fw_data->fw_patch_version_id = u16_temp;

			index +=2;
			u8_to_u32_be(&ubin_data[index], &temp);

        			fw_data->fw_patch_size = temp;
			if (fw_data->fw_patch_size == 0) {
				pr_info("[WLC] patch data cannot be empty ... ERROR %08X\n", E_FILE_PARSE);
				return E_FILE_PARSE;
			}

            fw_data->fw_patch_data = (u8 *)malloc(fw_data->fw_patch_size * sizeof(u8));

            if (fw_data->fw_patch_data == NULL) {
				pr_info("[WLC] Error allocating memory... ERROR %08X\n", E_FILE_PARSE);
				return E_FILE_PARSE;
				}

            index +=12;
            memcpy(fw_data->fw_patch_data, &ubin_data[index], fw_data->fw_patch_size);
            index += fw_data->fw_patch_size;

    			}
	u8_to_u32_be(&ubin_data[index], &temp);
	if (temp != SECTION_HEADER) {
		pr_info("[WLC] Wrong Section Signature ... ERROR %08X\n", E_FILE_PARSE);
			return E_FILE_PARSE;
		}

        	index +=4;
        	u8_to_u16_be(&ubin_data[index], &u16_temp);

        	if(u16_temp == WLC_FW_CONFIG) {
           	if (config_data_found) {
            	pr_info("[WLC] Cannot have more than one config  ... ERROR %08X\n", E_FILE_PARSE);
		return E_FILE_PARSE;
        	}

	config_data_found =1;

	index +=4;
        	u8_to_u32_be(&ubin_data[index], &temp);
        	fw_data->fw_config_size = temp;
		if (fw_data->fw_config_size == 0) {
			pr_info("[WLC] Config data cannot be empty  ... ERROR %08X\n", E_FILE_PARSE);
			return E_FILE_PARSE;
			}

           fw_data->fw_config_data = (u8 *)malloc(fw_data->fw_config_size * sizeof(u8));

           if (fw_data->fw_config_data == NULL) {
            	pr_info("[WLC] Error allocating memory  ... ERROR %08X\n", E_FILE_PARSE);
		return E_FILE_PARSE;
		}

	 index +=12;
            u16_temp = (ubin_data[index+1]<<8) + ubin_data[index];
            pr_info("[WLC] Cfg Id : %04X\n",u16_temp);
	 fw_data->fw_config_version_id = u16_temp;

            memcpy(fw_data->fw_config_data , &ubin_data[index], fw_data->fw_config_size);
            index += fw_data->fw_config_size;

	}
	}
    	pr_info("[WLC] Ubin file parsed successfully \n");
	return OK;
}
#endif
