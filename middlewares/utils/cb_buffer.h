#ifndef __CB_BUFFER_H
#define __CB_BUFFER_H

#ifdef __cplusplus
extern "C" {
#endif
#include "n32g43x.h"
#include <rtthread.h>

#include<stdio.h>
#include<string.h>
#include<stdbool.h>

typedef struct {
	u8	*bufptr;
	u32	buflen;
	u32	datalen;
	u32	readpos;
	u32	writepos;
	volatile u32  ppmngr_read_havechange;
	volatile u32  ppmngr_read_isusing;
}CircleBufferMngr;
/*
//Static_Mngr :static manager struct bind to handle
//buffer :static memory bind to manager
//buflen :buffer len 
*/
bool cb_init_static(CircleBufferMngr* Static_Mngr,u8* buffer,u32 buflen);//
/*
//not define or use
*/
bool cb_is_vaild(CircleBufferMngr *pmngr);
bool cb_deinit_static(CircleBufferMngr* Static_Mngr);

bool cb_full(CircleBufferMngr *pmngr);
bool cb_empty(CircleBufferMngr *pmngr);
u32 cb_datalen(CircleBufferMngr *pmngr);
u32 cb_freelen(CircleBufferMngr *pmngr);

u32 cb_read(CircleBufferMngr *pmngr,u8 *outbuf,s32 buflen);
u32 cb_write(CircleBufferMngr *pmngr,u8 *datptr,s32 datlen);

u32 cb_read_no_offset(CircleBufferMngr *pmngr,u8 *outbuf,s32 buflen);
void cb_read_move_offset(CircleBufferMngr *pmngr,s32 offset);

u32 cb_data_read_ptr_reset(CircleBufferMngr *pmngr,void** pdataptr);


#ifdef __cplusplus
}
#endif

#endif 

