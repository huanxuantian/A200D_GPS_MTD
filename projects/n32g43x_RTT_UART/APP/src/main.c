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

#include <rtthread.h>
#define DBG_LEVEL           DBG_INFO
#define DBG_SECTION_NAME    "MAIN"
#include <rtdbg.h>

#include <system_bsp.h>
#include "gnss_sk.h"
#include "gsm.h"
#include <business.h>
#include <protocol.h>

ALIGN(RT_ALIGN_SIZE)
struct rt_event system_event;
/**
 * @brief  Main program
 */
int main(void)
 {
		
    rt_err_t result;
	rt_uint8_t gnss_stack[ 1024 ];
	struct rt_thread gnss_thread;

	// #if USE_INPUT_BLOCK
	// init_block_data();
	// #endif
	//init_spi_flash();
	//http_update_info_init();
	//setting_init();
	//init_adc_config();
    /* init uart_rx_event */
    result = rt_event_init(&system_event, "event", RT_IPC_FLAG_FIFO);
    if (result != RT_EOK)
    {
        LOG_E("init event failed.");
    }
	#if USE_MODBUS_THREAD_TASK
	start_modbus_thread();
	#endif
	init_gnss_thread(&gnss_thread,"gnss",&gnss_stack[0],sizeof(gnss_stack));
	init_gsm_thread();
    while(1)
    {
        rt_thread_mdelay(50);
		wd_feed();			
		led_link_handle();
    }
}

/*@}*/
