#include <rtthread.h>

#include "utils.h"
#include <system_bsp.h>
#include "gps.h"

#include <protocol.h>
#include <business.h>

static location_report_t location_rep;

location_report_t get_location_report(void)
{
    return location_rep;
}


/*
* 填充位置汇报的数据结构
* @report : 位置汇报数据结构
*/
void gb905_build_report(location_report_t * report){
    GPSINFO gps_info;

    memset(&gps_info, 0, sizeof(gps_info));
    // 
	report->alarm.whole = 0;

	// 
	report->status.whole = 0;

    //set gps info and time
    gps_info = get_gps_info();
    
    report->status.flag.fix = gps_info.Statue?0:1;
	report->status.flag.acc = get_acc_state()?1:0;
    report->status.flag.circuit = get_ouput_state(CIRCUIT_OUT)?1:0;
    report->status.flag.door_lock = get_ouput_state(DOOR_OUT)?1:0;
	
	report->status.flag.loading = get_loading_state()?1:0;


    report->status.whole = EndianReverse32(report->status.whole);

    report->latitude = EndianReverse32(gps_info.Latitude);
	report->longitude = EndianReverse32(gps_info.Longitude);

    report->speed = EndianReverse16(gps_info.Speed);

    report->direction = (uint8_t)(gps_info.dir/2.0);

    gb905_build_timestamp(&(report->timestamp));

}

void sys_build_sensor_info(sensor_ext_info_t* info){
	info->info_id = SENSOR_INFO_ID;
	info->info_len = SENSOR_DAT_LEN;
	
	build_sensor_data(info->info_sensor,SENSOR_DAT_LEN);
}

/*
* 得到需要下次位置汇报的周期
* @setting : 设置参数
*/
unsigned int get_report_period(void){
	unsigned int time_int=0;
		params_setting_t * param; 
		param = get_setting_params();
		if(get_acc_state()>0){
			time_int = param->time_report_accon_interval;
		}else{
			time_int = param->time_report_accoff_interval;
		}
		if(time_int <30) time_int =30;
	
    return time_int;
}

unsigned int get_report1_period(void){
	unsigned int time_int=0;
	function_control_param_t* cur_func_param;
	cur_func_param = get_func_param();
	
	time_int = cur_func_param->time_report_accon_interval;
	#if 0
		if(get_acc_state()>0){
			time_int = cur_func_param->time_report_accon_interval;
		}else{
			time_int = cur_func_param->time_report_accoff_interval;
		}
	#endif
		if(time_int <60) time_int =120;
	
    return time_int;
}

static void gb905_send_report(unsigned char index){
    gb905_location_report_sensor_t gb905_location_report;
    memset((unsigned char*)&gb905_location_report,0,sizeof(gb905_location_report_sensor_t));

    gb905_build_header(&gb905_location_report.header,MESSAGE_LOCATION_REPORT,sizeof(location_report_905_t)+sizeof(sensor_ext_info_t));

    gb905_build_report(&gb905_location_report.report);
    sys_build_sensor_info(&gb905_location_report.sensor_ext);
    gb905_send_data(index,(unsigned char *)&gb905_location_report,sizeof(gb905_location_report_sensor_t));

}

void sys_gb905_report(int index){
    //check int
    gb905_send_report(index);
}

