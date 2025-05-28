#ifndef _GB905_PROTOCOL_H
#define	_GB905_PROTOCOL_H

#ifdef __cplusplus
extern "C" {
#endif
#include "bcd.h"

#define		PROTOCOL_VENDOR_ID	0x02

//msg_id
//up
#define		MESSAGE_GENERAL_UP_ACK				0x0001
#define		MESSAGE_HEART_BEAT					0x0002
#define		MESSAGE_GET_PARAMS_ACK				0x0104
#define		MESSAGE_UPDATE_RESULT				0x0105

#define		MESSAGE_LOCATION_REPORT				0x0200

#define 	MESSAGE_REMOTE_CTRL_ACK				0x0500

//down
#define		MESSAGE_GENERAL_DOWN_ACK			0x8001
#define		MESSAGE_SET_PARAMS					0x8103
#define		MESSAGE_GET_PARAMS					0x8104
#define		MESSAGE_CTRL_TERMINAL				0x8105

#define		MESSAGE_REMOTE_CTRL					0x8500

#define		MESSAGE_OPERATION_REPORT			0x0B05

//2023-12-20 新增锁定消息，替换原有8C27消息功能
#define MESSGAE_FT_NEW_SOFT_LOCK 				0x8c2A
#define MESSGAE_FT_NEW_SOFT_LOCK_ACK 			0x0c28

enum{
	GB905_RESULT_OK = 0,
	GB905_RESULT_FAIL,
	GB905_RESULT_UNKNOWN,
};

/*
* 描述设备编号信息
*/
typedef struct
{
	unsigned char vendor_id;						// 厂商编号 =0x02,飞田公司
	unsigned char product_id;						// 设备类型 =0x00,智能服务终端
	unsigned char mtd_id[3];						// 序列号
}__packed uuid_device_id_t;

typedef struct{
	unsigned short msg_id;							// 消息ID
	unsigned short msg_len;							// 消息体属性 （当终端标识第一字节为’0x10’时，消息体属性目前存储的为消息包的大小）
	unsigned char terminal_id[6];
	unsigned short msg_serial_number;				// 消息体流水号 （按发送顺序从0 开始循环累加）
}__packed gb905_msg_header_t,*p_gb905_msg_header_t;

/*
* 记录GB905  消息缓存
*/
typedef struct{
	unsigned char * raw_msg_buf;					// 转义前的原始数据缓存
	int raw_msg_len;								// 转义前的原始数据长度
	unsigned char * msg_buf;						// 转义后的消息数据缓存
	int msg_len;									// 转义后的消息数据长度
}__packed gb905_msg_t,*p_gb905_msg_t;

typedef struct{
	unsigned short seq;
	unsigned short id;
	unsigned char result;
}__packed msg_general_ack_t;

typedef  struct
{
    unsigned char start_magic_id;
    gb905_msg_header_t header;
    msg_general_ack_t ack;
    unsigned char _xor;
    unsigned char end_magic_id;
} __packed gb905_general_ack_t;

// 用BCD  码表示时间
typedef struct {
	unsigned char bcd[6];				// YY-MM-DD-hh-mm-ss
}__packed gb905_bcd_timestamp_t;

// 用BCD  码表示时间
typedef struct {
	unsigned char bcd[6];				// YYYY-MM-DD-hh-mm
}__packed gb905_bcd_login_time_t;
typedef struct {
	unsigned char bcd[7];				// YYYY-MM-DD-hh-mm-ss
}__packed gb905_bcd_all_time_t;
typedef union 
{
	unsigned int  id;
	
	struct
	{
		// low --> high
		unsigned sec:6;			// 秒 （0～59）
		unsigned min:6;			// 分 （0～59）
		unsigned hour:5;		// 时 （0～23）
		unsigned mday:5;		// 天 （1～31）
		unsigned mon:4;			// 月 （1～12）
		unsigned year:6;		// 年 （2000～2064）[new 2010~2074]
    }__packed timestamp;
}__packed gb905_timestamp_id_t;

unsigned int  build_mtdid(unsigned char* buff,int buff_size);

int gb905_protocol_anaylze(unsigned char index,unsigned char * buf,int len);

int gb905_send_data(unsigned char socket_index,unsigned char * buf, int len);
void gb905_send_ack(unsigned char socket_index,gb905_msg_header_t * header,unsigned char result);

void gb905_build_header(gb905_msg_header_t * header, unsigned short msg_id, unsigned short msg_len);
void gb905_build_timestamp(gb905_bcd_timestamp_t * timestamp);

unsigned int gb905_build_new_timestamp_id(void);

void gb905_debug_header(gb905_msg_header_t * header);
void gb905_debug_ack(unsigned char index,msg_general_ack_t * ack);

unsigned int get_heat_beat_period();
unsigned int get_heat_beat1_period(void);
void send_heart_beat(int index);

#ifdef __cplusplus
}
#endif

#endif
