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
 * @file board.c
 * @author Nations
 * @version V1.2.2
 *
 * @copyright Copyright (c) 2022, Nations Technologies Inc. All rights reserved.
 */

#include <rthw.h>
#include <rtthread.h>
#include "n32g43x.h"
#include "board.h"
#include <system_bsp.h>
#include "utils.h"
#include "protocol.h"

#if   defined ( __CC_ARM )
#define AT_FLASH(X) __attribute__((at(X)))
#elif defined ( __GNUC__ )
#define AT_FLASH(X) 
#endif
//32 bytes for version at 0x8002000 +0x200 
// for gcc ,this defined in ld script
char const FAPP_VERSION[] AT_FLASH(0x8002200) = "SHINKI_A200_0001_0005_A200 12345";


#if defined (__GNUC__)
void __libc_init_array(void)
{
    /* we not use __libc init_aray to initialize C++ objects */
}
#endif
/**
 * @brief  Configures Vector Table base location.
 */
void NVIC_Configuration(void)
{
#ifdef  VECT_TAB_RAM
    /* Set the Vector Table base location at 0x20000000 */
    NVIC_SetVectorTable(NVIC_VectTab_RAM, 0x0);
#else  /* VECT_TAB_FLASH  */
    /* Set the Vector Table base location at 0x08000000 */
    //NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x0);
		NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x2000);
#endif
}

/**
 * @brief This is the timer interrupt service routine.
 */
void SysTick_Handler(void)
{
    /* enter interrupt */
    rt_interrupt_enter();

    rt_tick_increase();

    /* leave interrupt */
    rt_interrupt_leave();
}

void rt_system_reset()
{
    rt_uint32_t level;
    #ifdef DEBUG_UPDATE
    handle_update_flag();
    rt_thread_mdelay(200);
    #endif
    level = rt_hw_interrupt_disable();
    NVIC_SystemReset();
    rt_hw_interrupt_enable(level);//if fault only reenable interrupt,otherwise never run to here
}

void rt_show_appversion(void){
		rt_kprintf("\n=======================================\n");
		rt_kprintf(" \\ | /\n");
		rt_kprintf(" FT APPVERSION: %s build %s [%s]\n",SW_VERSION_APP, __DATE__,__TIME__);
		rt_kprintf(" / | \\    \n");
		rt_kprintf(" 200x - 2024 Copyright by SHINKI team \n");
		rt_kprintf(" APP_FLAG %s\n",FAPP_VERSION);
		rt_kprintf("=======================================\n");

}
#include "MicroNMEA.h"
void rt_show_date_time(){
    time_t now;
    DateTime datetime;
    rt_device_t rtc_dev;
    rtc_dev = rt_device_find("rtc");
    if(rtc_dev!=RT_NULL){
        rt_device_control(rtc_dev, RT_DEVICE_CTRL_RTC_GET_TIME, &now);
        datetime = makeDateTime_utc_zone(now,0);//local time
        rt_kprintf("datetime:%04d-%02d-%02d %02d:%02d:%02d/WEEK:%d\n",
        datetime._year,datetime._month,datetime._day,
        datetime._hour,datetime._minute,datetime._second,
        datetime._week);

    }else{
        rt_kprintf("rtc device not found!!!\n");
    }
}

/**
 * @brief This function will initial N32G45x board.
 */
void rt_hw_board_init()
{
    /* NVIC Configuration */
    NVIC_Configuration();

    /* Configure the SysTick */
    SysTick_Config(SystemCoreClock / RT_TICK_PER_SECOND);   /* 10ms */
    
#ifdef RT_USING_HEAP
    /* init memory system */
    rt_system_heap_init((void *)N32G43X_SRAM_START, (void *)N32G43X_SRAM_END);
#endif //RT_USING_HEAP
    
    /* Call components board initial (use INIT_BOARD_EXPORT()) */
#ifdef RT_USING_COMPONENTS_INIT
    rt_components_board_init();
#endif

#ifdef RT_USING_CONSOLE
    rt_console_set_device(RT_CONSOLE_DEVICE_NAME);
#endif
    //bsp_init();
	rt_show_appversion();
    
}

/*@}*/
