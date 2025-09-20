/*****************************************************************************
 * Copyright (c) 2022, Nations Technologies Inc.
 *
 * All rights reserved.
 * ****************************************************************************
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the disclaimer below.
 *
 * Nations' name may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * DISCLAIMER: THIS SOFTWARE IS PROVIDED BY NATIONS "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * DISCLAIMED. IN NO EVENT SHALL NATIONS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ****************************************************************************/

/**
 * @file main.c
 * @author Nations
 * @version V1.2.2
 *
 * @copyright Copyright (c) 2022, Nations Technologies Inc. All rights reserved.
 */
#include "main.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "n32g43x_flash.h"
#include "spi_flash.h"
#include "flash_data_def.h"
#include "datasave.h"
#include "public.h"

__IO uint32_t FlashID = 0;
#if defined (__GNUC__)
void __libc_init_array(void)
{
    /* we not use __libc init_aray to initialize C++ objects */
}
#endif

uint8_t check_buff[FLASH_INIT_FLAG_SIZE];
#ifdef TEST_BACKUP_FLASH
int backup_flash()
{
	void* address;
	int offset=0;
	int size=0;
	uint32_t spi_address=FLASH_UPDATE_DATA_START;
	unsigned char update_flag_data[FLASH_UPDATE_LEN];
	unsigned char update_data[APP_RW_BLOCK_SIZE];
	do{
		memset(update_data,0,sizeof(update_data));
		address = (void*)(APP_START_ADDRESS+offset);
		memcpy(update_data,address,APP_RW_BLOCK_SIZE);
		if(update_data[0]==0xFF && update_data[1]==0xFF &&
			 update_data[2]==0xFF && update_data[3]==0xFF ){
				break;
		}
		//copy data to spi flash
		spi_address = FLASH_UPDATE_DATA_START + (offset);
		sFLASH_EraseSector(spi_address);
		DelyMs(20);
		sFLASH_WriteBuffer(update_data,spi_address,APP_RW_BLOCK_SIZE);
		offset +=APP_RW_BLOCK_SIZE;
	}while(offset<APP_MAX_SIZE);
	size = offset;//0x01a000
	return size;
}
#endif

#define TEST_FILE_LAST_BLOCK_OFFSET 0x019000
#define TEST_FILE_SIZE 0x019048
#define TEST_FILE_FUSE_SIZE 0x019080
#ifdef TEST_BACKUP_FLASH
void read_back_page(){
	int size=0;
	int offset=0;
	uint32_t spi_address=FLASH_UPDATE_DATA_START;
	unsigned char buffer_data[APP_RW_BLOCK_SIZE];
	sFLASH_ReadBuffer(buffer_data, FLASH_UPDATE_DATA_START+TEST_FILE_LAST_BLOCK_OFFSET, sizeof(buffer_data));
	size +=APP_RW_BLOCK_SIZE;
	#if 0 //test data only
	offset = TEST_FILE_SIZE - TEST_FILE_LAST_BLOCK_OFFSET;
	//test write crc16
	buffer_data[offset++]=0x34;
	buffer_data[offset++]=0x33;
	//fill 0x1A
	while(offset%128!=0){
		buffer_data[offset++]=0x1A;
	}
	sFLASH_EraseSector(FLASH_UPDATE_DATA_START+TEST_FILE_LAST_BLOCK_OFFSET);
	DelyMs(20);
	sFLASH_WriteBuffer(buffer_data,FLASH_UPDATE_DATA_START+TEST_FILE_LAST_BLOCK_OFFSET,APP_RW_BLOCK_SIZE);
	#endif
	
	return;
}
#endif
u16 CalCrc_xmode(u8 *ptr , int count,u16 crc_s,u16 *CRC_1,u16 *CRC_2,u16 *CRC_3)
{
	u16  crc = 0,crc1,crc2,crc3 ;
	char ii;
	crc = crc_s;
	crc1=crc_s;
	crc2=crc_s;
	crc3=crc_s;
	

	while(--count >= 0)
	{
	    crc = crc^(int) *ptr++<<8;
	    ii = 8;
	    do
	    {
	        if(crc & 0x8000)
	        {
	            crc = crc<<1^0x1021;
			}
			else
			{
			    crc = crc<<1;
			}
		}while(--ii);
		crc1=crc2;
		crc2=crc3;
		crc3=crc;
	}
	*CRC_1 = crc1;
	*CRC_2 = crc2;
	*CRC_3 = crc3;
	return(crc);
}

u8 judge_code_sure(u32 len)
{
	u32 i;
	u8 dd_buf[4096],*dd;
	u16 crc_a1=0,crc_a2=0,crc_a3=0,crc_code1,crc_code2,crc_code3,crc_ret;
//	u32 j,bp,real_len;
	u32 j,real_len;
	memset(dd_buf,0,sizeof(dd_buf));
	sFLASH_ReadBuffer((u8 *)&dd_buf[0],FLASH_UPDATE_DATA_START+len-130,256+2);
	real_len = len;
	j=0;			
	dd = (unsigned char *)(&dd_buf[130-1]);
	do{
		if((*dd) ==0X1A)
		{	real_len--;	
			dd--;
		}
		else 
			break;
	}while(++j <130);
	
	crc_code1 =(*(dd-1))*0x100+ (*(dd));
	crc_code2 =(*(dd))*0x100+ (*(dd+1));
	crc_code3 =(*(dd+1))*0x100+ (*(dd+2));
	crc_ret =0;
	for(i =0;i< real_len;)
	{
		memset(dd_buf,0,sizeof(dd_buf));
		sFLASH_ReadBuffer((u8 *)&dd_buf[0],FLASH_UPDATE_DATA_START+i,4096);
		if(real_len >=(i+4096) )
		{
			crc_ret = CalCrc_xmode((u8 *)&dd_buf[0],4096,crc_ret,&crc_a1,&crc_a2,&crc_a3);
			i+=4096;
			if((real_len-i) ==1)
			{
				if(crc_code1==crc_a2)
				{//代码有效
					return 1;
				}
				else
					return 0;
			}
			else if((real_len-i) ==2)
			{
				if(crc_code1==crc_a3)
				{//代码有效
					return 1;
				}
				else
					return 0;				
			}			
		}
		else
		{
			CalCrc_xmode((u8 *)&dd_buf[0],real_len-i,crc_ret,&crc_a1,&crc_a2,&crc_a3);
			break;
		}	
	}
	if((crc_code1==crc_a1)||(crc_code2==crc_a2)||(crc_code3==crc_a3))
	{//代码有效
		return 1;
	}
	return 0;
}			
#ifdef TEST_UPDATE_FLAG
void test_update_flag()
{
	u8 update_flag_buff[FLASH_UPDATE_LEN];
	u8 update1_flag_buff[FLASH_UPDATE_LEN];
  spi_update_flag* ptr_update_flag;
	memset(update_flag_buff,0,sizeof(update_flag_buff));
	sFLASH_ReadBuffer(update_flag_buff, FLASH_UPDATE_START, sizeof(update_flag_buff));
	ptr_update_flag = (spi_update_flag*)update_flag_buff;
	if(ptr_update_flag->start_flag==0xFFFF)
	{
		sFLASH_EraseSector(FLASH_UPDATE_START);
		DelyMs(20);
		memcpy(update1_flag_buff,update_flag_buff,FLASH_UPDATE_LEN);
		ptr_update_flag = (spi_update_flag*)update1_flag_buff;
		ptr_update_flag->start_flag=UPDATA_START_FLAG_OK;
		ptr_update_flag->device_type =0;
		ptr_update_flag->update_type = 1;
		ptr_update_flag->file_size = TEST_FILE_FUSE_SIZE;
		sFLASH_WriteBuffer(update1_flag_buff,FLASH_UPDATE_START,sizeof(update1_flag_buff));//write this test data
		memset(update_flag_buff,0,sizeof(update_flag_buff));
		sFLASH_ReadBuffer(update_flag_buff, FLASH_UPDATE_START, sizeof(update_flag_buff));
		ptr_update_flag = ptr_update_flag;
	}
}
#endif

void handle_update(spi_update_flag flag)
{
	u8 ret=0;
	if(flag.file_size >4*1024){
		ret=judge_code_sure(flag.file_size);
		if(ret>0){
			ret = updataProgram(flag.file_size);
			if(ret>0){
			}
		}
	}
	
}

int check_flash(){
	u8 update_flag_buff[FLASH_UPDATE_LEN];
	spi_update_flag update_flag;
  	spi_update_flag* ptr_update_flag;
	uint32_t flag = 0xFFFFFFFF;
	uint8_t count=0;
	//check flash
	FlashID = sFLASH_ReadID();
	if(FlashID == sFLASH_W25Q32_ID ||FlashID == sFLASH_W25Q32_N_ID ||
		FlashID == sFLASH_W25Q32_A_ID|| FlashID == sFLASH_W25Q128_ID)
	{
		memset(check_buff,0,sizeof(check_buff));
		//read back flag
		do{
		sFLASH_ReadBuffer(check_buff, FLASH_INIT_FLAG_START, sizeof(check_buff));
		flag = (check_buff[FLASH_INIT_FLAG_OFFSET]<<24)+(check_buff[FLASH_INIT_FLAG_OFFSET+1]<<16)+
			(check_buff[FLASH_INIT_FLAG_OFFSET+2]<<8)+(check_buff[FLASH_INIT_FLAG_OFFSET+3]);
			count++;
			DelyMs(20);
		}while(flag== 0xFFFFFFFF && count < 5);
			
		if(flag == 0xFFFFFFFF || flag != FLASH_INIT_FLAG){
			send_debug_data("EARSE INIT FLASH !!!\r\n");
			#if 1
			sFLASH_EraseSector(FLASH_INIT_FLAG_START);
			DelyMs(20);
			//write init_flag
			check_buff[FLASH_INIT_FLAG_OFFSET] = (FLASH_INIT_FLAG>>24)&0xFF;
			check_buff[FLASH_INIT_FLAG_OFFSET+1] = (FLASH_INIT_FLAG>>16)&0xFF;
			check_buff[FLASH_INIT_FLAG_OFFSET+2] = (FLASH_INIT_FLAG>>8)&0xFF;
			check_buff[FLASH_INIT_FLAG_OFFSET+3] = (FLASH_INIT_FLAG)&0xFF;
			sFLASH_WriteBuffer(check_buff, FLASH_INIT_FLAG_START, sizeof(check_buff));
			memset(check_buff,0,sizeof(check_buff));
			sFLASH_ReadBuffer(check_buff, FLASH_INIT_FLAG_START, sizeof(check_buff));
			flag = (check_buff[FLASH_INIT_FLAG_OFFSET]<<24)+(check_buff[FLASH_INIT_FLAG_OFFSET+1]<<16)+
			(check_buff[FLASH_INIT_FLAG_OFFSET+2]<<8)+(check_buff[FLASH_INIT_FLAG_OFFSET+3]);
			//sFLASH_ReadBuffer(check_buff, FLASH_PARAM_START+FLASH_PARAM_HEAD_LEN, sizeof(check_buff));
			flag = flag;
			#endif
		}
		memset(update_flag_buff,0,sizeof(update_flag_buff));
		#ifdef TEST_BACKUP_FLASH
		backup_flash();
		read_back_page();
		#endif
		sFLASH_ReadBuffer(update_flag_buff, FLASH_UPDATE_START, sizeof(update_flag_buff));
		ptr_update_flag = (spi_update_flag*)update_flag_buff;
		//check update flag start
		if(ptr_update_flag->start_flag==UPDATA_START_FLAG_OK && ptr_update_flag->end_flag!=UPDATA_END_FLAG_OK)
		{
			update_flag = *ptr_update_flag;//copy
			memset(update_flag_buff,0,sizeof(update_flag_buff));
			sFLASH_ReadBuffer(update_flag_buff, FLASH_UPDATE_WBACKUP_START, sizeof(update_flag_buff));
			ptr_update_flag = (spi_update_flag*)update_flag_buff;
			//check update handled
			if(ptr_update_flag->end_flag!=UPDATA_END_FLAG_OK){
				handle_update(update_flag);
			}
			
		}
		
		//if all ok or flush finish return 1	
		return 1;
	}else{
		FlashID = FlashID;
	}
	return -1;
}

typedef void (*pFunction)(void);
pFunction Jump_To_Application;
uint32_t JumpAddress;

void Jump_To_App(uint32_t address)
{
    if (((*(__IO uint32_t*)address) & 0x2FFE0000) == 0x20000000)
    {
		sFLASH_DeInit();
        JumpAddress = *(__IO uint32_t*) (address + 4);
		Jump_To_Application = (pFunction) JumpAddress;
        __set_MSP(*(__IO uint32_t*) address);
        Jump_To_Application();
    }
}

/**
 * @brief  Main program.
 */
int main(void)
{
		int ret=0;
		uint8_t state=0;
		Interrupt_usart_init();
		/* Initialize the SPI FLASH driver */
    sFLASH_Init();
    /*SystemInit() function has been called by startup file startup_n32g43x.s*/
		DelyMs(500);
		//wait for startup iap and boot app here
		if(judge_program()==1 )//判断是否升级程序
		{
			//extend_watchdog io setup for safe update only(nouse now)
			initEWD_IO();
			state=dowith_xmode_updata(getCOMM(),1);
		}
		//check update_flag and copy code to app 		
		ret = check_flash();
		stand_Deinit();
		if(ret>0){

		//try jump to app
		Jump_To_App(APP_START_ADDRESS);
		}else if(ret <0)
		{
			Jump_To_App(APP_START_ADDRESS);
		}

    while (1)
    {
			//if jump failed wait isp only
			//now do nothing wait for extend_watchdog reset
			DelyMs(1000);
			
    }
}

int entry(void)
{
    return main();
}

/**
 * @}
 */
