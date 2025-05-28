
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "n32g43x.h"
#include "n32g43x_flash.h"
#include "spi_flash.h"
#include "public.h"
#include "flash_data_def.h"

#define N32_FLASH_PAGE_SIZE	(0x800)

void initEWD_IO(void)
{
	
}

void wdg(void)
{
  //do nothing here now
}

/*
* 块擦除 2k对齐位置擦除
*/
int FlashSecEraseSec(uint32_t iAddr)
{
  if (FLASH_COMPL != FLASH_EraseOnePage(iAddr))
	{
		return 1;
	}
	return 0;
}

/*
* 块擦除 只有在块地址上才能擦除成功
*/
int FlashSecEraseAddr(uint32_t iAddr)
{
	uint32_t i;
    if(iAddr%N32_FLASH_PAGE_SIZE==0)
    {
        //if (STM205XX_ADDR_SEC_MAP[i][0] == iAddr)
        {
            return FlashSecEraseSec(iAddr);
		}
	}
	return 0;
}


#if 0
/****************
内部FLASH初始化
**********************/
bool Flashinit(void)
{
	bool err = true;
  	u32 Sector;
	u8 Status;
    //check opt lock init clock 
	return err;
}
#endif

/****************
把数据写入内部flash指定地址
入口参数：	u8 *databuf,数据地址
			u32 Address,FLASH地址
			u8 len,	数据的长度 
返回值：  成功返回TRUE ，失败返回FALSE
说明：把待写入区域读出修改后再写入，只有地址在块首是就会自动擦除
由于N32是4字节对齐一次写入4字节，因此如果len不为4的倍数时需要特殊处理
**********************/
int FlashWrite(u8 *databuf,u32 Address,u32 len)
{	 
	uint8_t divs=len%4;
	uint16_t w_count = (len/4) + (divs?1:0);
	uint32_t i,j,dat;
	FLASH_STS status;
	
	if((Address < APP_START_ADDRESS) ||((Address + len) > APP_MAX_ADDRESS))//地址越界
	{
	   return 0;
	}
	FLASH_Unlock();
	for(i = 0;i < w_count; i++)
	{	
        
		wdg();
		FlashSecEraseAddr(Address+i*4); // 块地址上删除块
		if(divs>0 && i==w_count-1)
		{
				dat =databuf[i*4];
				if(dat >1){//had second byte
					dat +=(databuf[i*4+1]<<8);
					if(dat >2){//has third byte
						dat +=(databuf[i*4+2]<<16);
						dat += 0xFF000000;
					}else{
						dat += 0xFFFF0000;
					}
				}else{
					dat += 0xFFFFFF00;
				}
		}else{
			dat =databuf[i*4] +(databuf[i*4+1]<<8)+(databuf[i*4+2]<<16)+(databuf[i*4+3]<<24);
		}
		#if 1
		j =3;
		do{
			j--;
			status = FLASH_ProgramWord(Address+i*4,dat);
		}while((j >0) &&(status != FLASH_COMPL));
		if ((status != FLASH_COMPL) || (databuf[i*4] !=  *(u8*)(Address+i*4)))
		{
			FLASH_Lock ();
			return 0;
		}
		#endif
        
	}
	FLASH_Lock ();
	return 1;  
}

u8 AppSpiFlashRead(u8 *pbuf,u32 ReadAddr,u16 len){
	sFLASH_ReadBuffer(pbuf, ReadAddr, len);
	return 1;
}
extern void DelyMs(u32 a);
u8 AppSpiFlashWrite(u8 *pbuf,u32 WriteAddr,u32 len,u8 erasetag)
{
	if(erasetag){
		sFLASH_EraseSector(WriteAddr);
		DelyMs(20);
	}
	sFLASH_WriteBuffer(pbuf, WriteAddr, len);
	return 1;
}
/****************
把数据从内部flash指定地址读出
**********************/
int FlashRead(u8 *databuf,int32_t Address,u32 len)
{
	u32 i;
	for (i = 0;i < len;i++)
	{
		databuf[i] =  *(u8*)Address;
        Address += 1;
	}
	return 1;
}

//update flag info write
int AppSpiFlashWriteInfo(unsigned char *databuf,unsigned int Address,unsigned int  len){

	u8 InfoAriaCach[FLASH_UPDATE_LEN];
    if (((Address) >= FLASH_UPDATE_WBACKUP_START) && ((Address + len) <= (FLASH_UPDATE_WBACKUP_END )))
    {
        AppSpiFlashRead(InfoAriaCach,FLASH_UPDATE_WBACKUP_START,sizeof(InfoAriaCach));
        memcpy(InfoAriaCach ,databuf,len);
        return AppSpiFlashWrite(InfoAriaCach,FLASH_UPDATE_WBACKUP_START,sizeof(InfoAriaCach),1);
    }

	return 1;
}

/************
应用程序更新函数
从备份程序区拷贝到程序区
**************/
int updataProgram (unsigned int len)
{
	u32 address;
	u8 code_buf[1024*4];
  	u8 update_flag_buff[FLASH_UPDATE_LEN];
  	spi_update_flag* update_flag;
	
	address = 0;
	//判断程序是否有效
	AppSpiFlashRead (&code_buf[0],FLASH_UPDATE_DATA_START,2048);
	if(memcmp(&code_buf[512],APP_CODE_STR_FLAG,11)!=0X00)
	{//缓冲区数据
		return 0;
	}
	send_debug_data("\r\n");
	send_debug_data(APP_CODE_STR_FLAG);
	send_debug_data("\r\n");
	//sendcom_byte('5');
	{
		for (address=0;address < len;)
		{
			//sendcom_byte('6');
			if(AppSpiFlashRead (&code_buf[0],address+FLASH_UPDATE_DATA_START,1024*4)<=0)
			{
				wdg();
				//sendcom_byte('7');
				if(AppSpiFlashRead (&code_buf[0],address+FLASH_UPDATE_DATA_START,1024*4)<=0)
					return 0 ;
			}
			wdg();
			//sendcom_byte('8');
			if(FlashWrite(&code_buf[0],address+APP_START_ADDRESS,1024*4) <=0)
			{
				//sendcom_byte('9');
				if(FlashWrite(&code_buf[0],address+APP_START_ADDRESS,1024*4) <=0)
					return 0;
			}
			address +=1024*4;
			wdg(); 
			//sendcom_byte('a');			
		}
	 }
	 //生成升级参数并保存
	AppSpiFlashRead((u8 *)&update_flag_buff,FLASH_UPDATE_START,FLASH_UPDATE_LEN);
	update_flag = (spi_update_flag*)update_flag_buff;
	update_flag->start_flag = UPDATA_START_FLAG_OK;
	wdg(); 
	send_debug_data("UPDATE FLAG SAVE\r\n");
	update_flag->end_flag = UPDATA_END_FLAG_OK;
	update_flag->sucess = UPDATA_SUCESS_OK;
	return AppSpiFlashWriteInfo((u8 *)&update_flag_buff,FLASH_UPDATE_WBACKUP_START,FLASH_UPDATE_LEN);
	return 0;
}