/* RT-Thread config file */
#ifndef __RTTHREAD_CFG_H__
#define __RTTHREAD_CFG_H__

//#define DEBUG_APP
//#define DEBUG_SWD

/* RT_NAME_MAX*/
#define RT_NAME_MAX     8

/* RT_ALIGN_SIZE*/
#define RT_ALIGN_SIZE   4

/* PRIORITY_MAX */
#define RT_THREAD_PRIORITY_MAX  32

/* Tick per Second */
#define RT_TICK_PER_SECOND      1000

#ifdef DEBUG_APP
/* SECTION: RT_DEBUG */
/* Thread Debug */
#define RT_DEBUG
#define RT_DEBUG_INIT           1
#define RT_USING_OVERFLOW_CHECK
#endif

/* Using Hook */
/* #define RT_USING_HOOK */

/* Using Software Timer */
/* #define RT_USING_TIMER_SOFT */
#define RT_TIMER_THREAD_PRIO            4
#define RT_TIMER_THREAD_STACK_SIZE      512
#define RT_TIMER_TICK_PER_SECOND        10

/* SECTION: IPC */
/* Using Semaphore*/
#define RT_USING_SEMAPHORE

/* Using Mutex */
#define RT_USING_MUTEX

/* Using Event */
#define RT_USING_EVENT

/* Using MailBox */
/* #define RT_USING_MAILBOX */

/* Using Message Queue */
/* #define RT_USING_MESSAGEQUEUE */

/* SECTION: Memory Management */
/* Using Memory Pool Management*/
/* #define RT_USING_MEMPOOL */

/* Using Dynamic Heap Management */
#define RT_USING_HEAP

/* Using Small MM */
#define RT_USING_SMALL_MEM
#define RT_USING_TINY_SIZE

/* SECTION: Device System */
/* Using Device System */
#define RT_USING_DEVICE
// <bool name="RT_USING_DEVICE_IPC" description="Using device communication" default="true" />
#define RT_USING_DEVICE_IPC
// <bool name="RT_USING_SERIAL" description="Using Serial" default="true" />
#define RT_USING_SERIAL

#define USE_METER_FUNC  (1)

#ifdef DEBUG_APP
/* SECTION: Console options */
#define RT_USING_CONSOLE
#define GPS_DBG (0)
#else
#undef RT_USING_CONSOLE
#endif

#define USE_MODBUS_THREAD_TASK (0)

#ifdef RT_USING_CONSOLE
#define ENABLE_SHELL (1)
/* the buffer size of console*/
#define RT_CONSOLEBUF_SIZE          128
// <string name="RT_CONSOLE_DEVICE_NAME" description="The device name for console" default="uart1" />
#ifdef DEBUG_UART1
#define RT_CONSOLE_DEVICE_NAME      "usart1"
#else
#define RT_CONSOLE_DEVICE_NAME      "uart5"
#endif
#endif
#ifdef DEBUG_SWD
#define DISABLE_SWD_DEBUG	(0)
#else
#define DISABLE_SWD_DEBUG	(1)
#endif

//232-TTL UART-OUT1
#define         RT_USING_USART1
//TTL UART-GSM(inter)
#define         RT_USING_USART2
//485-232 UART-OUT_AUX
#define         RT_USING_USART3
//TTL UART-GPS(inter)
#define         RT_USING_UART4
//232-TTL UART-OUT2
#define         RT_USING_UART5
//ADC PC0,PC1
#define         RT_USING_ADC
//SPI-FLASH
#define         RT_USING_SPI
//SPI2 (need fix CS to PB12)
#define         RT_USING_SPI2
#if USE_SFUD_FUNC
//#define USE_SPI_SYS
#define					RT_USING_SFUD
#define					SFUD_USING_SFDP

#define RT_DEBUG_SFUD
#define RT_SFUD_SPI_MAX_HZ 50000000
#else
//#define USE_SPI_SYS
#define RT_FLASH_SPI_MAX_HZ 50000000
#endif

//#define         RT_USING_I2C
//#define         RT_USING_I2C_BITOPS

//#define         RT_USING_HW_RTC_ONLY
#define         RT_USING_SW_RTC_ONLY

#define					RT_APP_VERSION_M	1
#define					RT_APP_VERSION_S	0
#define					RT_APP_VERSION_R	5

#define STR_M(x) 	#x
#define STR_CON_VER(x,y,z) STR_M(x.y.z)
#define                 SW_VERSION_APP  STR_CON_VER(RT_APP_VERSION_M,RT_APP_VERSION_S,RT_APP_VERSION_R)
//"1.0.3"
#define                 HW_VERSION_APP  "A200D"

//modbus control
#define 				RTU_USING_UART1
//AT GSM port
#define 				RTU_USING_UART2
//usart3 485
#define 				RTU_USING_UART3
#define					RTU_UART3_USING_RS485
//GPS reusing modbus_rt port 
#define 				RTU_USING_UART4
#define 				RTU_USING_UART5




#if USE_METER_FUNC
#ifdef METER_485
//485 port uart3-> meter
#define                 USE_METER_UART "usart3"
#else
#ifdef DEBUG_UART1
//UART1 ->debug uart5->meter
#define                 USE_METER_UART "uart5"
#else
//UART5 ->debug uart1->meter
#define                 USE_METER_UART "usart1"
#endif
#endif
#else
#ifdef RT_USING_CONSOLE
#ifdef DEBUG_UART1
#define                 USE_RTU_MASTER1 (0)
#define 				USE_RTU_MASTER2	(1)
#else
#define                 USE_RTU_MASTER1 (1)
#define 				USE_RTU_MASTER2	(0)
#endif
#else
#define                 USE_RTU_MASTER1 (1)
#define 				USE_RTU_MASTER2	(1)
#endif

#define 				USE_RTU_MASTER3	(1)
#endif



/* RT-Thread Components */

#define RT_USING_COMPONENTS_INIT
#define RT_USING_USER_MAIN

#if ENABLE_SHELL
/* SECTION: finsh, a C-Express shell */
#define RT_USING_FINSH
/* configure finsh parameters */
#define FINSH_THREAD_PRIORITY       25
#define FINSH_THREAD_STACK_SIZE     1024
#define FINSH_HISTORY_LINES         2
/* Using symbol table */
#define FINSH_USING_SYMTAB
#define FINSH_USING_DESCRIPTION
#define FINSH_USING_HISTORY
#define FINSH_USING_MSH
#define FINSH_USING_MSH_ONLY
#endif  //#if 1

#define RCC_GPIOA_CLK_ENABLE
#define RCC_GPIOB_CLK_ENABLE
#define RCC_GPIOC_CLK_ENABLE

#define RT_USING_PIN

/* SECTION: libc management */
#define RT_USING_LIBC

/* SECTION: device filesystem */
/* #define RT_USING_DFS */
//#define RT_USING_DFS_ELMFAT
#define RT_DFS_ELM_WORD_ACCESS
/* Reentrancy (thread safe) of the FatFs module.  */
#define RT_DFS_ELM_REENTRANT
/* Number of volumes (logical drives) to be used. */
#define RT_DFS_ELM_DRIVES           2
/* #define RT_DFS_ELM_USE_LFN       1 */
#define RT_DFS_ELM_MAX_LFN          255
/* Maximum sector size to be handled. */
#define RT_DFS_ELM_MAX_SECTOR_SIZE  512

#define RT_USING_DFS_ROMFS

/* the max number of mounted filesystem */
#define DFS_FILESYSTEMS_MAX         2
/* the max number of opened files   */
#define DFS_FD_MAX                  4
#if USE_METER_FUNC
#undef USE_MODBUS_RTU_MASTER
#else
//modbus function
#define USE_MODBUS_RTU_MASTER
#endif

#endif
