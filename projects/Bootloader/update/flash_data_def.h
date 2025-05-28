#ifndef __FLASH_DATA_DEF_H__
#define __FLASH_DATA_DEF_H__

#ifdef __cplusplus
extern "C" {
#endif

#define IAP_BOOT_VERSION	"1.0.1"
#define IAP_BOARD_VERSION	"A200D"

#define FLASH_INIT_FLAG	0x55AA5A5A

#define FLASH_INIT_FLAG_START	0x000000
#define FLASH_INIT_FLAG_OFFSET	0x10
#define FLASH_INIT_FLAG_ADDR	(FLASH_INIT_FLAG_START+FLASH_INIT_FLAG_OFFSET)

#define FLASH_INIT_FLAG_SIZE	0x20

#if FLASH_INIT_FLAG_SIZE < (FLASH_INIT_FLAG_OFFSET+4)
#error "FLASH_INIT_FLAG_SIZE OR FLASH_INIT_FLAG_OFFSET NOT FIXED ERROR"
#endif

#define APP_START_ADDRESS 0x08002000U
#define APP_RW_BLOCK_SIZE   4096
#define APP_MAX_SIZE		(120*1024)

#define APP_MAX_ADDRESS (APP_START_ADDRESS+APP_MAX_SIZE-APP_RW_BLOCK_SIZE)

//user base param area
#define FLASH_PARAM_START	0x001000
#define FLASH_PARAM_HEAD_LEN	24
#define FLASH_PARAM_SIZE	4096
#define FLASH_PARAM_PAGES	1

//update flag area
#define FLASH_UPDATE_START	0x008000
#define FLASH_UPDATE_LEN	32
#define FLASH_UPDATE_SIZE	4096
#define FLASH_UPDATE_PAGES	1
#define FLASH_UPDATE_END	(FLASH_UPDATE_START+FLASH_UPDATE_LEN)
//update flag bak area
#define FLASH_UPDATE_WBACKUP_START	0x009000
#define FLASH_UPDATE_WBACKUP_END	(FLASH_UPDATE_WBACKUP_START+FLASH_UPDATE_LEN)

#define UPDATA_START_FLAG_OK  0xAA55
#define UPDATA_END_FLAG_FI  0x55
#define UPDATA_END_FLAG_OK  0xAA
#define UPDATA_SUCESS_OK  0x55
#define UPDATA_SUCESS_FAILED  0x00
//struct for update flag
typedef struct{
    unsigned short start_flag;//0xAA55
    unsigned char end_flag;//FF -> AA
    unsigned char sucess;//FF -> 01
    unsigned int file_size;//>0 <APP_MAX_SIZE
    unsigned char device_type;//0:IAP,1:
    unsigned char update_type;//0:app
    unsigned char reserve[2];
    unsigned short reserve_flag;
    unsigned short crc16;
}spi_update_flag;//16Bytes update flag

//update file area
#define FLASH_UPDATE_DATA_START	0x100000
#define FLASH_UPDATE_DATA_APP_SIZE		(128*1024)
//other data reserve
#define FLASH_UPDATE_DATA_MAX_SIZE		(512*1024)

#define APP_CODE_STR_FLAG "SHINKI_A200"



#ifdef __cplusplus
}
#endif

#endif
