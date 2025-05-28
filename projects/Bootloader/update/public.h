#ifndef  __PUBLIC_H__
#define  __PUBLIC_H__

#include <stdio.h>
#include <string.h>
#include "n32g43x.h"

#ifndef __packed
#define __packed    __attribute__((packed))
#endif

#ifdef   MAIN_GLOBALS
#define  PUBLIC_EXT
#else
#define  PUBLIC_EXT  extern
#endif
typedef  void (*pFunction)(void);
/************全局变量****************/
/*与GPS相关的变量*/

#define IAP1_COM                        UART1
#define IAP2_COM                        UART2
#define IAP3_COM                        UART3
#define IAP4_COM                        UART4
#define IAP5_COM                        UART5

#define COM_NO_EXIST	0
#define COM1 	1
#define COM2 	2
#define COM3 	3
#define COM4 	4
#define COM5 	5
#define COM6 	6

#define  IAP1comm      	COM1
#define  IAP2comm      	COM2
#define  IAP3comm      	COM3
#define  IAP4comm      	COM4
#define  IAP5comm      	COM5


#define	PRIO_IAP1COMM		2  //UART4 连接GPS，并且用于IAP升级
#define	PRIO_IAP2COMM		2  //UART4 连接GPS，并且用于IAP升级
#define	PRIO_IAP3COMM		2  //UART4 连接GPS，并且用于IAP升级
#define	PRIO_IAP4COMM		2  //UART4 连接GPS，并且用于IAP升级
#define	PRIO_IAP5COMM		2  //UART4 连接GPS，并且用于IAP升级

#define IAPMAXAddress		  0X0805FFFF   //IAP程序能操作的最大地址范围

#define uart_iap_ram_buf_length	 132

/***********************/
#define 	xmode_SOH   	0x01 //包头
#define 	xmode_EOT   	0x04 //文件结束标记
#define 	xmode_ACK  		0x06 //正常相应 如数据包正确接收
#define 	xmode_NAK  		0x15 //非正常应答
#define     xmode_CAN     	0x18//取消命令
#define     xmode_BEGIN     0x43 //'C'
/**/

typedef struct 
{
	unsigned char	CommNum;   //串口索引，取值为:COM1--COM5 
	unsigned char  	Priority;  //该串口中断的优先级
	unsigned int 	Bandrate;  //该串口的波特率
	unsigned char 	DataBit;   //该串口的数据格式，取值为:5,6,7,8,
	unsigned char 	Parity;	   //该串口的数据校验方式，，取值为:?N'(无校验),'O'(奇校验),'E'(偶校验)
	unsigned char 	StopBit;   //该串口的停止位格式，取值为:1,2		
} StruCOMMCONFIG;

//IAP 升级信息结构体
typedef  struct
{
  unsigned char Flag;	// 升级状态 0： 升级完成   0xaa： 升级开始    0x11： 数据下载开始（升级固件开始）0x55数据下载完成
  unsigned char Device;// 升级的设备 0：表示升级主机 1：表示升级调度屏 2：升级POS机 3：升级计价器 4：升级LED显示屏 5：升级空车牌 6:升级计价器
  unsigned char Success; // 最近一次升级状态 0 失败 0xAA将程序拷贝到APP成功
  unsigned char Source;	 // 0--从服务器升级，1－从IAP串口升级，2－从SD卡升级 3 从IC卡升级 4Xmode升级 5 计价器升级软件升级
  unsigned int Page;  // 程序在flash中存储的页数
  unsigned int Size;	// 升级数据长度
  unsigned short crc;     //升级程序CRC校验值       
}__packed UPFLAG_STRUCT;

/*关于串口的定义*/
typedef struct 
{
	u8 EofTimes;
	u8 time;
	u8 state;  //串口接收状态 0-> start ready,0x08 ->eot pack fin 0x12 ->pack err
	u8 point;
	u8 jiaoyan;
	u8 jiaoyanma;
	u8 minglingma; //1:fin eot 0:not fin, only for 08 pack fin detect 
	u8 changduma;
	u8 pocket_length;
	u8 sbuf;
	u8 ram_buf[200];
}UART_IAP;
unsigned char getCOMM();
void send_debug_data(char* string);
/************全局函数****************/
PUBLIC_EXT  u16  CalCrc(u8 *ptr , int count);
PUBLIC_EXT void READ_UART_BYTE(u8 rev,u8 com_num);

PUBLIC_EXT u8 Uart_init(StruCOMMCONFIG *pHandle);
PUBLIC_EXT void Interrupt_usart_init(void);
PUBLIC_EXT void Interrupt_usart_deinit(void);
PUBLIC_EXT void DelyMs(u32 a);

PUBLIC_EXT u8 judge_program(void);
PUBLIC_EXT void UartBaudRateSet(USART_Module* USARTx,u32 baudrate);
PUBLIC_EXT u8 receive_xmode(u8 SEND_DATA,u16 Pocket,u8 comm_num,u16 DELAY_TIME,u8 *comm_number);
PUBLIC_EXT u8 dowith_xmode_updata(u8 comm_num,u8 have_read);
PUBLIC_EXT void sendcom_byte(u8 c,u8 com_num);
void stand_Deinit(void);
#endif                                                          /* End of module include.                               */
