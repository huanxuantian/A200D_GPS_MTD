#ifndef _GB905_OPERATE_H
#define	_GB905_OPERATE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "business.h"

typedef struct {
	location_report_t	begin_report;
	location_report_t	end_report;
	gb905_timestamp_id_t operation_id;
	gb905_timestamp_id_t eval_id;
	unsigned char eval_state;
	unsigned short eval_ext;
    unsigned int order_id;
}__packed operate_t;


typedef  struct
{
	unsigned char start_magic_id;
    gb905_msg_header_t header;
	operate_t pre_operate;
    gb2014_meter_operation_t operate;
    unsigned char xor;
    unsigned char end_magic_id;
} __packed gb905_operate_report_t;


void gb905_build_pre_operate_start();
void gb905_send_operate(gb2014_meter_operation_t* operate);
void gb905_operate_check(unsigned char index,int clrflag);



#ifdef __cplusplus
}
#endif

#endif