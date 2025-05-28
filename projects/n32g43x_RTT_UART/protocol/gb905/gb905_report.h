#ifndef _GB905_REPORT_H
#define	_GB905_REPORT_H
//关于位置汇报部分:国标905与苏标adas(808)有较大差别:报警标志定义/状态定义基本都不相同
#ifdef __cplusplus
extern "C" {
#endif
#include "utils.h"

// 报警标志定义
typedef union 
{
	unsigned int whole;
	
	struct
	{
		unsigned emergency:1;				// 紧急报警
		unsigned early_warning:1;			// 预警
		unsigned gps_fault:1;				// GPS 模块故障
		unsigned gps_antenna_cut_fault:1;	// GPS 天线未接或被剪断
		unsigned gps_antenna_short_fault:1;	// GPS 天线短路
		unsigned under_voltage:1;			// 电源欠压
		unsigned power_down:1;				// 电源掉电
		unsigned lcd_device_fault:1;		// 显示终端故障
		unsigned tts_fault:1;				// tts 模块故障
		unsigned camera_fault:1;			// 摄像头故障
		unsigned meter_fault:1;				// 计价器故障
		unsigned eval_fault:1;				// 服务评价器故障
		unsigned led_adver_fault:1;			// LED 广告屏故障
		unsigned lcd_module_fault:1;		// LCD 显示屏故障
		unsigned tsm_faule:1;				// 安全模块故障
		unsigned led_light_fault:1;			// LED 顶灯故障
		unsigned overspeed:1;				// 超速报警
		unsigned driving_timeout:1;			// 连续驾驶超时
		unsigned day_driving_timeout:1;		// 当天累计驾驶超时
		unsigned parking_timeout:1;			// 超时停车
		unsigned out_of_area:1;				// 进出区域/路线
		unsigned road_driving:1;			// 路段行驶时间不足/过长
		unsigned prohibited_area:1;			// 禁行路段行驶
		unsigned sersor_fault:1;			// 车速传感器故障
		unsigned illegal_ignition:1;		// 车辆非法点火
		unsigned illegal_displacement:1;	// 车辆非法位移
		unsigned abnormal_storage:1;		// 终端存储异常
		unsigned reserved:2;
		#ifndef NEW_EXT_FLAG
		unsigned meter_cheat:1;
		#else
		unsigned button_alarm_fault:1;	//报警按钮故障
		#endif
		unsigned int:0;
    }__packed flag;
}__packed report_alarm_t;

// 状态定义
typedef union 
{
	unsigned whole;
	
	struct
	{
		unsigned fix:1;					// =0,已卫星定位;=1,未卫星定位
		unsigned lat:1;					// =0,北纬;=1,南纬
		unsigned lon:1;					// =0,东经;=1,西经
		unsigned operating:1;			// =0,营运状态;=1,停运状态
		unsigned reservation:1;			// =0,未预约;=1,预约
		//unsigned reserved:3;//
		unsigned empty2loading:1;//shenzhen  flag
		unsigned loading2empty:1;//shenzhen  flag
		#ifdef NEW_EXT_FLAG
		unsigned screen_state:1;			// =0,正常营运;=1,空车牌暂停
		#else
		unsigned reserved:1;
		#endif
		unsigned acc:1;					// =0,ACC 关;=1,ACC 开
		unsigned loading:1;				// =0,空车;=1,重车
		unsigned oil:1;					// =0,车辆油路正常;=1,车辆油路断开
		unsigned circuit:1;				// =0,车辆电路正常;=1,车辆电路断开
		unsigned door_lock:1;			// =0,车门解锁;=1,车门加锁
		unsigned car_lock:1;			//=0，车辆解锁;=1，车辆锁定
		unsigned :0;
    }__packed flag;
}__packed report_status_t;


// 位置汇报的网络传输数据格式
typedef struct {
	report_alarm_t	alarm;				// 报警标志
	report_status_t status;				// 状态
	
	unsigned int latitude;				// 纬度 (1/10000 分)
	unsigned int longitude;				// 经度 (1/10000 分)
	#ifdef	USE_GB808
	unsigned short high;
    #endif
	unsigned short speed;				// 速度 (1/10KM/H)
	#ifdef	USE_GB808
    unsigned short direction;// 方向 (0~359,刻度=1  度,正北为0,顺时针)
    #else
    unsigned char direction;			// 方向 (0~178,刻度=2  度,正北为0,顺时针)
    #endif

	gb905_bcd_timestamp_t timestamp;	// 时间 (BCD码 YY-MM-DD-hh-mm-ss)
}__packed location_report_t;

typedef struct {
	report_alarm_t	alarm;				// 报警标志
	report_status_t status;				// 状态
	
	unsigned int latitude;				// 纬度 (1/10000 分)
	unsigned int longitude;				// 经度 (1/10000 分)
	unsigned short speed;				// 速度 (1/10KM/H)
    unsigned char direction;			// 方向 (0~178,刻度=2  度,正北为0,顺时针)

	gb905_bcd_timestamp_t timestamp;	// 时间 (BCD码 YY-MM-DD-hh-mm-ss)
}__packed location_report_905_t;

typedef  struct
{
	unsigned char start_magic_id;
    gb905_msg_header_t header;
    location_report_905_t report;
    unsigned char _xor;
    unsigned char end_magic_id;
} __packed gb905_location_report_base_t;

typedef struct {
	report_alarm_t	alarm;				// 报警标志
	report_status_t status;				// 状态
	
	unsigned int latitude;				// 纬度 (1/10000 分)
	unsigned int longitude;				// 经度 (1/10000 分)
	unsigned short high;				//m
	unsigned short speed;				// 速度 (1/10KM/H)
    unsigned short direction;// 方向 (0~359,刻度=1  度,正北为0,顺时针)

	gb905_bcd_timestamp_t timestamp;	// 时间 (BCD码 YY-MM-DD-hh-mm-ss)
}__packed location_report_808_t;



#define SENSOR_INFO_ID	0x20
#define SENSOR_DAT_LEN	24
typedef struct{
	unsigned char info_id;//0x20 for face 
	unsigned char info_len;//len=24(0x18)
	unsigned char info_sensor[SENSOR_DAT_LEN];//data of sensor
}__packed sensor_ext_info_t;


typedef  struct
{
	unsigned char start_magic_id;
    gb905_msg_header_t header;
    location_report_t report;
    sensor_ext_info_t sensor_ext;
    unsigned char _xor;
    unsigned char end_magic_id;
} __packed gb905_location_report_sensor_t;

//send report now
void sys_gb905_report(int index);
unsigned int get_report_period(void);
unsigned int get_report1_period(void);
//build report info now
void gb905_build_report(location_report_t * report);
//without update last report info for ack compare
location_report_t get_location_report(void);

#ifdef __cplusplus
}
#endif

#endif