
#define MAIN_GLOBALS
#include "public.h"
#include "n32g43x_usart.h"
#include "flash_data_def.h"
#include "spi_flash.h"

PUBLIC_EXT unsigned char  UPDATE_USE_COMM=0;//升级使用的串口

PUBLIC_EXT UART_IAP uart_iap[6];//5个串口
//IAP相关的变量
PUBLIC_EXT u8 IAP_Buf[2048];
PUBLIC_EXT u32 IAP_Count;

USART_Module * const UART_BASE_ADDR[]={0,USART1,USART2,USART3,UART4,UART5};
const u8 UART_INT_ID[]={0,USART1_IRQn,USART2_IRQn,USART3_IRQn,UART4_IRQn,UART5_IRQn};

extern u8 AppSpiFlashRead(u8 *pbuf,u32 ReadAddr,u16 len);
extern u8 AppSpiFlashWrite(u8 *pbuf,u32 WriteAddr,u32 len,u8 erasetag);
#if 0
const StruCOMMCONFIG IAP1ComParamter=	
{
	IAP1comm,
	PRIO_IAP1COMM,
	57600,
	8,
	'N',
	1,
};


const StruCOMMCONFIG IAP2ComParamter=	
{
	IAP2comm,
	PRIO_IAP2COMM,
	57600,
	8,
	'N',
	1,
};

const StruCOMMCONFIG IAP3ComParamter=	
{
	IAP3comm,
	PRIO_IAP3COMM,
	57600,
	8,
	'N',
	1,
};

const StruCOMMCONFIG IAP4ComParamter=	
{
	IAP4comm,
	PRIO_IAP4COMM,
	57600,
	8,
	'N',
	1,
};
#endif

const StruCOMMCONFIG IAP5ComParamter=	
{
	IAP5comm,
	PRIO_IAP5COMM,
	115200,
	8,
	'N',
	1,
};

unsigned char getCOMM(){
	return UPDATE_USE_COMM;
}
/***************************************************************/
//函 数 名：calcrc 
//功    能：crc 校验计算
//输入参数：*ptr 指向需要校验的数据
//           count  校验的数据字节长度 
//输出参数：无
//返    回：校验结果
/***************************************************************/
u16 CalCrc(u8 *ptr , int count)
{
	u16  crc = 0 ;
	char ii;

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
	}
	return(crc);
}

static void delay_ms(u32 DelayTime)
{	
	u32 i;
	while(DelayTime>0)
	{	
		DelayTime--;
		for(i=0;i<9000;i++)//for(i=0;i<335;i++)//
		{
			__NOP();
		}
		wdg();
	}

}
/**
  * @brief  Print a character on the HyperTerminal
  * @param  c: The character to be printed
  * @retval None
  */
void sendcom_byte(u8 c,u8 com_num)
{
	u32 i;
	i=50000;
	#if 0
	if((com_num ==0)||(com_num ==1))
	{
		USART_SendData(USART1, c);	
	}
	if((com_num ==0)||(com_num ==2))
	{
		USART_SendData(USART2, c);	
	}
	if((com_num ==0)||(com_num ==3))
	{
		USART_SendData(USART3, c);	
	}
	if((com_num ==0)||(com_num ==4))
	{
		USART_SendData(UART4, c);	
	}
	#endif
	if((com_num ==0)||(com_num ==5))
	{
		USART_SendData(UART5, c);	
	}
	#if 0
	if((com_num ==0)||(com_num ==1))
	{	i=50000;
		while (USART_GetFlagStatus(USART1, USART_FLAG_TXDE) == RESET)
		{
			i--;
			if(i == 0)
			break;
		}
	}
	if((com_num ==0)||(com_num ==2))
	{	i=50000;
		while (USART_GetFlagStatus(USART2, USART_FLAG_TXDE) == RESET)
		{
			i--;
			if(i == 0)
			break;
		}
	}
	if((com_num ==0)||(com_num ==3))
	{	i=50000;
		while (USART_GetFlagStatus(USART3, USART_FLAG_TXDE) == RESET)
		{
			i--;
			if(i == 0)
			break;
		}
	}
	if((com_num ==0)||(com_num ==4))
	{	i=50000;
		while (USART_GetFlagStatus(UART4, USART_FLAG_TXDE) == RESET)
		{
			i--;
			if(i == 0)
			break;
		}
	}
	#endif
	if((com_num ==0)||(com_num ==5))
	{	i=50000;
		while (USART_GetFlagStatus(UART5, USART_FLAG_TXDE) == RESET)
		{
			i--;
			if(i == 0)
			break;
		}
	}
}
#define DEBUG_UART UART5
void send_debug_data(char* string)
{
	int i,wait_count,datalen=0;
	if(strlen(string)<=0){ return;}
	datalen = strlen(string);
	if(datalen >128){
		datalen=128;
	}
	for(i=0;i<datalen;i++){
		USART_SendData(DEBUG_UART, string[i]);
		wait_count=50000;
		while (USART_GetFlagStatus(DEBUG_UART, USART_FLAG_TXDE) == RESET)
		{
			wait_count--;
			if(wait_count == 0)
			break;
		}
	}
}


/**
  * @brief  Print a string on the HyperTerminal
  * @param  s: The string to be printed
  * @retval None
  */


/*********
串口初始化函数
**********/
u8 Uart_init(StruCOMMCONFIG *pHandle)
{	
	GPIO_InitType GPIO_InitStructure;
	USART_InitType USART_InitStructure;
	NVIC_InitType NVIC_InitStructure;
	switch(pHandle ->CommNum)
	{
		#if 0
		case COM1:
			RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_USART1 |RCC_APB2_PERIPH_GPIOA |RCC_APB2_PERIPH_AFIO, ENABLE);
			GPIO_InitStruct(&GPIO_InitStructure);
			/* connect port to USARTx_Rx PA10 */
			GPIO_InitStructure.Pin            = GPIO_PIN_10;
			GPIO_InitStructure.GPIO_Pull 			= GPIO_Pull_Up;
			GPIO_InitStructure.GPIO_Mode      = GPIO_Mode_AF_PP;
				GPIO_InitStructure.GPIO_Alternate = GPIO_AF4_USART1;
			GPIO_InitPeripheral(GPIOA, &GPIO_InitStructure);
			/* connect port to USARTx_Tx PA9 */
			GPIO_InitStructure.Pin            = GPIO_PIN_9;
			GPIO_InitStructure.GPIO_Mode      = GPIO_Mode_AF_PP;
				GPIO_InitStructure.GPIO_Alternate = GPIO_AF4_USART1;
			GPIO_InitPeripheral(GPIOA, &GPIO_InitStructure);

			break;
		case COM2:
			RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_GPIOA |RCC_APB2_PERIPH_AFIO, ENABLE);
			RCC_EnableAPB1PeriphClk(RCC_APB1_PERIPH_USART2, ENABLE);
			GPIO_InitStruct(&GPIO_InitStructure);
			/* connect port to USARTx_Rx PA3 */
			GPIO_InitStructure.Pin            = GPIO_PIN_3;
			GPIO_InitStructure.GPIO_Pull 			= GPIO_Pull_Up;
			GPIO_InitStructure.GPIO_Mode      = GPIO_Mode_AF_PP;
				GPIO_InitStructure.GPIO_Alternate = GPIO_AF4_USART2;
			GPIO_InitPeripheral(GPIOA, &GPIO_InitStructure);
			/* connect port to USARTx_Tx PA2 */
			GPIO_InitStructure.Pin            = GPIO_PIN_2;
			GPIO_InitStructure.GPIO_Mode      = GPIO_Mode_AF_PP;
				GPIO_InitStructure.GPIO_Alternate = GPIO_AF4_USART2;
			GPIO_InitPeripheral(GPIOA, &GPIO_InitStructure);
			break;
		case COM3:
			RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_GPIOB |RCC_APB2_PERIPH_AFIO, ENABLE);
			RCC_EnableAPB1PeriphClk(RCC_APB1_PERIPH_USART3, ENABLE);
			GPIO_InitStruct(&GPIO_InitStructure);
			/* connect port to USARTx_Rx PB11 */
			GPIO_InitStructure.Pin            = GPIO_PIN_11;
			GPIO_InitStructure.GPIO_Pull 			= GPIO_Pull_Up;
			GPIO_InitStructure.GPIO_Mode      = GPIO_Mode_AF_PP;
				GPIO_InitStructure.GPIO_Alternate = GPIO_AF0_USART3;
			GPIO_InitPeripheral(GPIOB, &GPIO_InitStructure);
			/* connect port to USARTx_Tx PB10 */
			GPIO_InitStructure.Pin            = GPIO_PIN_10;
			GPIO_InitStructure.GPIO_Mode      = GPIO_Mode_AF_PP;
				GPIO_InitStructure.GPIO_Alternate = GPIO_AF0_USART3;
			GPIO_InitPeripheral(GPIOB, &GPIO_InitStructure);
			break;		
		case COM4:
			RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_UART4|RCC_APB2_PERIPH_GPIOC |RCC_APB2_PERIPH_AFIO, ENABLE);
			
			GPIO_InitStruct(&GPIO_InitStructure);
			/* connect port to USARTx_Rx */
			GPIO_InitStructure.Pin            = GPIO_PIN_11;
			GPIO_InitStructure.GPIO_Pull 			= GPIO_Pull_Up;
			GPIO_InitStructure.GPIO_Mode      = GPIO_Mode_AF_PP;
				GPIO_InitStructure.GPIO_Alternate = GPIO_AF6_UART4;
			GPIO_InitPeripheral(GPIOC, &GPIO_InitStructure);
			/* connect port to USARTx_Tx */
			GPIO_InitStructure.Pin            = GPIO_PIN_10;
			GPIO_InitStructure.GPIO_Mode      = GPIO_Mode_AF_PP;
				GPIO_InitStructure.GPIO_Alternate = GPIO_AF6_UART4;
			GPIO_InitPeripheral(GPIOC, &GPIO_InitStructure);
			break;	
		#endif			
		case COM5:
			RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_UART5|RCC_APB2_PERIPH_GPIOC | RCC_APB2_PERIPH_GPIOD | RCC_APB2_PERIPH_AFIO, ENABLE);
			GPIO_InitStruct(&GPIO_InitStructure);
			/* connect port to USARTx_Rx */
			GPIO_InitStructure.Pin            = GPIO_PIN_2;
			GPIO_InitStructure.GPIO_Pull 			= GPIO_Pull_Up;
			GPIO_InitStructure.GPIO_Mode      = GPIO_Mode_AF_PP;
			GPIO_InitStructure.GPIO_Alternate = GPIO_AF6_UART5;
			GPIO_InitPeripheral(GPIOD, &GPIO_InitStructure);

			GPIO_InitStruct(&GPIO_InitStructure);
			/* connect port to USARTx_Tx */
			GPIO_InitStructure.Pin            = GPIO_PIN_12;
			GPIO_InitStructure.GPIO_Pull 			= GPIO_Pull_Up;
			GPIO_InitStructure.GPIO_Mode      = GPIO_Mode_AF_PP;
			GPIO_InitStructure.GPIO_Alternate = GPIO_AF6_UART5;
			GPIO_InitPeripheral(GPIOC, &GPIO_InitStructure);
			break;				
		default:
			return 0;
	}
  	/* Configure the NVIC Preemption Priority Bits */  		    
    if(pHandle ->CommNum <COM6 && pHandle ->CommNum>COM_NO_EXIST)        
	{
		/* Enable the USARTx Interrupt */
		/* disable rx irq */
		NVIC_InitStructure.NVIC_IRQChannel            = UART_INT_ID[pHandle ->CommNum];
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
		NVIC_InitStructure.NVIC_IRQChannelSubPriority = PRIO_IAP1COMM;
		NVIC_InitStructure.NVIC_IRQChannelCmd         = DISABLE;
		NVIC_Init(&NVIC_InitStructure);
		/* disable interrupt */
		USART_ConfigInt(UART_BASE_ADDR[pHandle ->CommNum], USART_INT_RXDNE, DISABLE);
	
		USART_InitStructure.BaudRate = pHandle ->Bandrate;			  
		USART_InitStructure.WordLength = (pHandle ->DataBit<=8)?USART_WL_8B:USART_WL_9B;
		USART_InitStructure.StopBits =  (pHandle ->StopBit==1)?USART_STPB_1:USART_STPB_2;
		if(pHandle ->Parity =='O')
			USART_InitStructure.Parity = USART_PE_ODD;
		else if(pHandle ->Parity =='E')
			USART_InitStructure.Parity = USART_PE_EVEN;
		else
			USART_InitStructure.Parity = USART_PE_NO;
		USART_InitStructure.HardwareFlowControl = USART_HFCTRL_NONE;
		USART_InitStructure.Mode = USART_MODE_RX | USART_MODE_TX;
		
		/* Configure USART1 */
		USART_Init(UART_BASE_ADDR[pHandle ->CommNum], &USART_InitStructure);

		/* enable interrupt */
		USART_ConfigInt(UART_BASE_ADDR[pHandle ->CommNum], USART_INT_RXDNE, ENABLE);
		/* enable rx irq */
		NVIC_InitStructure.NVIC_IRQChannel            = UART_INT_ID[pHandle ->CommNum];
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
		NVIC_InitStructure.NVIC_IRQChannelSubPriority = PRIO_IAP1COMM;
		NVIC_InitStructure.NVIC_IRQChannelCmd         = ENABLE;
		NVIC_Init(&NVIC_InitStructure);
		/* Enable the USART */
		USART_Enable(UART_BASE_ADDR[pHandle ->CommNum], ENABLE); //使能串口		
    }

	return 1;
}
#define  UART_INT_DISABLE_ALL (USART_IT_CTS|USART_IT_LBD|USART_IT_TXE|USART_IT_TC|USART_IT_RXNE|USART_IT_IDLE|USART_IT_PE)
/************
跳转到应用程序执行
************/
void stand_Deinit(void)
{
	wdg(); 
	SysTick->CTRL = 0;
	Interrupt_usart_deinit();
	#if 0
	USART_ConfigInt(USART1, USART_INT_RXDNE|USART_INT_TXDE, DISABLE);
	USART_Enable(USART1,DISABLE); //使能串口
	
	USART_ConfigInt(USART2, USART_INT_RXDNE|USART_INT_TXDE, DISABLE);
	USART_Enable(USART2,DISABLE); //使能串口
	
	USART_ConfigInt(USART3, USART_INT_RXDNE|USART_INT_TXDE, DISABLE);
	USART_Enable(USART3,DISABLE); //使能串口
	
	USART_ConfigInt(UART4, USART_INT_RXDNE|USART_INT_TXDE, DISABLE);
	USART_Enable(UART4,DISABLE); //使能串口
	#endif
	USART_ConfigInt(UART5, USART_INT_RXDNE|USART_INT_TXDE, DISABLE);
	USART_Enable(UART5,DISABLE); //使能串口

}


/**********
延时函数
*********/
void DelyMs(u32 a)
{
  delay_ms(5*a);
}
void OutputInit(GPIO_Module* GPIOx, uint16_t Pin)
{
    GPIO_InitType GPIO_InitStructure;

    /* Check the parameters */
    assert_param(IS_GPIO_ALL_PERIPH(GPIOx));

    /* Enable the GPIO Clock */
    if (GPIOx == GPIOA)
    {
        RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_GPIOA, ENABLE);
    }
    else if (GPIOx == GPIOB)
    {
        RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_GPIOB, ENABLE);
    }
    else if (GPIOx == GPIOC)
    {
        RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_GPIOC, ENABLE);
    }
    else
    {
        if (GPIOx == GPIOD)
        {
            RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_GPIOD, ENABLE);
        }
    }

    /* Configure the GPIO pin */
    if (Pin <= GPIO_PIN_ALL)
    {
        GPIO_InitStruct(&GPIO_InitStructure);
        GPIO_InitStructure.Pin        = Pin;
        GPIO_InitStructure.GPIO_Current = GPIO_DC_4mA;
        GPIO_InitStructure.GPIO_Pull    = GPIO_No_Pull;
        GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
        GPIO_InitPeripheral(GPIOx, &GPIO_InitStructure);
    }
}
void setOutput(GPIO_Module* GPIOx, uint16_t Pin,int value){
	if (value != Bit_RESET)
	{
			GPIOx->PBSC = Pin;
	}
	else
	{
			GPIOx->PBC = Pin;
	}
}
void bsp_pwr_init(){
	//gsm pwr off
	OutputInit(GPIOA,GPIO_PIN_15);
	GPIO_WriteBit(GPIOA,GPIO_PIN_15,0);
	//gnss pwr off
	OutputInit(GPIOB,GPIO_PIN_5);
	GPIO_WriteBit(GPIOB,GPIO_PIN_5,0);
	
}
#if 0
void uart_disable()
{
		Interrupt_usart_deinit();
	
	USART_ConfigInt(USART1, USART_INT_RXDNE|USART_INT_TXDE, DISABLE);
	USART_Enable(USART1,DISABLE); //使能串口
	
	USART_ConfigInt(USART2, USART_INT_RXDNE|USART_INT_TXDE, DISABLE);
	USART_Enable(USART2,DISABLE); //使能串口
	
	USART_ConfigInt(USART3, USART_INT_RXDNE|USART_INT_TXDE, DISABLE);
	USART_Enable(USART3,DISABLE); //使能串口
	
	USART_ConfigInt(UART4, USART_INT_RXDNE|USART_INT_TXDE, DISABLE);
	USART_Enable(UART4,DISABLE); //使能串口
}
#endif
/*************
串口初始化函数
*************/
void Interrupt_usart_init(void)
{
	bsp_pwr_init();
	memset(uart_iap,0,sizeof(uart_iap));
	//uart_disable();
	#if 0
	Uart_init((StruCOMMCONFIG *) &IAP1ComParamter);
	Uart_init((StruCOMMCONFIG *) &IAP2ComParamter);
	Uart_init((StruCOMMCONFIG *) &IAP3ComParamter);
	Uart_init((StruCOMMCONFIG *) &IAP4ComParamter);
	#endif
	Uart_init((StruCOMMCONFIG *) &IAP5ComParamter);
	
}
void Interrupt_usart_deinit(void)
{
	NVIC_InitType NVIC_InitStructure;
	#if 0
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	//USART2_IRQn,USART3_IRQn,UART4_IRQn,UART5_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = PRIO_IAP1COMM;
	NVIC_InitStructure.NVIC_IRQChannelCmd = DISABLE;
	NVIC_Init(&NVIC_InitStructure);
	
	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	//USART2_IRQn,USART3_IRQn,UART4_IRQn,UART5_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = PRIO_IAP2COMM;
	NVIC_InitStructure.NVIC_IRQChannelCmd = DISABLE;
	NVIC_Init(&NVIC_InitStructure);
	
	NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
	//USART2_IRQn,USART3_IRQn,UART4_IRQn,UART5_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = PRIO_IAP3COMM;
	NVIC_InitStructure.NVIC_IRQChannelCmd = DISABLE;
	NVIC_Init(&NVIC_InitStructure);
	
	NVIC_InitStructure.NVIC_IRQChannel = UART4_IRQn;
	//USART2_IRQn,USART3_IRQn,UART4_IRQn,UART5_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = PRIO_IAP4COMM;
	NVIC_InitStructure.NVIC_IRQChannelCmd = DISABLE;
	NVIC_Init(&NVIC_InitStructure);
	#endif
	NVIC_InitStructure.NVIC_IRQChannel = UART5_IRQn;
	//USART2_IRQn,USART3_IRQn,UART4_IRQn,UART5_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority =PRIO_IAP5COMM;
	NVIC_InitStructure.NVIC_IRQChannelCmd = DISABLE;
	NVIC_Init(&NVIC_InitStructure);
	#if 0
	USART_DeInit(USART1);
	USART_DeInit(USART2);
	USART_DeInit(USART3);
	USART_DeInit(UART4);
	#endif
	USART_DeInit(UART5);
}
#if 0
/*********
设置串口波特率
***********/
void UartBaudRateSet(USART_Module* USARTx,u32 baudrate)
{
	USART_InitType USART_InitStructure;
	USART_Enable(USARTx, DISABLE);
	USART_InitStructure.BaudRate = baudrate;
	USART_InitStructure.WordLength = USART_WL_8B;
	USART_InitStructure.StopBits = USART_STPB_1;
	USART_InitStructure.Parity = USART_PE_NO;
	USART_InitStructure.HardwareFlowControl = USART_HFCTRL_NONE;
	USART_InitStructure.Mode = USART_MODE_RX | USART_MODE_TX;
	USART_Init(USARTx, &USART_InitStructure);
	USART_Enable(USARTx, ENABLE);
}
#endif
u8 uart_ram_buf[150];
/***********
从串口接收数据
**********/
void READ_UART_BYTE(u8 rev,u8 comm_num)
{	
	static int recvcount=0;
	u8 kk;
	kk=rev;
	wdg();
	recvcount++;
	uart_ram_buf[recvcount]=kk;
	if(recvcount==132){
		recvcount=0;
	}
	{
		uart_iap[comm_num].time =0;
		if(uart_iap[comm_num].state == 0x00)
		{ //接收第一个字符
			if(xmode_SOH == kk)
			{
				uart_iap[comm_num].state =1;
				uart_iap[comm_num].minglingma= 0;
				uart_iap[comm_num].point =0;
				uart_iap[comm_num].pocket_length =0;
				uart_iap[comm_num].EofTimes =0;	
			}
			else if(xmode_EOT == kk)
			{
			 	
				uart_iap[comm_num].state =8;
				uart_iap[comm_num].EofTimes++;
				uart_iap[comm_num].minglingma= 1;
			}
		}
		else if(uart_iap[comm_num].state == 1)
		{  //接收第二个字符
			uart_iap[comm_num].EofTimes =0;
			if(uart_iap[comm_num].point >=132)
			{
				uart_iap[comm_num].state =0x12;
				uart_iap[comm_num].point =132;
			}
			uart_iap[comm_num].ram_buf[uart_iap[comm_num].point] = kk;
			uart_iap[comm_num].point++;
			if(uart_iap[comm_num].point >=132)
			 {
				uart_iap[comm_num].state =8;
				uart_iap[comm_num].minglingma= 0;
			}
		}
	}		
}


/***********
检测是否需要升级
串口升级的序号
无需升级返回0
***********/
u8 judge_program(void)
{
	u32 i;
	u8 comm_n=IAP5comm;
	for(i=0;i<2;i++) //按键查询0.5秒钟
	{
		DelyMs(1);
		if(receive_xmode(xmode_BEGIN,0,0,200,&comm_n)==0)
		{
			if(comm_n !=0)
			{
				UPDATE_USE_COMM = comm_n;
				return 1;
			}	
		}	
	}
	return 0;
}

/*********************************************************************/
/*
	接收数据 
	最后一包数据为 0x02,成功0，失败1 ,3非本公司程序
    SEND_DATA  预发送数据
    Pocket  包序号
    comm_num   发送串口 0 表示1~5都发送
	DELAY_TIME   延时时间
	comm_n  实际收到数据的串口；
*/
u8 receive_xmode(u8 SEND_DATA,u16 Pocket,u8 comm_num,u16 DELAY_TIME,u8 *comm_n)
{
	u8 lop,send_againtime =0,real_comm=0;
	u16 delay_time;
	u8 a,b;
	int ii;
	int im;
	uart_iap[0].state=uart_iap[1].state=uart_iap[2].state=uart_iap[3].state=uart_iap[4].state= uart_iap[5].state=0;
	if(Pocket ==0x0000)
	{
		sendcom_byte(xmode_BEGIN,comm_num);//发送‘C’	
	}
	else
	{
		sendcom_byte(SEND_DATA,comm_num);	
	}
	IAP_Count = 0;
	//下面等待接收2K的数据
	while(IAP_Count < 2048)
	{
			
		for( lop =0;lop<4;lop++)
		{
			wdg();
			for(delay_time=0;delay_time<DELAY_TIME;delay_time++)
			{	
				DelyMs(1);
				//if(uart_iap.state==0x08)
				real_comm =0;	
				if(comm_num==0)
				{
					#if 0
					if(uart_iap[1].state==0x08)
						*comm_n=real_comm=comm_num =1;
					else if(uart_iap[2].state==0x08)
						*comm_n=real_comm=comm_num =2;
					else if(uart_iap[3].state==0x08)
						*comm_n=real_comm=comm_num =3;
					else if(uart_iap[4].state==0x08)
						*comm_n=real_comm=comm_num =4;
					else if(uart_iap[5].state==0x08)
						*comm_n=real_comm =comm_num=5;	
					#else
					if(uart_iap[5].state==0x08)
						*comm_n=real_comm =comm_num=5;
					#endif			
				}
				else if(uart_iap[comm_num].state==0x08)
					real_comm = comm_num;
				if(real_comm !=0)//真正反回数据的串口
				{	
					if(uart_iap[real_comm].minglingma ==0x01)
					{	
						DelyMs(2);
						sendcom_byte(xmode_ACK,real_comm);
						DelyMs(120);
						return(2);
					}  
					a = uart_iap[real_comm].ram_buf[0];
					b = (~uart_iap[real_comm].ram_buf[1]);
					if( a != b)
					{
						if(send_againtime<3)
						{	
							send_againtime++;
							uart_iap[real_comm].state=0;
							sendcom_byte(xmode_NAK,real_comm);
						}
						break;
					}
					//CRC校验
					ii = ((u16)uart_iap[real_comm].ram_buf[130])*0X100;
					ii += ((u16)uart_iap[real_comm].ram_buf[131]);
					im = CalCrc (&uart_iap[real_comm].ram_buf[2],128);
					if(ii!=	im)
					{
						//校验失败
						if(send_againtime<3)
						{	
							send_againtime++;
							uart_iap[real_comm].state=0;
							sendcom_byte(xmode_NAK,real_comm);
						}
						else
						{
							//sendcom_byte(xmode_CAN);
							return 1;
						}	
					}
					else 
					{
						memcpy(&IAP_Buf[IAP_Count],&uart_iap[real_comm].ram_buf[2],128);
											
						//////
						ii =Pocket+1;
						if(((u8)ii) ==uart_iap[real_comm].ram_buf[0]) //包号正确
						{
							//成功收到一包数据
							//if((Pocket%2==0))
							if(Pocket == 5)//第五包数据
							{
								if(memcmp(&IAP_Buf[512],APP_CODE_STR_FLAG,11)!=0X00)
									return(3);//非本设备的程序							
							} 
							uart_iap[real_comm].state=0;
							IAP_Count += 128;
							Pocket++;
							if(IAP_Count <2048)
								sendcom_byte(xmode_ACK,real_comm);
							else
							{
								return(0);
							}
						}
						else
						{	uart_iap[real_comm].state=0;
							if((((u8)ii) >uart_iap[real_comm].ram_buf[0])||((((u8)ii) ==0)&&( ( uart_iap[real_comm].ram_buf[0]==0xff) ||(uart_iap[real_comm].ram_buf[0] ==0xfe)))) 
								sendcom_byte(xmode_ACK,real_comm);
							
						}
				   }
				   break;
				}
				real_comm =0;	
				if(comm_num==0)
				{
					#if 0
					if(uart_iap[1].state==0x12)
						real_comm=comm_num =1;
					else if(uart_iap[2].state==0x12)
						real_comm=comm_num =2;
					else if(uart_iap[3].state==0x12)
						real_comm=comm_num =3;
					else if(uart_iap[4].state==0x12)
						real_comm=comm_num =4;
					else if(uart_iap[5].state==0x12)
						real_comm =comm_num=5;
					#else
					if(uart_iap[5].state==0x12)
						real_comm =comm_num=5;
					#endif				
				}
				else if(uart_iap[comm_num].state==0x12)
					real_comm = comm_num;
				if(real_comm!=0x00)
				{
					if(send_againtime<3)
					{	
						send_againtime++;
						uart_iap[real_comm].state=0;
						sendcom_byte(xmode_NAK,real_comm);
					}	
				}
			}
			if(( SEND_DATA == xmode_BEGIN) &&(lop>=1) )
				return(1);
			if((send_againtime<3) &&(delay_time>=DELAY_TIME))
			{	
				send_againtime++;
				if(Pocket ==0x0000)
					sendcom_byte(xmode_BEGIN,real_comm);//发送‘C’	
				else
					sendcom_byte(xmode_NAK,real_comm);
			}
			else  if((send_againtime >=3) || (delay_time>=DELAY_TIME))
			 	return(1);
			else
				break;
			
		}
		if(lop < 4)
		{
			;//	
		}
		
	}
	return(1);
}
int UpdateFlag_FlashWrite(u8 *databuf,u32 Address,u32 len)
{
	u8 InfoAriaCach[FLASH_UPDATE_SIZE];
    if (((Address) >= FLASH_UPDATE_START) && ((Address + len) <= (FLASH_UPDATE_END )))
    {
        sFLASH_ReadBuffer(InfoAriaCach,FLASH_UPDATE_START,sizeof(InfoAriaCach));
        memcpy(InfoAriaCach ,databuf,len);
				sFLASH_EraseSector(FLASH_UPDATE_START);
				DelyMs(20);
        sFLASH_WriteBuffer(InfoAriaCach,FLASH_UPDATE_START,sizeof(InfoAriaCach));
    }
	return 1;
}

void earseBackupFlag(){
		sFLASH_EraseSector(FLASH_UPDATE_WBACKUP_START);
		DelyMs(20);	
}

int spiFlashWrite(u8 *databuf,u32 Address,u32 len)
{
	if(Address >= FLASH_UPDATE_DATA_START  && len < FLASH_UPDATE_DATA_APP_SIZE){
		if(Address%APP_RW_BLOCK_SIZE==0){
			sFLASH_EraseSector(Address);
			DelyMs(20);
		}
		sFLASH_WriteBuffer(databuf, Address, len);
	}
	return 1;
}
/**************
Xmode 升级
comm_num  使用的串口序号，

成功返回0 失败返回1
***********/
u8 dowith_xmode_updata(u8 comm_num,u8 have_read)
{
	u8 res,Fres,k;
	u32 Page;
	//u8 RBuf[2048];
	u8 update_flag_buff[FLASH_UPDATE_LEN];
	spi_update_flag* ptr_update_flag;
	Page = 0X0000;
	if(have_read)
		res =0;
	else 
		res	= receive_xmode(xmode_BEGIN,Page,comm_num,1000,0);
	if(res == 0x00)
	{
		//生成升级参数并保存
		memset(update_flag_buff,0,sizeof(update_flag_buff));
		ptr_update_flag = (spi_update_flag*)update_flag_buff;
		ptr_update_flag->start_flag=UPDATA_START_FLAG_OK;
		ptr_update_flag->end_flag =0xFF;//default flag
		ptr_update_flag->sucess = 0xFF;//default flag
		ptr_update_flag->device_type =0;
		ptr_update_flag->update_type = 0;//0->IAP 1:APP_HTTP/APP_FTP
		ptr_update_flag->file_size = 0;
		Fres = UpdateFlag_FlashWrite((u8 *)update_flag_buff,FLASH_UPDATE_START,sizeof(update_flag_buff));	//写升级的进程标识位
		earseBackupFlag();
		//存储到备份程序区第一页
		for(k=0;k<3;k++)
		{
			Fres = spiFlashWrite(&IAP_Buf[0],FLASH_UPDATE_DATA_START,IAP_Count);
			if(Fres == 1)
			{
				
				#if 0
				//AppSpiFlashRead(&RBuf[0],FLASH_UPDATE_DATA_START,IAP_Count);
				if(memcmp(IAP_Buf,RBuf,2048) == 0)
				{
					Fres =1;
					break;
				}
				#else
				break;
				#endif
			}
		}
		//第一包接受正确 则继续
		while(1)
		{
			Page += IAP_Count/128;
			res = receive_xmode(xmode_ACK,Page,comm_num,1200,0);
			if(res == 0)
			{	//成功收到一包数据
				Fres = spiFlashWrite(&IAP_Buf[0],FLASH_UPDATE_DATA_START + Page*128,IAP_Count);
				if(Fres == 0)
				{
					 sendcom_byte(xmode_CAN,comm_num);
					 DelyMs(10);
					 return 1;
				}
				DelyMs(1);
			}
			else if(res == 2)
			{
				//程序下载完成
				Fres =  spiFlashWrite(&IAP_Buf[0],FLASH_UPDATE_DATA_START + Page*128,IAP_Count);
				if(Fres == 0)
				{
					 sendcom_byte(xmode_CAN,comm_num);
					 DelyMs(10);
					 return 1;
				}
				
				ptr_update_flag->file_size = Page*128 + IAP_Count;
				ptr_update_flag->end_flag =UPDATA_END_FLAG_FI;//OK flag
				ptr_update_flag->sucess = 0xFF;//default flag
				Fres = UpdateFlag_FlashWrite((u8 *)update_flag_buff,FLASH_UPDATE_START,sizeof(update_flag_buff));
				if(Fres == 0)
				{
					 sendcom_byte(xmode_CAN,comm_num);
					 DelyMs(10);
					 return 1;
				}
				return 0; 
			}
			else 
			{
				sendcom_byte(xmode_CAN,comm_num);
				DelyMs(10);
				return 1;
			}

		}
	}
	else
		return 1;
}

