#ifndef __PKG_UTILS_H_
#define __PKG_UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <rtdbg.h>
#include <rtthread.h>

#ifndef __packed
#define __packed    __attribute__((packed))
#endif

#define DbgFuncEntry() 
#define DbgFuncExit() 

#define DbgWarn LOG_W
#define DbgError LOG_E

#define DbgPrintf LOG_I
#define DbgGood LOG_D


#define DbgHexGood(X,...) ((void*)0)

#ifndef MIN
#define		MIN(A,B)			((A)<(B)?(A):(B))
#endif
#ifndef MAX
#define		MAX(A,B)			((A)>(B)?(A):(B))
#endif
#ifndef ARRAY_SIZE
#define		ARRAY_SIZE(arr)		(sizeof(arr) / sizeof((arr)[0]))
#endif

#ifndef offsetof
#define 	offsetof(STRUCTURE,FIELD)		((long)((char*)&((STRUCTURE*)0)->FIELD))
#endif

#define EndianReverse16(val)	((((unsigned short)(val) & 0xff00) >> 8) | (((unsigned short)(val) & 0x00ff) << 8))
							
#define EndianReverse32(val)	((((unsigned int)(val) & 0xff000000) >> 24) | \
								 (((unsigned int)(val) & 0x00ff0000) >> 8) | \
								 (((unsigned int)(val) & 0x0000ff00) << 8) | \
								 (((unsigned int)(val) & 0x000000ff) << 24))

#define EndianReverse64(val)	((((unsigned long long)(val) & 0xff00000000000000ULL) >> 56) | \
								 (((unsigned long long)(val) & 0x00ff000000000000ULL) >> 40) | \
								  (((unsigned long long)(val) & 0x0000ff0000000000ULL) >> 24)|\
								  (((unsigned long long)(val) & 0x000000ff00000000ULL) >> 8)|\
								  (((unsigned long long)(val) & 0x00000000ff000000ULL) << 8) | \
								  (((unsigned long long)(val) & 0x0000000000ff0000ULL) << 24) | \
								 (((unsigned long long)(val) & 0x000000000000ff00ULL) << 40) | \
								 (((unsigned long long)(val) & 0x00000000000000ffULL) << 56))

typedef struct{
	unsigned char * buf;					
	int len;									
}__packed buff_mgr_t;
typedef struct{
	buff_mgr_t raw;												
	buff_mgr_t now;					
}__packed double_buff_mgr_t;

#include	"bcd.h"
#include	"cb_buffer.h"


#ifdef __cplusplus
}
#endif

#endif /* __PKG_UTILS_H_ */