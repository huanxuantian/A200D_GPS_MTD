
#ifndef _SETTING_H
#define	_SETTING_H

#ifdef __cplusplus
extern "C" {
#endif
//utils 
#ifndef __packed
#define __packed    __attribute__((packed))
#endif
#include "utils.h"


#define DEFAULT_GB905_HOST	"183.192.69.158"
#define DEFAULT_GB905_PORT	19002


#define AUTH_HOST   "auth.user" //RESERVE
#define AUTH_PORT   8888

#define SEED_HOST   "1.tcp.cpolar.cn"
#define SEED_PORT 21415

#define			MAX_IMEI_CHARS_SIZE		16

typedef struct {
	unsigned int	heartbeat_interval;			// 终端心跳发送间隔，秒
	unsigned int	tcp_msg_ack_timeout;		// TCP 消息应答超时时间，秒
	unsigned int	tcp_msg_retry_count;		// TCP 消息重传次数
	

	unsigned char 	main_server_ipaddr[32];		// 主服务器地址,IP 或域名
	unsigned char 	vice_server_ipaddr[32];		// 备份服务器地址,IP 或域名
	unsigned int	main_server_tcp_port;		// 主服务器TCP 端口
	unsigned int	vice_server_tcp_port;		// 备份服务器TCP 端口

	unsigned int	location_report_policy;		// 位置汇报策略 : =0,定时汇报; =1,定距汇报; =2,定时 + 定距
	unsigned int	location_report_scheme;		// 位置汇报方案 : =0,根据ACC状态; =1,根据空重车状态;=2,根据登录状态 + ACC 状态;=3,根据登录状态 + 空重车状态

	unsigned int	time_report_accoff_interval;		// ACC OFF 汇报时间间隔，秒，>0
	unsigned int	time_report_accon_interval;			// ACC ON 汇报时间间隔，秒，>0
	
	unsigned int	max_speed;							// 最高速度，KM/H
	unsigned int	over_speed_duration;				// 超速持续时间，秒
	unsigned char	meter_operation_count_limit;		// 计价器营运次数限制: 0-99;=0,表示不做限制
	unsigned char	meter_operation_count_time[5];		// 计价器营运时间限制: YYYY-MM-DD-hh;=0000000000,表示不做限制
	
	unsigned short	time_idle_after_accoff;				// ACC OFF 后进入休眠模式的时间: 单位S
	//认证信息的IMEI，ICCID保留，MTDID为认证ID，版本号保留
    unsigned char imei[20];
    unsigned char iccid[32];
    unsigned char mtdid[12];
    unsigned char version[12];
	unsigned char io_data[8];

	unsigned short crc;
}__packed params_setting_mini_t;

typedef params_setting_mini_t params_setting_t;

typedef struct{
	unsigned int mtd_id;
	char imei_id[MAX_IMEI_CHARS_SIZE];
	unsigned short crc;
}__packed device_info_t,*p_device_info_t;


typedef struct{
    unsigned short volumetric_moisture;//体积含水量0.1%
    unsigned short temperature;//温度0.1摄氏度
    unsigned short ec;//电导率 us/cm
    unsigned short ph;//PH值（0.1）
    unsigned short nitro;//氮含量（mg/kg）
    unsigned short phosphorus;//磷含量（mg/kg）
    unsigned short potassium;//钾含量（mg/kg）
    unsigned short salinity;//盐分（mg/kg）
}__packed soil_sensor_data_t;

typedef struct{
    unsigned short temperature;//温度0.1摄氏度
    unsigned short humidity;//湿度0.1%
}__packed thm_sensor_data_t;
typedef struct{
    unsigned int lux;//0.001lux
}__packed lux_sensor_data_t;

typedef struct{
    thm_sensor_data_t thm_data;
    lux_sensor_data_t lux_data;
    soil_sensor_data_t soil_data;
}__packed meter_sensor_data_t;

typedef struct {
	unsigned int	update_policy;//升级策略 0，禁止升级，1，只能手动升级，2,只能自动检测升级，
						//3,手动或自动，自动优先，当手动时会马上自动检测版本，自动跟手动不一致时以自动为准，自动无配置则执行手动之后上报自动信息
	unsigned int	update_scheme;//升级限制开关，0;不开启，
						//1，只开启自动升级的指定域名ip升级限制，2，只开启手动模式的指定域名ip升级限制，
						//3,开启手动自动模式的指定域名ip升级限制
	unsigned int	link_policy;//0第二网关长连汇报数据，1第二网关短连，检测时间由参数指定，并且会汇报数据，
						//2第二网关短连，不汇报数据，只根据参数检测链接
						//3,第二网关短连，指定时间段链接，不汇报数据
	unsigned int	heartbeat_interval;			// 终端心跳发送间隔，秒,仅在长连时有效
	unsigned int	link_check_interval;		//重连/断连检测间隔，在长连时超过该时间才会检查重连，在短连模式，超过该时间无数剧断连
	unsigned int	link_continue_interval;		//短连检测时间，只在短连模式下有效，根据该时间间隔去长连，时间必须大于3分钟防止频繁链接
	
	unsigned int	time_report_accoff_interval;		// ACC OFF 汇报时间间隔，秒，>0仅限于第二网关开启
	unsigned int	time_report_accon_interval;			// ACC ON 汇报时间间隔，秒，>0仅限于第二网关

	unsigned char update_host0_spec[32];//当scheme为1或者3时用于自动升级的ip限制，只能从该指定ip域名获得升级文件（包含升级引用的引导文件等）
	unsigned char update_host1_spec[32];//当scheme为2或者3时用于手动升级的ip限制，只能从该指定ip域名获得升级文件（包含升级引用的引导文件等）
	unsigned short crc;
}__packed function_control_param_t;

unsigned short crc16_compute(const unsigned char * p_data, unsigned int size, const unsigned short * p_crc);

function_control_param_t* get_func_param(void);
params_setting_t *  get_setting_params(void);
void set_setting_params(void);
void init_setting_params(void);

void get_device_info(device_info_t * info);
void clr_device_info();
void set_device_info(device_info_t * info);
void get_hw_version(char * buff);
int get_ouput_state(unsigned char chn);
int set_output_state(unsigned char chn,unsigned char flag);

void update_soil_data(soil_sensor_data_t data);
void update_thm_data(thm_sensor_data_t data);
void update_lux_data(lux_sensor_data_t data);
void build_sensor_data(unsigned char* buff,int buff_len);

int setting_init();

#ifdef __cplusplus
}
#endif

#endif
