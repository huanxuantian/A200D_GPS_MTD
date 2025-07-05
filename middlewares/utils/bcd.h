#ifndef _BCD_H
#define	_BCD_H


#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

unsigned char xor8_computer(unsigned char * p_data, unsigned int len);

int decimal2bcd(unsigned int dec,unsigned char *buf, int len);
unsigned int bcd2decimal(unsigned char *buf, int len);
bool is_bcd(unsigned char *buf, int len);

int str2BCDByteArray(char *str,unsigned char *bcdByteArray);
int bcdByteArray2Str(unsigned char *bcdByteArray,int len,char *str);



void bcdtime_to_char(unsigned char *bcd_time,unsigned char* str_time);
void bcdtime_to_dvr_time(unsigned char *bcd_time,unsigned char* str_time);

void debug_print_block_data(uint32_t base_address,uint8_t* data_ptr,uint32_t block_size);

void debug_utc(time_t time_utc);
uint32_t OSTimeGet();
uint32_t OSTimeGetMS();
unsigned short usMBCRC16( unsigned char * pucFrame, unsigned short usLen );
#ifdef __cplusplus
}
#endif



#endif

