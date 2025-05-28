#include <rtthread.h>
#include "utils.h"
#include <system_bsp.h>

#include <protocol.h>


unsigned char gb905_remote_control_treat(unsigned char index,unsigned short seq,unsigned char *buf,int len)
{
	msg_remote_t * msg_remote;
	
	DbgFuncEntry();

    msg_remote = (msg_remote_t *)buf;

	DbgPrintf("channel = 0x%x\r\n",msg_remote->channel);

	DbgPrintf("cmd = 0x%x\r\n",msg_remote->cmd);
	switch(msg_remote->channel)
    {
        case TAXI_CONTROL_OIL:   //油路控制项 0：恢复车辆油路 1：断开车辆油路
			break;
        case TAXI_CONTROL_CIRCUIT://电路控制项 0：恢复车辆电路 1：断开车辆电路
            if(msg_remote->cmd == 0){
                control_output(CIRCUIT_OUT,OUT_LEVEL_LOW);
            }else{
                control_output(CIRCUIT_OUT,OUT_LEVEL_HIGH);
            }
            gb905_remote_control_ack(index,EndianReverse16(seq));
            break;
        case TAXI_CONTROL_DOOR_LOCK://门开关控制项  0：车门解锁    1：车门加锁
            if(msg_remote->cmd == 0){
                control_output(DOOR_OUT,OUT_LEVEL_LOW);
            }else{
                control_output(DOOR_OUT,OUT_LEVEL_HIGH);
            }
            gb905_remote_control_ack(index,EndianReverse16(seq));
            break;
        case TAXI_CONTROL_CAR_LOCK://车辆加锁控制  0：车辆解除锁定 1：车辆锁定
            break;
        case TAXI_CONTROL_METER_LOCK://锁定或解锁计价器
            break;
		default:
			break;
    }
        
	DbgFuncExit();

	return GB905_RESULT_OK;
}

void gb905_remote_control_ack(unsigned char index,unsigned short seq_number)
{
	gb905_remote_contrl_ack_t gb905_remote_contrl_ack;
		
	DbgFuncEntry();

	gb905_build_header(&gb905_remote_contrl_ack.header,MESSAGE_REMOTE_CTRL_ACK, sizeof(remote_contrl_ack_t));
	gb905_debug_header(&gb905_remote_contrl_ack.header);
	
	gb905_remote_contrl_ack.contrl_ack.ack_serial_number = seq_number;
	gb905_build_report(&gb905_remote_contrl_ack.contrl_ack.report);
	
	gb905_send_data(index,(unsigned char *)&gb905_remote_contrl_ack,sizeof(gb905_remote_contrl_ack_t));
	
	DbgFuncExit();
}

unsigned char fleety_remote_new_meter_lock_treat(unsigned short seq,unsigned char *buf,int len)
{
	gb905_fleety_new_meter_lock_info* lock_info;
	meter_limit_lock_info_t meter_lock_ctl;
	unsigned short lock_subtype=0;
	
	if(len<sizeof(gb905_fleety_new_meter_lock_info))
	{
		return GB905_RESULT_OK;
	}
	lock_info = (gb905_fleety_new_meter_lock_info*)buf;
	lock_subtype=EndianReverse16(lock_info->lock_subtype);
	if(lock_subtype>=8){
		DbgWarn("subtype [%d] not usefull!not control to lock/unlock!\r\n",lock_info->lock_subtype);
		return GB905_RESULT_OK;
	}
	DbgWarn("%s detect,lock_flag=%d,subtype=%d!\r\n",__FUNCTION__,lock_info->lock_flag,lock_subtype);
	memset(&meter_lock_ctl,0,sizeof(meter_lock_ctl));

	if(lock_info->lock_flag==0)
	{
        #if 1
			set_meter_soft_new_lock_status(0,lock_subtype);
			set_meter_soft_lock_seq(lock_subtype,seq);
			set_meter_soft_new_lock_flag(1,lock_subtype);

			if(get_meter_soft_new_lock_status()==0)
			{
				meter_lock_ctl.lock_flag = 0;
				set_meter_limit_lock_info(&meter_lock_ctl);
			}
        #endif
	}
	else
	{
        #if 1
		set_meter_soft_new_lock_status(1,lock_subtype);
		set_meter_soft_lock_seq(lock_subtype,seq);
		set_meter_soft_new_lock_flag(1,lock_subtype);
		//if(get_meter_soft_new_lock_status()!=0)
		{
			meter_lock_ctl.lock_flag = 1;
			set_meter_limit_lock_info(&meter_lock_ctl);
		}
		//detect_lock_logout();
		#endif
	}
	return GB905_RESULT_OK;
}
void fleety_remote_new_meter_lock_ack(unsigned char result,unsigned short seq,unsigned short subtype)
{
	gb905_fleety_new_meter_lock_ack_t ack_msg;
	DbgFuncEntry();	
	memset(&ack_msg,0,sizeof(ack_msg));
	gb905_build_header(&ack_msg.header,MESSGAE_FT_NEW_SOFT_LOCK_ACK, 5);
	ack_msg.result = result;
	ack_msg.ack_seq = EndianReverse16(seq);
	ack_msg.lock_subtype= EndianReverse16(subtype);
	gb905_send_data(1,(unsigned char *)&ack_msg,sizeof(gb905_fleety_new_meter_lock_ack_t));
	DbgFuncExit();
}