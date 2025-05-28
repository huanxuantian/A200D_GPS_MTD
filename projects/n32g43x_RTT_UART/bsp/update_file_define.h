#ifndef _UPDATE_FILE_SYS_H
#define	_UPDATE_FILE_SYS_H

#ifdef __cplusplus
extern "C" {
#endif
#include "flash_data_def.h"
#define UPDATE_SHINKI_APP_PATH "SPI-0"
#define FLASH_VER_OFFSET	0x200
#define FLASH_TEST_OFFSET	0x16500

#define FLASH_UPDATE_START_ADDRESS FLASH_UPDATE_DATA_START	

#ifdef __cplusplus
}
#endif

#endif