#include "n32g43x.h"
#include <rtthread.h>
#include	<stdio.h>
#include	<string.h>

 
#include	<stdlib.h>
//#include	<assert.h>

#include	"cb_buffer.h"


bool cb_full(CircleBufferMngr *pmngr)
{
	if(!cb_is_vaild(pmngr)) return true;
	return (bool)(pmngr->buflen == pmngr->datalen);    
}
 
bool cb_empty(CircleBufferMngr *pmngr)
{
	if(!cb_is_vaild(pmngr)) return true;
	return (bool)(0 == pmngr->datalen);    
}

bool cb_is_vaild(CircleBufferMngr *pmngr){
	if(pmngr ==NULL) return false;
	return pmngr->bufptr!=NULL&& pmngr->buflen >0;
}

bool cb_init_static(CircleBufferMngr* Static_Mngr,u8* buffer,u32 buflen)
{	  
	if (0 == buflen|| buffer==NULL)
		 return false;
	if(Static_Mngr ==NULL) return false;
	if(cb_is_vaild(Static_Mngr)){return false;}
	memset((void*)Static_Mngr, 0, sizeof(CircleBufferMngr));
	  
	Static_Mngr->bufptr = (u8*)&buffer[0];
	Static_Mngr->buflen = buflen;
	memset((void*)Static_Mngr->bufptr, 0, buflen);
	Static_Mngr->ppmngr_read_isusing =0;	
	Static_Mngr->ppmngr_read_havechange =0;
 
	return true;	
}

bool cb_deinit_static(CircleBufferMngr* Static_Mngr){
	if(cb_is_vaild(Static_Mngr)){
		Static_Mngr->bufptr = NULL;
		Static_Mngr->buflen =0;
		memset((void*)Static_Mngr, 0, sizeof(CircleBufferMngr));
	}
	return true;
}

u32 cb_read(CircleBufferMngr *pmngr,u8 *outbuf,s32 buflen)
{
	u32 readlen = 0, tmplen = 0;

	if(!cb_is_vaild(pmngr)) return 0;
	if(cb_empty(pmngr))
		return 0;
	    
	readlen = buflen > pmngr->datalen ? pmngr->datalen : buflen;
	tmplen = pmngr->buflen - pmngr->readpos;
	  
	if(NULL != outbuf)
	{
		if(readlen <= tmplen)
		{
			memcpy((void*)outbuf,(void*)&pmngr->bufptr[pmngr->readpos],readlen);
		}
		else
		{
			memcpy((void*)outbuf,(void*)&pmngr->bufptr[pmngr->readpos],tmplen);
			  
			memcpy((void*)&outbuf[tmplen],(void*)pmngr->bufptr,readlen - tmplen);
		}
	}
	  
	pmngr->readpos = (pmngr->readpos + readlen) % pmngr->buflen;
	pmngr->datalen -= readlen;
	  
	return readlen;
}

 
u32 cb_read_no_offset(CircleBufferMngr *pmngr,unsigned char *outbuf,s32 buflen)
{
	u32 readlen = 0, tmplen = 0;
 	if(!cb_is_vaild(pmngr)) return 0;
	if(cb_empty(pmngr))
		return 0;
 
	readlen = buflen > pmngr->datalen ? pmngr->datalen : buflen;
	tmplen = pmngr->buflen - pmngr->readpos;
	  
	if(NULL != outbuf)
	{
		if(readlen <= tmplen)
		{
			memcpy(
				(void*)outbuf,
				  (void*)&pmngr->bufptr[pmngr->readpos],
				  readlen);
		}
		else
		{
			 memcpy(
					(void*)outbuf,
					(void*)&pmngr->bufptr[pmngr->readpos],
					tmplen);
			  
			 memcpy(
				  (void*)&outbuf[tmplen],
				  (void*)pmngr->bufptr,
				  readlen - tmplen);
		}
	}
	  
	return readlen;
}

void cb_read_move_offset(CircleBufferMngr *pmngr,s32 offset)
{
	u32 len;
	pmngr->ppmngr_read_isusing =1;
	if(!cb_is_vaild(pmngr)) return ;
	len = cb_datalen(pmngr);
	 
	if(len <= offset)
	{
		pmngr->readpos = pmngr->writepos;
		pmngr->datalen = 0;
	}
	else
	{
		pmngr->readpos = (pmngr->readpos + offset) % pmngr->buflen;
		pmngr->datalen -= offset;
	} 
	pmngr->ppmngr_read_isusing =0;
}
  
u32 cb_write(CircleBufferMngr *pmngr, u8 *datptr, s32 datlen)
{
	u32 writelen = 0, tmplen = 0;
	if(!cb_is_vaild(pmngr)) return 0;
	if(cb_full(pmngr))
		return 0;
 
	tmplen = pmngr->buflen - pmngr->datalen;
	writelen = tmplen > datlen ? datlen : tmplen;
 
	if(pmngr->writepos < pmngr->readpos)
	{
		memcpy((void*)&pmngr->bufptr[pmngr->writepos],(void*)datptr,writelen);
	}
	else
	{
		tmplen = pmngr->buflen - pmngr->writepos;
		if(writelen <= tmplen)
		{
			memcpy((void*)&pmngr->bufptr[pmngr->writepos],(void*)datptr,writelen);
		}
		else
		{
			memcpy((void*)&pmngr->bufptr[pmngr->writepos],(void*)datptr,tmplen);
 
			memcpy((void*)pmngr->bufptr,(void*)&datptr[tmplen],writelen - tmplen);
		}
	}
	if(pmngr->ppmngr_read_isusing ==0)
	{	pmngr->writepos = (pmngr->writepos + writelen) % pmngr->buflen;
		pmngr->datalen += writelen;
	    pmngr->ppmngr_read_havechange =1;
		return writelen;
	}
	else 
		return 0;


}
 
 
u32 cb_datalen(CircleBufferMngr *pmngr)
{
	if(!cb_is_vaild(pmngr)) return 0;
	return pmngr->datalen;
}

u32 cb_data_read_ptr_reset(CircleBufferMngr *pmngr,void** pdataptr)
{
	u32 readlen = 0, tmplen = 0;
	if(!cb_is_vaild(pmngr)) return 0;
	
	readlen = pmngr->datalen;
	tmplen = pmngr->buflen - pmngr->readpos;
	
	if(readlen <= tmplen)
	{
		*pdataptr = (void*)&pmngr->bufptr[pmngr->readpos];
		//should be error here (must not set data before use pdataptr)
		cb_read_move_offset(pmngr,readlen);
		return readlen;
	}
	
	*pdataptr = (void*)&pmngr->bufptr[pmngr->readpos];
	//should be error here (must not set data before use pdataptr)
	cb_read_move_offset(pmngr,tmplen);
	return tmplen;
}

u32 cb_freelen(CircleBufferMngr *pmngr)
{
	u32 len;
	if(!cb_is_vaild(pmngr)) return 0;
	
	len = pmngr->buflen - pmngr->datalen;

	return len;
}

