#define DBG_LEVEL           DBG_INFO
#define DBG_SECTION_NAME    "STATUS"
#include <rtdbg.h>

#include "status.h"
#include "gsm.h"

#include "s2j.h"

static uint8_t acc_state=0;

void update_acc_state(uint8_t state){
	if(acc_state!=state){
		acc_state = state;
		save_satte_file();
	}
}

uint8_t get_acc_state()
{
    return acc_state;
}

static int loading_state=0;

void update_loading_state(uint8_t state){
	if(loading_state!=state){
		loading_state = state;
		LOG_E("loading state change %d",state);
		report_trige(1);
		save_satte_file();
	}
}

uint8_t get_loading_state()
{
	return loading_state;
}
//计价器锁定应答信息缓存
meter_lock_seq_info_t meter_lock_seq_info;

unsigned short get_meter_soft_lock_seq(){
	if(meter_lock_seq_info.soft_flag){
		return meter_lock_seq_info.soft_lock_seq;
	}
	return 0;
}
unsigned short get_meter_soft_lock_subtype(){
	if(meter_lock_seq_info.soft_flag){
		return meter_lock_seq_info.soft_lock_subtype;
	}
	return 0;
}

void set_meter_soft_lock_seq(unsigned short subtype,unsigned short seq){
		meter_lock_seq_info.soft_flag=1;
		meter_lock_seq_info.soft_lock_subtype=subtype;
		meter_lock_seq_info.soft_lock_seq=seq;
		//save_meter_lock_seq();
		save_satte_file();
}

void clear_meter_soft_lock_seq()
{
	if(meter_lock_seq_info.soft_flag!=0)
	{
		meter_lock_seq_info.soft_flag=0;
		meter_lock_seq_info.soft_lock_seq=0;
		//save_meter_lock_seq();
		save_satte_file();
	}
}
//计价器锁定的信息缓存
static meter_limit_lock_info_t meter_limit_lock_info;

void set_meter_limit_lock_info(meter_limit_lock_info_t* limit_lock_info)
{
	memcpy((void*)&meter_limit_lock_info,(void*)limit_lock_info,sizeof(meter_limit_lock_info_t));
	//task 20171130::save info here 
	//save_meter_limit_lock_info(&meter_limit_lock_info);
}

meter_limit_lock_info_t* get_meter_limit_lock_info()
{
	return &meter_limit_lock_info;
}
//计价器锁定标志（标明是否需要等待计价器状态应答以及对应锁定状态）
static unsigned int meter_lock_cmd_status;

void set_meter_soft_new_lock_status(unsigned char lock_status,unsigned char lock_subtype)
{
	unsigned char new_lock_type;

	if(lock_subtype>=8) return ;
	new_lock_type= METER_LOCK_TYPE_NEW_S_SOFT+lock_subtype;
	
	if(lock_status!=0){
		meter_lock_cmd_status |= (1<<(new_lock_type));
	}else{
		meter_lock_cmd_status &= ~(1<<(new_lock_type));
	}
	DbgWarn("new soft lock new state:%d,subtype:%d;lock status:0x%08x\r\n",
		lock_status,lock_subtype,meter_lock_cmd_status);
    save_satte_file();
}

void clr_meter_soft_new_lock_status()
{
	meter_lock_cmd_status &= ~(0xFF<<(METER_LOCK_TYPE_NEW_S_SOFT));
	DbgWarn("new soft lock clear;lock status:0x%08x\r\n",
		meter_lock_cmd_status);
    save_satte_file();
}

void set_meter_soft_new_lock_flag(unsigned char lock_status,unsigned char lock_subtype)
{
	unsigned char save_flag_type;
	if(lock_subtype>=8) return ;
	save_flag_type = METER_LOCK_TYPE_NEW_FLAG_S_SOFT+lock_subtype;

	if(lock_status!=0){
		meter_lock_cmd_status |= (1<<(save_flag_type));
	}else{
		meter_lock_cmd_status &= ~(1<<(save_flag_type));
	}
	DbgWarn("new soft lock new state:%d,subtype:%d;lock status:0x%08x\r\n",
		lock_status,lock_subtype,meter_lock_cmd_status);
	save_satte_file();
    
}
void clr_meter_soft_new_lock_flag()
{
	meter_lock_cmd_status &= ~(0xFF<<(METER_LOCK_TYPE_NEW_FLAG_S_SOFT));
	DbgWarn("new soft lock flag clear;lock status:0x%08x\r\n",
		meter_lock_cmd_status);
	save_satte_file();
}

unsigned char get_meter_soft_new_lock_status_subtype(unsigned char lock_subtype){
	unsigned char new_lock_type;
	unsigned int flag;
	if(lock_subtype>=8) return 0;
	new_lock_type= METER_LOCK_TYPE_NEW_S_SOFT+lock_subtype;
	flag = meter_lock_cmd_status&(1<<(new_lock_type));
	if(flag!=0)
	{
		return 1;
	}
	return 0;
}

unsigned char get_meter_soft_new_lock_status()
{
	unsigned int flag;
	flag = (meter_lock_cmd_status>>METER_LOCK_TYPE_NEW_S_SOFT)&0xFF;
	//8-15 lock bit
	if(flag!=0)
	{
		return 1;
	}
	return 0;
}

unsigned char get_meter_soft_new_lock_flag()
{
	unsigned int flag;
	flag = (meter_lock_cmd_status>>METER_LOCK_TYPE_NEW_FLAG_S_SOFT)&0xFF;
	//16-24 lock flag bit
	if(flag!=0)
	{
		return flag;
	}
	return 0;
}


void clear_all_meter_lock_status()
{
	if(meter_lock_cmd_status!=0)
	{
		meter_lock_cmd_status=0;
		save_satte_file();
		{
			meter_limit_lock_info_t meter_lock_ctl;
			meter_lock_ctl.lock_flag = 0;
			set_meter_limit_lock_info(&meter_lock_ctl);
		}
	}
}

void set_test_lock()
{
	if(get_meter_soft_new_lock_status()==0)
	{
		set_meter_soft_new_lock_status(1,7);
		set_meter_soft_lock_seq(7,15);
		set_meter_soft_new_lock_flag(1,7);
		save_satte_file();
		if(get_meter_soft_new_lock_status()!=0)
		{
			meter_limit_lock_info_t meter_lock_ctl;
			meter_lock_ctl.lock_flag = 1;
			set_meter_limit_lock_info(&meter_lock_ctl);
		}
	}

}

cJSON* s2j_state()
{
	//创建主对象
	s2j_create_json_obj(state_json);

	s2j_json_set_basic_element(state_json,&meter_lock_seq_info,int,soft_flag);
	s2j_json_set_basic_element(state_json,&meter_lock_seq_info,int,soft_lock_seq);
	s2j_json_set_basic_element(state_json,&meter_lock_seq_info,int,soft_lock_subtype);

	s2j_create_json_obj(substate_json);
	//加入结构外的对象数据
	cJSON_AddItemToObject(state_json,"substate",substate_json);
	cJSON_AddNumberToObject(substate_json,"lock_state_flag",meter_lock_cmd_status);
	cJSON_AddNumberToObject(substate_json,"loading_state",loading_state);
	cJSON_AddNumberToObject(substate_json,"acc_state",acc_state);
	
	return state_json;
}

char* build_state_json()
{
	cJSON* data = s2j_state();
	char* text = cJSON_PrintBuffered(data,256,0);

	cJSON_Delete(data);

	return text;
}

void parse_state_json(char* string)
{
	unsigned int substate_flag=0;
	cJSON* data = cJSON_Parse(string);
	if(data!=NULL){
		s2j_create_struct_obj(seq_info,meter_lock_seq_info_t);

		s2j_struct_get_basic_element(seq_info,data,int,soft_flag);
		s2j_struct_get_basic_element(seq_info,data,int,soft_lock_seq);
		s2j_struct_get_basic_element(seq_info,data,int,soft_lock_subtype);

		LOG_W("parse:soft_flag=%d",seq_info->soft_flag);
		LOG_W("parse:soft_lock_seq=%d",seq_info->soft_lock_seq);
		LOG_W("parse:soft_lock_subtype=%d",seq_info->soft_lock_subtype);

		meter_lock_seq_info = *seq_info;

		cJSON* substate = cJSON_GetObjectItem(data,"substate");
		if(substate){
			//substate_flag = 
			json_temp = cJSON_GetObjectItem(substate, "lock_state_flag");
			if(json_temp){
				substate_flag = (unsigned int )json_temp->valueint;
				LOG_W("parse:substate.lock_state_flag=%d",substate_flag);
				meter_lock_cmd_status = (unsigned int)(substate_flag&0xFFFFFFFF);
				if(get_meter_soft_new_lock_status()!=0)
				{
					meter_limit_lock_info_t meter_lock_ctl;
					meter_lock_ctl.lock_flag = 1;
					set_meter_limit_lock_info(&meter_lock_ctl);
				}
			}
			json_temp = cJSON_GetObjectItem(substate, "loading_state");
			if(json_temp){
				substate_flag = (unsigned int )json_temp->valueint;
				LOG_W("parse:substate.loading_state=%d",substate_flag);
				loading_state = substate_flag&0xFF;
			}
			json_temp = cJSON_GetObjectItem(substate, "acc_state");
			if(json_temp){
				substate_flag = (unsigned int )json_temp->valueint;
				LOG_W("parse:substate.acc_state=%d",substate_flag);
				acc_state = substate_flag&0xFF;
			}
		}
		cJSON_Delete(data);
		rt_free(seq_info);

	}

}