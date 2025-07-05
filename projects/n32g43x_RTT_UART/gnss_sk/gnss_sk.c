#define DBG_LVL    DBG_WARNING
#define DBG_TAG    "GNSS"
#include <rtdbg.h>

#include "utils.h"

#include "pin.h"
#include <gps.h>
#include "gnss_port.h"
#include "gnss_if.h"
#include "gnss_sk.h"
#include <system_bsp.h>
#include <business.h>


static void gnss_power_reset()
{
    rt_pin_write(GNSS_PWR_PIN, PIN_LOW);
    rt_thread_mdelay(50);
    rt_pin_write(GNSS_PWR_PIN, PIN_HIGH);
}

void gnss_io_init()
{
    rt_pin_mode(GNSS_PWR_PIN, PIN_MODE_OUTPUT);

    rt_pin_mode(ANT_IN_NON_PIN, PIN_MODE_INPUT_PULLUP);
    rt_pin_mode(ANT_IN_NSHORT_PIN, PIN_MODE_INPUT_PULLUP);
    gnss_power_reset();
}
static uint8_t pps_flag=0;
void change_pps(){
    if(pps_flag)
    {
        //rt_pin_write(GNSS_PWR_PIN, PIN_LOW);
			pps_flag=0;
    }else{
        //rt_pin_write(GNSS_PWR_PIN, PIN_HIGH);
				pps_flag=1;
    }
    
}

static int gnss_paser_callback(gnss_ctx_t *ctx,int read_len,int ack_flag){
	#if GPS_DBG
	int i=0;
	LOG_I("parse req rec_len=%d",read_len);
	for(i=0;i<read_len;i++){
	if(i==0){LOG_I("data recv size=%d[",read_len);}
	#if (DBG_LEVEL >= DBG_INFO)
	LOG_RAW("%c",*(ctx->read_buf+i));
	#endif
	if(i==(read_len-1)){LOG_I("]");}
	}
	#endif
	(void)(ctx);
	gps_paser_callback(ctx->read_buf,read_len);

	return 0;
}


static void gnss_thread_entry(void* parameter){
	int ret = GNSS_RT_EOK;
	time_t now;
	unsigned long utc_gnss;
	DateTime datetime_utc;
	gnss_device_t gnss_dev;
	rt_device_t rtc_dev;
	uint8_t uid_buf[32];
	int acc_state=0;
	int acc =-1;
	int last_acc =-1;

	rt_uint32_t adc_v1_value = 0;
	rt_uint32_t adc_v2_value = 0;
	uint32_t last_check_rtc_time=0;
	#if USE_METER_FUNC
	#else
	#if USE_MODBUS_THREAD_TASK
	#else
	uint32_t last_check_sensor_time=0;
	#endif
	#endif

	//input acc state
	acc_state = acc_state_poll(0);
	if(acc_state==ACC_ON_DETECT)
	{
		acc=1;
	}else{
		acc=0;
	}
	LOG_I("acc init=%d ",acc);
	last_acc = acc;
	//update_acc_state(acc);

	gnss_io_init();
	GpsDataInit();
	#if USE_METER_FUNC
	meter_init();
	#else
	#if USE_MODBUS_THREAD_TASK
	#else
	modbus_init();
	#endif
	#endif
	gnss_dev = create_gnss_modify_buffer(256);
	if(gnss_dev==NULL){
		LOG_E("gnss_dev init master error");
		return;
	}
	ret = gnss_set_serial(gnss_dev,GNSS_SERIAL_NAME,9600,8,'N',STOP_BITS_1,0);
	if(GNSS_RT_EOK != ret ){
		LOG_E("gnss_dev init uart error");
		return;
	}
	//set callback
	gnss_set_parse_callback(gnss_dev,gnss_paser_callback);
	ret = gnss_open_default(gnss_dev);
	if(GNSS_RT_EOK != ret ){
		LOG_E("gnss_dev open uart error");
		return;
	}

		//adc power 
	adc_v1_value =get_pwr_mv();
    LOG_I("the PC0 voltage value is %d mV", adc_v1_value);
		rt_thread_mdelay(20);
		adc_v2_value = get_ext_mv();
    LOG_I("the PC1 voltage value is %d mV", adc_v2_value);
	#if USE_METER_FUNC
	#else
	#if USE_MODBUS_THREAD_TASK
	#else
	last_check_sensor_time = OSTimeGet();
	#endif
	#endif
    while(1)
    {
        rt_thread_mdelay(20);
		sync_gpsinfo();
		//block for check acc deley key_value check
		acc_state = acc_state_poll(1);
		if(acc_state==ACC_ON_DETECT)
		{
			acc=1;
		}else if(acc_state==ACC_OFF_DETECT){ 
			acc=0;
		}
		if(acc!=last_acc){
			LOG_W("acc change: %d ",acc);
			last_acc = acc;
			update_acc_state(acc);
			//control_output(1,acc);
			//control_output(2,acc>0?0:1);
		}
		#if USE_METER_FUNC
		handle_meter_task();
		#else
		#if USE_MODBUS_THREAD_TASK
		#else
		if(OSTimeGet() - last_check_sensor_time > 10)
		{
			last_check_sensor_time = OSTimeGet();
			handle_modbus_task();
		}
		#endif
		#endif
		if(OSTimeGet()-last_check_rtc_time>=60){
			adc_v1_value =get_pwr_mv();
			LOG_I("the PC0 voltage value is %d mV", adc_v1_value);
			rt_thread_mdelay(20);
			adc_v2_value = get_ext_mv();
			LOG_I("the PC1 voltage value is %d mV", adc_v2_value);
			rtc_dev = rt_device_find("rtc");
			if(rtc_dev!=RT_NULL){
				rt_device_control(rtc_dev, RT_DEVICE_CTRL_RTC_GET_TIME, &now);
			}else{
				LOG_W("rtc device not found!!!");
			}
			LOG_I("read from rtc:");
			debug_utc(now);
			
			if(get_gps_status()>0){
				LOG_I("read from gnss:");
				datetime_utc = makeDateTime();
				utc_gnss = makeDatetimeUTC(datetime_utc);
				debug_utc(utc_gnss);
				if((utc_gnss >now && utc_gnss - now > 3*60)||
					(now > utc_gnss && now - utc_gnss > 3*60)){
					LOG_I("sync from gnss:");
					if(rtc_dev!=RT_NULL){
					rt_device_control(rtc_dev, RT_DEVICE_CTRL_RTC_SET_TIME, &utc_gnss);	
					rt_thread_mdelay(20);
					rt_device_control(rtc_dev, RT_DEVICE_CTRL_RTC_GET_TIME, &now);
					}
					LOG_I("read from rtc:");
					debug_utc(now);
				}
			}
			memset(&uid_buf[0],0,sizeof(uid_buf));
			build_sensor_data(&uid_buf[0],sizeof(uid_buf));
			LOG_W("build_sensor_data:");
			debug_print_block_data(0,&uid_buf[0],sizeof(uid_buf));

			last_check_rtc_time = OSTimeGet();
		}

	}
}

void init_gnss_thread(struct rt_thread *gnss_thread,
                        const char       *name,
						void             *stack_start,
                        rt_uint32_t       stack_size)
{
    rt_err_t result;
	/* init gnss device thread */
	result = rt_thread_init(gnss_thread, name, gnss_thread_entry, RT_NULL, (rt_uint8_t*)stack_start, stack_size, 6, 5);
    if (result == RT_EOK)
    {
        rt_thread_startup(gnss_thread);
    }
}
