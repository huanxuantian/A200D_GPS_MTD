#ifndef __STATUS_H_
#define	__STATUS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "utils.h"

typedef  struct{
	unsigned char soft_flag;
	unsigned short soft_lock_seq;
	unsigned short soft_lock_subtype;
	unsigned short crc;
} __packed meter_lock_seq_info_t;


typedef struct {
	unsigned char bcd[7];				// YYYY-MM-DD-HH 实际有效的只到小时
}__packed limit_lock_timestamp_t;

typedef struct{
	unsigned char lock_flag;
	limit_lock_timestamp_t limit_time;
}__packed meter_limit_lock_info_t;

#define		METER_LOCK_TYPE_NEW_S_SOFT 8//标识新的锁类型开始,822A下发的,支持0-8的8种不同标识的锁
#define		METER_LOCK_TYPE_NEW_E_SOFT 15
#define		METER_LOCK_TYPE_NEW_FLAG_S_SOFT 16//标识是否需要检测计价器锁定状态,支持0-8的8种不同标识的锁
#define		METER_LOCK_TYPE_NEW_FLAG_E_SOFT 23

void update_acc_state(uint8_t state);
uint8_t get_acc_state();

void update_loading_state(uint8_t state);
uint8_t get_loading_state();

unsigned char get_meter_soft_new_lock_status();
unsigned char get_meter_soft_new_lock_flag();
unsigned short get_meter_soft_lock_subtype();
void set_meter_soft_lock_seq(unsigned short subtype,unsigned short seq);
void clear_meter_soft_lock_seq();
unsigned short get_meter_soft_lock_seq();

void set_meter_soft_new_lock_flag(unsigned char lock_status,unsigned char lock_subtype);
unsigned char get_meter_soft_new_lock_status_subtype(unsigned char lock_subtype);
void set_meter_soft_new_lock_status(unsigned char lock_status,unsigned char lock_subtype);
void clr_meter_soft_new_lock_flag();
void clear_all_meter_lock_status();
void set_test_lock();

void set_meter_limit_lock_info(meter_limit_lock_info_t* limit_lock_info);
meter_limit_lock_info_t* get_meter_limit_lock_info();

char* build_state_json();
void parse_state_json(char* string);

#ifdef __cplusplus
}
#endif

#endif
