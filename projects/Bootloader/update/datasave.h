#ifndef __FLASH_DATA_SAVE_H__
#define __FLASH_DATA_SAVE_H__

#ifdef __cplusplus
extern "C" {
#endif
void initEWD_IO(void);
void wdg(void);
int updataProgram (unsigned int len);

int AppSpiFlashWriteInfo(unsigned char *databuf,unsigned int Address,unsigned int  len);

#ifdef __cplusplus
}
#endif

#endif
