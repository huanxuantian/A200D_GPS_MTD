#ifndef _GB905_REMOTE_H
#define	_GB905_REMOTE_H

#ifdef __cplusplus
extern "C" {
#endif
#include "utils.h"

#define	TAXI_CONTROL_OIL					0x00
#define	TAXI_CONTROL_CIRCUIT				0x01
#define	TAXI_CONTROL_DOOR_LOCK				0x02
#define	TAXI_CONTROL_CAR_LOCK				0x03
#define	TAXI_CONTROL_METER_LOCK				0x04


typedef struct{
	unsigned char channel;
	unsigned char cmd;
}__packed msg_remote_t;


typedef struct {
	unsigned short ack_serial_number;
	location_report_t report;
}__packed remote_contrl_ack_t;

typedef  struct
{
    unsigned char start_magic_id;
    gb905_msg_header_t header;
    remote_contrl_ack_t contrl_ack;
    unsigned char xor;
    unsigned char end_magic_id;
} __packed gb905_remote_contrl_ack_t;

typedef struct
{
	unsigned char lock_flag;
	unsigned char lock_time[6];//YYMMDDhhmmss
	unsigned short lock_subtype;//
	unsigned char tips[1];
}__packed gb905_fleety_new_meter_lock_info;
typedef  struct
{
    unsigned char start_magic_id;
    gb905_msg_header_t header;
    unsigned char result;
	unsigned short ack_seq;
	unsigned short lock_subtype;//
    unsigned char xor;
    unsigned char end_magic_id;
} __packed gb905_fleety_new_meter_lock_ack_t;

#define METER_SOFT_LOCK_ACK_OK      0
#define METER_SOFT_LOCK_ACK_FAIL    1
#define METER_SOFT_UNLOCK_ACK_OK    2
#define METER_SOFT_UNLOCK_ACK_FAIL  3

unsigned char gb905_remote_control_treat(unsigned char index,unsigned short seq,unsigned char *buf,int len);
void gb905_remote_control_ack(unsigned char index,unsigned short seq_number);

unsigned char fleety_remote_new_meter_lock_treat(unsigned short seq,unsigned char *buf,int len);
void fleety_remote_new_meter_lock_ack(unsigned char result,unsigned short seq,unsigned short subtype);

#ifdef __cplusplus
}
#endif

#endif