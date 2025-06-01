#ifndef __METER_H_
#define __METER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "utils.h"

#define		METER_TYPE_ID		0x02

#define		METER_VENDOR_ID		0x02

#define		METER_RESULT_OK		0x90
#define		METER_RESULT_FAIL	0xFF


typedef struct {
	unsigned short magic_start;
	unsigned short len;
	unsigned char type;
	unsigned char vendor;
	unsigned short cmd;
}__packed gb905_meter_header_t;

typedef struct {
	unsigned char xor;
	unsigned short magic_end;
}__packed gb905_meter_tail_t;


#define		METER_MAGIC_NUM		0x55AA

#define		METER_COMMAND_OPERATION_OCCUPIED	0x00E7
#define		METER_COMMAND_OPERATION_EMPTY		0x00E8
#define		METER_COMMAND_HEART_BEAT			0x00E9


#define		METER_COMMAND_TEST_LOCK				0xA0F1
#define		METER_COMMAND_TEST_UNLOCK			0xA0F0


typedef struct{
    gb905_meter_header_t header;
    unsigned char result;
    gb905_meter_tail_t tail;
}__packed gb905_meter_common_ack_msg_t;

// 用BCD  码表示当前时间
typedef struct{
	unsigned char bcd[7];				// YYYY-MM-DD-hh-mm-ss
}__packed gb905_meter_bcd_timestamp_t;

//2014 protocol
/*------------------------------------------------------------------------------*/
typedef struct{
	unsigned char licence_plate[6];			// 车牌号
	
	unsigned char company_num_bcd[16];		// 单位代码
	unsigned char driver_num_bcd[19];		// 司机代码
	unsigned char board_time_bcd[5];		// 上车时间(YY-MM-DD-hh-mm)
	unsigned char alighting_time_bcd[2];	// 下车时间(hh-mm)
	unsigned char taximeter_bcd[3];			// 计程公里(xxxxx.x km)
	unsigned char empty_run_bcd[2];			// 空驶公里(xxx.x km)
	unsigned char surcharge_bcd[3];			// 附加费(xxxxx.x 元)
	unsigned char waiting_time_bcd[2];		// 等待计时时间(hh-mm)
	unsigned char transaction_amount_bcd[3];// 交易金额(xxxxx.x 元)
	unsigned int  car_number;				// 当前车次	
	unsigned char business_type;
}__packed gb2014_meter_operation_t;

// 计价器当前状态
typedef union 
{
	unsigned char whole;
	
	struct
	{
		// low --> high
		unsigned char loading:1;	// =0,空车；=1,重车	
		unsigned char login:1;		// =0,签退；=1,登录		
		unsigned char force_on:1;	// =0,正常；=1,强制开机
		unsigned char force_off:1;	// =0,正常；=1,强制关机
		unsigned char reserve:2;
		unsigned char stop:1;		//锁定状态（时间锁定）
		unsigned char :0;
    }__packed flag;
}__packed meter_state_t;

typedef struct{
	meter_state_t meter_state;
	unsigned char company_num_bcd[16];
	unsigned char driver_num_bcd[19];
}__packed gb2014_meter_heart_beat_t;

// 用BCD  码表示时间限制
typedef struct {
	unsigned char bcd[5];				// YYYY-MM-DD-hh
}__packed meter_bcd_time_threhold_t;


typedef struct{
	unsigned short terminal_state;
	meter_bcd_time_threhold_t time_threhold;
	unsigned char number_threhold;
	unsigned short rfu;
}__packed gb905_meter_heart_beat_ack_t;

typedef struct{
    gb905_meter_header_t header;
    gb905_meter_heart_beat_ack_t ack;
    gb905_meter_tail_t tail;
}__packed gb905_meter_heart_beat_ack_msg_t;

void gb2014_meter_heart_beat_send();

int meter_protocol_anaylze(unsigned char * buf,int len);

void meter_init();
void handle_meter_task();
void stop_meter_task();

#ifdef __cplusplus
}
#endif

#endif /* __METER_H_ */