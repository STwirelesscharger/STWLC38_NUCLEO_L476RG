/****************************************************************************
 **                            STMicroelectronics                          **
 ****************************************************************************
 *                                                                          *
 * STWLC38 Wireless Charger Driver (WLC)                                    *
 *                                                                          *
 * This is reference driver for STWLC38 wireless charger                    *
 *                                                                          *
 ****************************************************************************/

/***************************************************************************
 * File Name:		STWLC38.h
 * Description:	This file contains all the definitions and structs
 *					used generally by the driver
 ****************************************************************************/


/****************************************************************************
 * Preprocessor definitions
 ****************************************************************************/
#ifndef STWLC38_H
#define STWLC38_H


/****************************************************************************
 * Included files
 ****************************************************************************/
#include "main.h"


/****************************************************************************
 * Macro definitions
 ****************************************************************************/
/* NVM constants */
#define NVM_SECTOR_SIZE_BYTES			256
#define NVM_PATCH_START_SECTOR_INDEX	0
#define NVM_CFG_START_SECTOR_INDEX		126

/* FW registers */
#define FWREG_CHIP_ID_ADDR				0x0000
#define FWREG_OP_MODE_ADDR				0x000E
#define FWREG_SYS_CMD_ADDR				0x0020
#define FWREG_NVM_SECTOR_INDEX_ADDR		0x0024
#define FWREG_TX_CMD_ADDR				0x0110
#define FWREG_AUX_DATA_00_ADDR			0x0180
#define FWREG_NVM_PWD_ADDR				0x0022

/* SYSREG registers */
#define HWREG_HW_VER_ADDR				0x2001C002
#define HWREG_TM_CONFIG_ADDR			0x2001C166
#define HWREG_RST_ADDR					0x2001F200

#define WRITE_READ_OPERATION			0x01
#define WRITE_OPERATION					0x02
#define OPCODE_WRITE					0xFA
#define MIN_WR_BYTE_LENGTH				5
#define MIN_W_BYTE_LENGTH				4
#define CMD_STR_LEN						1024

#define I2C_CHUNK_SIZE					256
#define AFTER_SYS_RESET_SLEEP_MS		50
#define GENERAL_SLEEP_MS				10

/* Error codes */
#define OK								0x00000000
#define E_BUS_R							0x80000001
#define E_BUS_W							0x80000002
#define E_BUS_WR						0x80000003
#define E_UNEXPECTED_OP_MODE			0x80000004
#define E_NVM_WRITE						0x80000005
#define E_INVALID_INPUT					0x80000006
#define E_MEMORY_ALLOC					0x80000007
#define E_UNEXPECTED_HW_REV				0x80000008
#define E_TIMEOUT						0x80000009
#define E_NVM_DATA_MISMATCH				0x8000000A
#define E_UNEXPECTED_CHIP_ID			0x8000000B
#define E_NO_FILE						0x8000000E
#define E_FILE_PARSE					0x8000000F

//#define UBIN
#ifdef UBIN
#define BIN_HEADER_SIZE					(32 + 4) /* /< fw ubin main header size including crc */
#define SECTION_HEADER_SIZE				20 /* /< fw ubin section header size */
#define BIN_HEADER						0xBABEFACE /* /< fw ubin main header identifier constant */
#define SECTION_HEADER					0xB16B00B5 /* /< fw ubin section header identifier constant */
#endif

#define CHIP_ID 						0X0026

/* Driver version string format */
#define WLC_DRV_VERSION					"1.2.1"

/* Primitive type definition*/
#define u8	uint8_t
#define u16	uint16_t
#define u32	uint32_t

#define PAGE_SIZE						1024
//#define DEBUG_I2C
/****************************************************************************
 * Enums
 ****************************************************************************/
typedef enum {
	FW_OP_MODE_SA	= 1,
	FW_OP_MODE_RX	= 2,
	FW_OP_MODE_TX	= 3
} fw_op_mode_t;

#ifdef UBIN
typedef enum {
	WLC_FW_PATCH	= 0x0010,
	WLC_FW_CONFIG	= 0x0011,

} fw_section_t;
#endif

/****************************************************************************
 * Structures
 ****************************************************************************/
struct wlc_chip_info {
	u16 chip_id;
	u8 chip_revision;
	u8 customer_id;
	u16 project_id;
	u16 nvm_patch_id;
	u16 ram_patch_id;
	u16 config_id;
	u16 pe_id;
	u8 cut_id;
};

#ifdef UBIN
struct firmware_file {
	u16 chip_id;
	u16 fw_config_version_id;
	u16 fw_patch_version_id;
	u32 fw_patch_size;
	u8 *fw_patch_data;
	u32 fw_config_size;
	u8 *fw_config_data;
	u8  chip_revision;
};
#endif
/****************************************************************************
 * Function Prototypes
 ****************************************************************************/
void pr_err(char *msg, ...);
void pr_info(char *msg, ...);

int chip_info_show(char *buf);
int nvm_program_show(char *buf);


#endif
