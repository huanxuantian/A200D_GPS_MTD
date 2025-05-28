#ifndef _GB905_PARAM_H
#define	_GB905_PARAM_H

#ifdef __cplusplus
extern "C" {
#endif
#include "utils.h"

typedef struct{
	unsigned short id;
	unsigned char len;
	unsigned int content;
}__packed msg_params_t;

void report_keyparam(unsigned char index);
void report1_keyparam(unsigned char index);
void gb905_get_params_treat(unsigned char index,unsigned char *buf,int len);
unsigned char gb905_set_params_treat(unsigned char index,unsigned char *buf,int len);


#ifdef __cplusplus
}
#endif

#endif