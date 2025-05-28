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
 * @file drv_rtc.c
 * @author Huanxuantian@msn.cn
 * @version V1.2.2
 *
 * @copyright Copyright (c) 2022, Nations Technologies Inc. All rights reserved.
 */
#include "drv_rtc.h"
#ifdef RT_USING_HW_RTC_ONLY
#include "n32g43x.h"
#include "n32g43x_rtc.h"
#endif

#include "gnss_sk.h"

#if defined(RT_USING_RTC) ||defined(RT_USING_HW_RTC_ONLY) || defined(RT_USING_SW_RTC_ONLY)

/* 2018-01-30 14:44:50 = RTC_TIME_INIT(2018, 1, 30, 14, 44, 50)  */
#define RTC_TIME_INIT(year, month, day, hour, minute, second)        \
    {.tm_year = year - 1900, .tm_mon = month - 1, .tm_mday = day, .tm_hour = hour, .tm_min = minute, .tm_sec = second}

#ifndef SOFT_RTC_TIME_DEFAULT
#define SOFT_RTC_TIME_DEFAULT                    RTC_TIME_INIT(2018, 1, 1, 0, 0 ,0)
#endif
#ifdef RT_USING_HW_RTC_ONLY
static RTC_DateType  RTC_DateStructure;
static RTC_DateType  RTC_DateDefault;
static RTC_TimeType  RTC_TimeStructure;
static RTC_TimeType  RTC_TimeDefault;
static RTC_InitType  RTC_InitStructure;
static uint32_t SynchPrediv, AsynchPrediv;
#endif
//使用systik 修正RTC误差，当每次网络或者GPS校对时记录系统tick和对应UTC的秒
//后续读取时钟时获取系统tick并算出相对时间差，当差大于指定fix的秒数后修正
//同时更新当前tick和参考时间
#ifdef RT_USING_SW_RTC_ONLY
static uint32_t ref_time_tick;
static uint32_t ref_time_utc_base;
#endif

static struct rt_device n32g43x_rtc_dev;
static void setutc(time_t time_utc);
//makeDatetimeUTC
static time_t getutc()
{
	unsigned long ref_c_sec;
	unsigned long utc_sec;
	#ifdef RT_USING_SW_RTC_ONLY
	ref_c_sec = ref_time_utc_base + ((rt_tick_get() - ref_time_tick)/RT_TICK_PER_SECOND);
	utc_sec = ref_c_sec;
	#endif
	#ifdef RT_USING_HW_RTC_ONLY
	DateTime datetime;
	//get from rtc
	RTC_GetDate(RTC_FORMAT_BIN, &RTC_DateStructure);
	RTC_GetTime(RTC_FORMAT_BIN, &RTC_TimeStructure);
	datetime._year = RTC_DateStructure.Year + 2000;
	datetime._month = RTC_DateStructure.Month;
	datetime._day = RTC_DateStructure.Date;
	datetime._week = RTC_DateStructure.WeekDay;
	datetime._hour = RTC_TimeStructure.Hours;
	datetime._minute = RTC_TimeStructure.Minutes;
	datetime._second = RTC_TimeStructure.Seconds;
	
	utc_sec = makeDatetimeUTC(datetime);
	#endif
	return utc_sec;
}

static void setutc(time_t time_utc){
	#ifdef RT_USING_SW_RTC_ONLY
		ref_time_utc_base = time_utc - (rt_tick_get() - ref_time_tick) / RT_TICK_PER_SECOND;
	#endif
	#ifdef RT_USING_HW_RTC_ONLY
	DateTime datetime;
	datetime = makeDateTime_utc_zone(time_utc,0);
	//set to rtc;
	RTC_DateStructure.Year = datetime._year -2000;
	RTC_DateStructure.Month = datetime._month;
	RTC_DateStructure.Date = datetime._day;
	RTC_TimeStructure.Hours = datetime._hour;
	RTC_TimeStructure.Minutes = datetime._minute;
	RTC_TimeStructure.Seconds = datetime._second;
	RTC_DateStructure.WeekDay = datetime._week;
	
	  if (RTC_SetDate(RTC_FORMAT_BIN, &RTC_DateStructure) == ERROR)
    {
        rt_kprintf("\n\r>> !! RTC Set Date failed. !! <<\n\r");
			return;
    }
		if (RTC_ConfigTime(RTC_FORMAT_BIN, &RTC_TimeStructure) == ERROR)
    {
        rt_kprintf("\n\r>> !! RTC Set Time failed. !! <<\n\r");
    }
	#endif
}

static rt_err_t n3243x_rtc_control(rt_device_t dev, int cmd, void *args)
{
    time_t *time;
    struct tm time_temp;

    RT_ASSERT(dev != RT_NULL);
    memset(&time_temp, 0, sizeof(struct tm));

    switch (cmd)
    {
        case RT_DEVICE_CTRL_RTC_GET_TIME:
            time = (time_t *) args;
            *time = getutc();
            
            break;

        case RT_DEVICE_CTRL_RTC_SET_TIME:
        {
            time = (time_t *) args;
            //init_time = *time - (rt_tick_get() - init_tick) / RT_TICK_PER_SECOND;
						setutc(*time);
            break;
        }
    }

    return RT_EOK;
}

#ifdef RT_USING_HW_RTC_ONLY
/**
 * @brief  Display the current Date on the Hyperterminal.
 */
void RTC_DateShow(void)
{
    /* Get the current Date */
    RTC_GetDate(RTC_FORMAT_BIN, &RTC_DateStructure);
    rt_kprintf("\n\r //=========== Current Date Display ==============// \n\r");
    rt_kprintf("\n\r The current date (WeekDay-Date-Month-Year) is :  %02u-%02u-%02u-%02u \n\r",\
             RTC_DateStructure.WeekDay,\
             RTC_DateStructure.Date,\
             RTC_DateStructure.Month,\
             RTC_DateStructure.Year);
}

/**
 * @brief  Display the current time on the Hyperterminal.
 */
void RTC_TimeShow(void)
{
    /* Get the current Time and Date */
    RTC_GetTime(RTC_FORMAT_BIN, &RTC_TimeStructure);
    rt_kprintf("\n\r //============ Current Time Display ===============// \n\r");
    rt_kprintf("\n\r The current time (Hour-Minute-Second) is :  %02u:%02u:%02u \n\r",\
             RTC_TimeStructure.Hours,\
             RTC_TimeStructure.Minutes,\
             RTC_TimeStructure.Seconds);
    /* Unfreeze the RTC DAT Register */
    (void)RTC->DATE;
}

static void RTC_DateAndTimeDefaultVale(struct tm time_new){
// Date
    RTC_DateDefault.WeekDay = time_new.tm_wday;
    RTC_DateDefault.Date    = time_new.tm_mday;
    RTC_DateDefault.Month   = time_new.tm_mon+1;
    RTC_DateDefault.Year    = time_new.tm_year - 100;
    // Time
    RTC_TimeDefault.H12     = RTC_AM_H12;
    RTC_TimeDefault.Hours   = time_new.tm_hour;
    RTC_TimeDefault.Minutes = time_new.tm_min;
    RTC_TimeDefault.Seconds = time_new.tm_sec;
}

/**
 * @brief  RTC date regulate with the default value.
 * @return An ErrorStatus enumeration value:
 *          - SUCCESS: RTC date regulate success
 *          - ERROR: RTC date regulate failed
 */
static  ErrorStatus RTC_DateRegulate(void)
{
    unsigned int tmp_hh = 0xFF, tmp_mm = 0xFF, tmp_ss = 0xFF;
    tmp_hh = RTC_DateDefault.WeekDay;
    if (tmp_hh == 0xff)
    {
        return ERROR;
    }
    else
    {
        RTC_DateStructure.WeekDay = tmp_hh;
    }
    tmp_hh = 0xFF;
    tmp_hh = RTC_DateDefault.Date;
    if (tmp_hh == 0xff)
    {
        return ERROR;
    }
    else
    {
        RTC_DateStructure.Date = tmp_hh;
    }
    tmp_mm = RTC_DateDefault.Month;
    if (tmp_mm == 0xff)
    {
        return ERROR;
    }
    else
    {
        RTC_DateStructure.Month = tmp_mm;
    }
    tmp_ss = RTC_DateDefault.Year;
    if (tmp_ss == 0xff)
    {
        return ERROR;
    }
    else
    {
        RTC_DateStructure.Year = tmp_ss;
    }
    /* Configure the RTC date register */
    if (RTC_SetDate(RTC_FORMAT_BIN, &RTC_DateStructure) == ERROR)
    {
        rt_kprintf("\n\r>> !! RTC Set Date failed. !! <<\n\r");
        return ERROR;
    }
    else
    {
        rt_kprintf("\n\r>> !! RTC Set Date success. !! <<\n\r");
        RTC_DateShow();
        return SUCCESS;
    }
}

/**
 * @brief  RTC time regulate with the default value.
 * @return An ErrorStatus enumeration value:
 *          - SUCCESS: RTC time regulate success
 *          - ERROR: RTC time regulate failed
 */
static  ErrorStatus RTC_TimeRegulate(void)
{
    unsigned int tmp_hh = 0xFF, tmp_mm = 0xFF, tmp_ss = 0xFF;
    rt_kprintf("\n\r //==============Time Settings=================// \n\r");
    RTC_TimeStructure.H12 = RTC_TimeDefault.H12;
    rt_kprintf("\n\r Please Set Hours \n\r");
    tmp_hh = RTC_TimeDefault.Hours;
    if (tmp_hh == 0xff)
    {
        return ERROR;
    }
    else
    {
        RTC_TimeStructure.Hours = tmp_hh;
    }
    rt_kprintf(": %02u\n\r", tmp_hh);
    rt_kprintf("\n\r Please Set Minutes \n\r");
    tmp_mm = RTC_TimeDefault.Minutes;
    if (tmp_mm == 0xff)
    {
        return ERROR;
    }
    else
    {
        RTC_TimeStructure.Minutes = tmp_mm;
    }
    rt_kprintf(": %02u\n\r", tmp_mm);
    rt_kprintf("\n\r Please Set Seconds \n\r");
    tmp_ss = RTC_TimeDefault.Seconds;
    if (tmp_ss == 0xff)
    {
        return ERROR;
    }
    else
    {
        RTC_TimeStructure.Seconds = tmp_ss;
    }
    rt_kprintf(": %02u\n\r", tmp_ss);
    /* Configure the RTC time register */
    if (RTC_ConfigTime(RTC_FORMAT_BIN, &RTC_TimeStructure) == ERROR)
    {
        rt_kprintf("\n\r>> !! RTC Set Time failed. !! <<\n\r");
        return ERROR;
    }
    else
    {
        rt_kprintf("\n\r>> !! RTC Set Time success. !! <<\n\r");
        RTC_TimeShow();
        return SUCCESS;
    }
}


/**
 * @brief  RTC prescaler config.
 */
static  void RTC_PrescalerConfig(void)
{
    /* Configure the RTC data register and RTC prescaler */
    RTC_InitStructure.RTC_AsynchPrediv = AsynchPrediv;
    RTC_InitStructure.RTC_SynchPrediv  = SynchPrediv;
    RTC_InitStructure.RTC_HourFormat   = RTC_24HOUR_FORMAT;
    /* Check on RTC init */
    if (RTC_Init(&RTC_InitStructure) == ERROR)
    {
        rt_kprintf("\r\n //******* RTC Prescaler Config failed **********// \r\n");
    }
}

/**
 * @brief  Configures the RTC Source Clock Type.
 * @param Clk_Src_Type specifies RTC Source Clock Type.
 *   This parameter can be on of the following values:
 *     @arg RTC_CLK_SRC_TYPE_HSE128
 *     @arg RTC_CLK_SRC_TYPE_LSE
 *     @arg RTC_CLK_SRC_TYPE_LSI
 * @param Is_First_Cfg_RCC specifies Is First Config RCC Module.
 *   This parameter can be on of the following values:
 *     @arg true
 *     @arg false
 * @return An ErrorStatus enumeration value:
 *          - SUCCESS: RTC clock configure success
 *          - ERROR: RTC clock configure failed
 */
static ErrorStatus RTC_CLKSourceConfig(RTC_CLK_SRC_TYPE Clk_Src_Type, bool Is_First_Cfg_RCC)
{
    uint8_t lse_ready_count=0;
    ErrorStatus Status=SUCCESS;
    /* Enable the PWR clock */
    RCC_EnableAPB1PeriphClk(RCC_APB1_PERIPH_PWR, ENABLE);
    RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_AFIO, ENABLE);
    /* Allow access to RTC */
    PWR_BackupAccessEnable(ENABLE);
    /* Disable RTC clock */
    RCC_EnableRtcClk(DISABLE);
    if (RTC_CLK_SRC_TYPE_HSE_DIV32 == Clk_Src_Type)
    {
        rt_kprintf("\r\n RTC_ClkSrc Is Set HSE/32! \r\n");
        if (true == Is_First_Cfg_RCC )
        {
            /* Enable HSE */
            RCC_EnableLsi(DISABLE);
            RCC_ConfigHse(RCC_HSE_ENABLE);
            while (RCC_WaitHseStable() == ERROR)
            {
            }
            RCC_ConfigRtcClk(RCC_RTCCLK_SRC_HSE_DIV32);
        }
        else
        {
            RCC_EnableLsi(DISABLE);
            RCC_ConfigRtcClk(RCC_RTCCLK_SRC_HSE_DIV32);
            /* Enable HSE */
            RCC_ConfigHse(RCC_HSE_ENABLE);
            while (RCC_WaitHseStable() == ERROR)
            {
            }
        }
        SynchPrediv  = 0x7A0; // 8M/32 = 250KHz
        AsynchPrediv = 0x7F;  // value range: 0-7F
    }
    else if (RTC_CLK_SRC_TYPE_LSE == Clk_Src_Type)
    {
        rt_kprintf("\r\n RTC_ClkSrc Is Set LSE! \r\n");
        if (true == Is_First_Cfg_RCC)
        {
            /* Enable the LSE OSC32_IN PC14 */
            RCC_EnableLsi(DISABLE); // LSI is turned off here to ensure that only one clock is turned on
    #if (_TEST_LSE_BYPASS_)
            RCC_ConfigLse(RCC_LSE_BYPASS,0x1FF);
    #else
            RCC_ConfigLse(RCC_LSE_ENABLE,0x1FF);
    #endif
            lse_ready_count=0;
            /****Waite LSE Ready *****/
            while((RCC_GetFlagStatus(RCC_LDCTRL_FLAG_LSERD) == RESET) && (lse_ready_count<RTC_LSE_TRY_COUNT))
            {
                lse_ready_count++;
                rt_thread_mdelay(10);
                /****LSE Ready failed or timeout*****/
                if(lse_ready_count>=RTC_LSE_TRY_COUNT)
                {
                    Status = ERROR;
                    rt_kprintf("\r\n RTC_ClkSrc Set LSE Faile! \r\n");
                    break;
                }
            }
            RCC_ConfigRtcClk(RCC_RTCCLK_SRC_LSE);
        }
        else
        {
            /* Enable the LSE OSC32_IN PC14 */
            RCC_EnableLsi(DISABLE);
            RCC_ConfigRtcClk(RCC_RTCCLK_SRC_LSE);
    #if (_TEST_LSE_BYPASS_)
            RCC_ConfigLse(RCC_LSE_BYPASS,0x1FF);
    #else
            RCC_ConfigLse(RCC_LSE_ENABLE,0x1FF);
    #endif
            lse_ready_count=0;
            /****Waite LSE Ready *****/
            while((RCC_GetFlagStatus(RCC_LDCTRL_FLAG_LSERD) == RESET) && (lse_ready_count<RTC_LSE_TRY_COUNT))
            {
                lse_ready_count++;
                rt_thread_mdelay(10);
                /****LSE Ready failed or timeout*****/
                if(lse_ready_count>=RTC_LSE_TRY_COUNT)
                {
                   Status = ERROR;
                   rt_kprintf("\r\n RTC_ClkSrc Set LSE Faile! \r\n");
                   break;
                }
            }
        }
        SynchPrediv  = 0xFF; // 32.768KHz
        AsynchPrediv = 0x7F; // value range: 0-7F
    }
    else if (RTC_CLK_SRC_TYPE_LSI == Clk_Src_Type)
    {
        rt_kprintf("\r\n RTC_ClkSrc Is Set LSI! \r\n");
        if (true == Is_First_Cfg_RCC)
        {
            /* Enable the LSI OSC */
            RCC_EnableLsi(ENABLE);
            while (RCC_GetFlagStatus(RCC_CTRLSTS_FLAG_LSIRD) == RESET)
            {
            }
            RCC_ConfigRtcClk(RCC_RTCCLK_SRC_LSI);
        }
        else
        {
            RCC_ConfigRtcClk(RCC_RTCCLK_SRC_LSI);
            /* Enable the LSI OSC */
            RCC_EnableLsi(ENABLE);
            while (RCC_GetFlagStatus(RCC_CTRLSTS_FLAG_LSIRD) == RESET)
            {
            }
        }
        SynchPrediv  = 0x137; // 40kHz
				//SynchPrediv  = 0x14A; // 41828Hz
        AsynchPrediv = 0x7F;  // value range: 0-7F
    }
    else
    {
        rt_kprintf("\r\n RTC_ClkSrc Value is error! \r\n");
    }
    /* Enable the RTC Clock */
    RCC_EnableRtcClk(ENABLE);
    RTC_WaitForSynchro();
    return Status;
}

/**
 * @brief  Writes user data to the specified Data Backup Register.
 * @param BKP_DAT specifies the Data Backup Register.
 *   This parameter can be BKP_DATx where x:[1, 20]
 * @param Data data to write
 */
void BKP_WriteBkpData(uint32_t BKP_DAT, uint32_t Data)
{
    __IO uint32_t tmp = 0;
    /* Check the parameters */
    assert_param(IS_BKP_DAT(BKP_DAT));
    tmp = (uint32_t)&RTC->BKP1R;
    tmp += BKP_DAT;
    *(__IO uint32_t*)tmp = Data;
}

/**
 * @brief  Reads data from the specified Data Backup Register.
 * @param BKP_DAT specifies the Data Backup Register.
 *   This parameter can be BKP_DATx where x:[1, 20]
 * @return The content of the specified Data Backup Register
 */
uint32_t BKP_ReadBkpData(uint32_t BKP_DAT)
{
    __IO uint32_t tmp = 0;
    uint32_t value = 0;
    /* Check the parameters */
    assert_param(IS_BKP_DAT(BKP_DAT));
    tmp = (uint32_t)&RTC->BKP1R;
    tmp += BKP_DAT;
    value = (*(__IO uint32_t*)tmp);
    return value;
}

#endif

static rt_err_t n3243x_rtc_init(rt_device_t dev){
		
    struct tm time_new = SOFT_RTC_TIME_DEFAULT;
		#ifdef RT_USING_SW_RTC_ONLY
		time_t time_utc;
		DateTime datetime;
		datetime._year = time_new.tm_year+1900;
		datetime._month = time_new.tm_mon+1;
		datetime._day = time_new.tm_mday;
	
		datetime._hour = time_new.tm_hour;
		datetime._minute = time_new.tm_min;
		datetime._second = time_new.tm_sec;
		time_utc = makeDatetimeUTC(datetime);
		ref_time_utc_base = time_utc;
		ref_time_tick = rt_tick_get();
		#endif
		#ifdef RT_USING_HW_RTC_ONLY
    RTC_DateAndTimeDefaultVale(time_new);
    /* Enable the PWR clock */
    RCC_EnableAPB1PeriphClk(RCC_APB1_PERIPH_PWR, ENABLE);
    /* Allow access to RTC */
    PWR_BackupAccessEnable(ENABLE);
    if (USER_WRITE_BKP_DAT1_DATA != BKP_ReadBkpData(BKP_DAT1) )
    {
        /* RTC clock source select */
        if(SUCCESS==RTC_CLKSourceConfig(RTC_CLK_SRC_TYPE_LSI, true))
        {
           RTC_PrescalerConfig();
           /* Adjust time by values entered by the user on the hyperterminal */
           RTC_DateRegulate();
           RTC_TimeRegulate();
           BKP_WriteBkpData(BKP_DAT1, USER_WRITE_BKP_DAT1_DATA);
           rt_kprintf("\r\n RTC Init Success\r\n");
        }
        else
        {
           rt_kprintf("\r\n RTC Init Faile\r\n");
        }
    }else{
        rt_kprintf("rtc no need to init again!!!");
    }
		#endif
    return RT_EOK;
}

#ifdef RT_USING_DEVICE_OPS
const static struct rt_device_ops soft_rtc_ops = 
{
    RT_NULL,
    RT_NULL,
    RT_NULL,
    RT_NULL,
    RT_NULL,
    n3243x_rtc_control
};
#endif


int rt_hw_rtc_init(void)
{
    static rt_bool_t init_ok = RT_FALSE;

    if (init_ok)
    {
        return 0;
    }
    /* make sure only one 'rtc' device */
    RT_ASSERT(!rt_device_find("rtc"));

    n32g43x_rtc_dev.type    = RT_Device_Class_RTC;

    /* register rtc device */
#ifdef RT_USING_DEVICE_OPS
    n32g43x_rtc_dev.ops     = &soft_rtc_ops;
#else
    n32g43x_rtc_dev.init    = RT_NULL;
    n32g43x_rtc_dev.open    = RT_NULL;
    n32g43x_rtc_dev.close   = RT_NULL;
    n32g43x_rtc_dev.read    = RT_NULL;
    n32g43x_rtc_dev.write   = RT_NULL;
    n32g43x_rtc_dev.control = n3243x_rtc_control;
#endif
		n3243x_rtc_init(&n32g43x_rtc_dev);
    /* no private */
    n32g43x_rtc_dev.user_data = RT_NULL;

    rt_device_register(&n32g43x_rtc_dev, "rtc", RT_DEVICE_FLAG_RDWR);

    init_ok = RT_TRUE;

    return 0;
}
INIT_DEVICE_EXPORT(rt_hw_rtc_init);


#endif /* RT_USING_RTC */

