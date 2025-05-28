#ifndef _GB905_CONTROL_H
#define	_GB905_CONTROL_H

#ifdef __cplusplus
extern "C" {
#endif
#define			UPDATE_SHINKI_APP		0x00

#define			UPDATE_METER_APP		0x04 // only for meter


enum{
	GB905_CONTROL_UPDATE=1,

    GB808_CONTROL_CONNECT_SERVER = 2,
    GB808_CONTROL_POWER_OFF,
    GB808_CONTROL_RESET,
    GB808_CONTROL_RECOVERY,
    GB808_CONTROL_OFF_SOCKET,
	GB808_CONTROL_OFF_COMMUNICATION,    
    
	GB905_CONTROL_POWER_OFF = 2,
	GB905_CONTROL_RESET,
	GB905_CONTROL_RECOVERY,
	GB905_CONTROL_OFF_SOCKET,
	GB905_CONTROL_OFF_COMMUNICATION,
};

enum{
	GB905_UPDATE_UPDATE=0,
	GB905_UPDATE_OK,
	GB905_UPDATE_FAIL,
	GB905_UPDATE_VENDOR_MISMATCH,
	GB905_UPDATE_HW_MISMATCH,
	GB905_UPDATE_FAIL_DOWNLOAD,
	GB905_UPDATE_GIVE_UP,
};
//kunming 
enum{
	UPGRADE_SAME_VERSION,
	UPGRADE_OK,
	UPGRADE_FAILE,
	UPGRADE_ERROR_COMPANY_MARK, //厂商标志不同
	UPGRADE_ERROR_HW_VERSION,		//	硬件版本号不一致
	UPGRADE_DOWNLOAD_FAILE,		//下载升级文件失败
	UPGRADE_PLATE_ACTIVE_CANCEL,	//服务器主动取消升级
	UPGRADE_DEVICE_ACTIVE_CANCLE,	//设备主动取消升级（非自身程序）
	UPGRADE_SERVER_CONNECT_FAILE,	//服务器连接失败
	UPGRADE_SERVER_LOGIN_FAILE,	//服务器登陆失败
	};
//
typedef struct{
	unsigned char device_type;						// 设备类型 =0x00,智能服务终端
	unsigned char vendor_id;						// 厂商编号 =0x02,飞田公司
}__packed update_msg_req_t ,*p_update_msg_req_t;

typedef struct{
	update_msg_req_t base_req;
	unsigned int base_address;
}__packed update_msg_code_req_t ,*p_update_msg_code_req_t;

typedef struct{
	update_msg_req_t base_req;
	unsigned char hw_version;
	unsigned char sw_version[2];
	unsigned int  total_length;
	unsigned int  check_sum;
	unsigned int  download_len;
	unsigned short crc;
}__packed update_msg_info_rep_t ,*p_update_msg_info_rep_t;

typedef  struct
{
    unsigned char	device_type;
    unsigned char	vendor_id;
    unsigned char	hw_version;
    unsigned char	sw_version[2];
} __packed msg_device_info_t;

typedef  struct
{
    unsigned char	control_cmd;

	msg_device_info_t device_info;
	unsigned int content;  
} __packed gb905_msg_control_t;


typedef  struct
{
    unsigned char	control_cmd;
	msg_device_info_t device_info;
	unsigned char 	update_server_apn[32];		// 升级服务器APN，无线通信拨号访问点
	unsigned char 	update_server_username[32];	// 升级服务器无线通信拨号用户名
	unsigned char 	update_server_password[32];	// 升级服务器无线通信拨号密码
	unsigned char 	update_server_ipaddr[32];	// 升级服务器地址,IP 或域名

	unsigned short	update_server_tcp_port;		// 升级服务器TCP 端口
} __packed msg_control_t;


typedef  struct
{
    unsigned char	connect_ctrl;
	unsigned char 	update_server_apn[32];		// 升级服务器APN，无线通信拨号访问点
	unsigned char 	update_server_username[32];	// 升级服务器无线通信拨号用户名
	unsigned char 	update_server_password[32];	// 升级服务器无线通信拨号密码
	unsigned char 	update_server_ipaddr[32];	// 升级服务器地址,IP 或域名
	unsigned short	update_server_tcp_port;		// 升级服务器TCP 端口
	unsigned short	update_server_udp_port;		// 升级服务器UDP 端口
	unsigned char	vendor_id[5];
    unsigned char	authorize[32];
    unsigned char	hw_version[32];
    unsigned char	sw_version[32];
    unsigned char	url_addr[100];
    unsigned int    connect_timeout;
} __packed msg_control_808_t;

typedef  struct
{   
	msg_device_info_t device_info;
 	unsigned char result;
} __packed msg_update_t;

typedef  struct
{   
    unsigned char start_magic_id;
    gb905_msg_header_t header;
    
	msg_update_t update_result;
 	
    unsigned char xor;
    unsigned char end_magic_id;
} __packed gb905_msg_update_t;


typedef  struct
{   
    char host[64];
	int port;
	char file_name[64];
} __packed gb905_full_diak_update_t;

unsigned short get_http_ack_seq();

unsigned char gb905_control_treat(unsigned char index,unsigned short seq,unsigned char *buf,int len);

void send_update_result(unsigned char index,unsigned char result);

unsigned char get_mcu32_update_socket(void);
void set_mcu32_update_socket(unsigned char index);
#ifdef __cplusplus
}
#endif

#endif

