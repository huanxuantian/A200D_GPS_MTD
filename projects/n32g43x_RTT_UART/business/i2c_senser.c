
#define DBG_LEVEL           DBG_INFO
#define DBG_SECTION_NAME    "MAIN"
#include <rtdbg.h>
#include "board.h"

#include "bh1750.h"
#include "sht3x.h"
#include "business.h"

void check_sensor_data(){
	static uint32_t last_check_sensor_time=0;
	static uint8_t th_first_flag=0;
	static uint8_t lux_first_flag=0;
	rt_device_t bh1750_dev;
	rt_device_t sht3x_dev;
	if(last_check_sensor_time==0||OSTimeGetMS()-last_check_sensor_time>=10*1000)
	{
		last_check_sensor_time = OSTimeGetMS();
		LOG_I("i2c check sensor data now!\r\n");
		bh1750_dev =  rt_device_find("bh1750");
		if(bh1750_dev!=RT_NULL){
				rt_err_t result = RT_EOK;
			struct rt_bh1750_data s_data;
			rt_uint32_t cmd = RT_BH1750_POWER_NORMAL;
			memset(&s_data,0,sizeof(s_data));
			if(lux_first_flag>1){
				lux_first_flag=0;
				cmd = RT_BH1750_POWER_DOWN;
				result = rt_device_control(bh1750_dev, RT_BH1750_CTRL_SET_POWER, (void*)cmd);
				rt_thread_mdelay(50);
				//if(result==RT_EOK){lux_first_flag=1;}
			}
			cmd = RT_BH1750_POWER_NORMAL;
			result = rt_device_control(bh1750_dev, RT_BH1750_CTRL_SET_POWER, (void*)cmd);
			rt_thread_mdelay(20);
			result = rt_device_control(bh1750_dev, RT_BH1750_CTRL_FETCH_REGS, (void*)&s_data);
			debug_print_block_data(0,(uint8_t*)&s_data,sizeof(struct rt_bh1750_data));
			if(result==RT_EOK){
				LOG_I("i2c read lux data:%d\r\n",s_data.value);
				lux_sensor_data_t lux_data;
				lux_data.lux = EndianReverse32(s_data.value);
				update_lux_data(lux_data);
				LOG_I("lux sensor lux %d\r\n", EndianReverse32(lux_data.lux));
			}else{
				LOG_E("i2c read lux data err(result=%d)!!!\r\n",result);
			}
			rt_thread_mdelay(50);
		}
		sht3x_dev =  rt_device_find("sht3x");
		if(sht3x_dev!=RT_NULL){
			rt_err_t th_result = RT_EOK;
			sht3x_status state;
			struct rt_sth3x_data th_data;
			rt_uint32_t cmd = RT_SHT3X_HEATER_ON;
			memset(&state,0,sizeof(state));
			memset(&th_data,0,sizeof(th_data));
			//reset only once
			th_result = rt_device_control(sht3x_dev,RT_SHT3X_CTRL_RESET,RT_NULL);
			rt_thread_mdelay(20);
			if(th_first_flag==0){
				th_first_flag=1;
			//heater on
			cmd = RT_SHT3X_HEATER_ON;
			th_result = rt_device_control(sht3x_dev,RT_SHT3X_CTRL_HEATER,(void*)cmd);
			rt_thread_mdelay(20);
			//heater off
			cmd = RT_SHT3X_HEATER_OFF;
			th_result = rt_device_control(sht3x_dev,RT_SHT3X_CTRL_HEATER,(void*)cmd);
			rt_thread_mdelay(10);

			}
			
			//get state(debug only)
			th_result = rt_device_control(sht3x_dev,RT_SHT3X_CTRL_READ_STATE,&state);
			if(th_result == RT_EOK){
				LOG_I("i2c read state:0x%04x\r\n",state.status_word);
				//read sensor data(value tran ) 
				th_result = rt_device_control(sht3x_dev,RT_SHT3X_CTRL_READ_DATA,&th_data);
				if(th_result == RT_EOK){
					LOG_I("i2c read temp data:%d\r\n",th_data.temperature_value);
					LOG_I("i2c read humi data:%d\r\n",th_data.humidity_value);
					int16_t t_value = th_data.temperature_value;//sign keep only
					thm_sensor_data_t thm_data;
					thm_data.temperature = (unsigned short)t_value;
					thm_data.humidity = (unsigned short)th_data.humidity_value;

					thm_data.temperature = EndianReverse16(thm_data.temperature);
					thm_data.humidity = EndianReverse16(thm_data.humidity);
					update_thm_data(thm_data);
				}
				else{
					LOG_E("i2c read data err(result=%d)!!!\r\n",th_result);
				}
			}
			else{
					LOG_E("i2c read state err(result=%d)!!!\r\n",th_result);
			}
		}
	}
}
