#include <rtthread.h>

#include "utils.h"
#include <system_bsp.h>
#include <business.h>
#include <protocol.h>

#define		PARAMS_ID_INTERVAL_HEARTBEAT			0x0001
#define		PARAMS_ID_TCP_MSG_ACK_TIMEOUT			0x0002
#define		PARAMS_ID_TCP_MSG_RETRY_COUNT			0x0003

#define		PARAMS_ID_MAIN_SERVER_IPADDR			0x0013

#define		PARAMS_ID_VICE_SERVER_IPADDR			0x0017
#define		PARAMS_ID_MAIN_SERVER_TCP_PORT			0x0018
#define		PARAMS_ID_VICE_SERVER_TCP_PORT			0x0019

#define		PARAMS_ID_LOCATION_REPORT_POLICY		0x0020
#define		PARAMS_ID_LOCATION_REPORT_SCHEME		0x0021

#define		PARAMS_ID_TIME_REPORT_ACCOFF_INTERVAL	0x0023
#define		PARAMS_ID_TIME_REPORT_ACCON_INTERVAL	0x0024

#define		PARAMS_ID_MAX_SPEED						0x0055
#define		PARAMS_ID_OVER_SPEED_DURATION			0x0056

#define		PARAMS_ID_METER_OPERATION_COUNT_LIMIT	0x0090
#define		PARAMS_ID_METER_OPERATION_COUNT_TIME	0x0091

#define		PARAMS_ID_TIME_IDLE_AFTER_ACCOFF		0x00AF

#define		PARAMS_ID_HARDWARE_VERSION				0x0095
#define		PARAMS_ID_TERMINAL_APP_VERSION			0x009E

#define		PARAM_ID_MTDID							0x3503//mtdid for 

#define		PARAM_ID_SIM_ICCID						0x6036
#define		PARAM_ID_4G_IMEI						0x1005



static unsigned short KNOWN_ID_LIST[]={
    PARAMS_ID_INTERVAL_HEARTBEAT,
    PARAMS_ID_TCP_MSG_ACK_TIMEOUT,
    PARAMS_ID_TCP_MSG_RETRY_COUNT,
    PARAMS_ID_MAIN_SERVER_IPADDR,
    PARAMS_ID_VICE_SERVER_IPADDR,
    PARAMS_ID_MAIN_SERVER_TCP_PORT,
    PARAMS_ID_VICE_SERVER_TCP_PORT,
    PARAMS_ID_LOCATION_REPORT_POLICY,
    PARAMS_ID_LOCATION_REPORT_SCHEME,
    PARAMS_ID_TIME_REPORT_ACCOFF_INTERVAL,
    PARAMS_ID_TIME_REPORT_ACCON_INTERVAL,
    PARAMS_ID_MAX_SPEED,
    PARAMS_ID_OVER_SPEED_DURATION,
    PARAMS_ID_METER_OPERATION_COUNT_LIMIT,
    PARAMS_ID_METER_OPERATION_COUNT_TIME,
    PARAMS_ID_TIME_IDLE_AFTER_ACCOFF,

    PARAMS_ID_HARDWARE_VERSION,
    PARAMS_ID_TERMINAL_APP_VERSION,
    PARAM_ID_MTDID,
    PARAM_ID_SIM_ICCID,
    PARAM_ID_4G_IMEI,

};
static unsigned short AUTOREPORT_ID_LIST[]={
    PARAMS_ID_MAIN_SERVER_IPADDR,
    PARAMS_ID_MAIN_SERVER_TCP_PORT,
    PARAMS_ID_TIME_REPORT_ACCOFF_INTERVAL,
    PARAMS_ID_TIME_REPORT_ACCON_INTERVAL,
    
    PARAMS_ID_HARDWARE_VERSION,
    PARAMS_ID_TERMINAL_APP_VERSION,
	PARAM_ID_MTDID,
    PARAM_ID_SIM_ICCID,
    PARAM_ID_4G_IMEI,

};
#if 0
static unsigned short AUTOREPORT1_ID_LIST[]={


};
#endif
typedef  struct
{
	unsigned char start_magic_id;
    gb905_msg_header_t header;
    unsigned short data[ARRAY_SIZE(AUTOREPORT_ID_LIST)];
    unsigned char _xor;
    unsigned char end_magic_id;
} __packed gb905_param_autoreq_t;
#if 0
typedef  struct
{
	unsigned char start_magic_id;
    gb905_msg_header_t header;
    unsigned short data[ARRAY_SIZE(AUTOREPORT1_ID_LIST)];
    unsigned char _xor;
    unsigned char end_magic_id;
} __packed gb905_param1_autoreq_t;
#endif

void report_keyparam(unsigned char index){
    gb905_param_autoreq_t autoreq;
	unsigned char* req_data=NULL;
	gb905_msg_header_t* header;
	unsigned int msg_len=0;
	unsigned char* msg_data=NULL;
	unsigned int req_len=0;
	int i=0;
	unsigned short* p_param_id;
	msg_len = sizeof(AUTOREPORT_ID_LIST);
	req_len = sizeof(gb905_param_autoreq_t);
	
	req_data = (unsigned char*)&autoreq;
	memset(req_data,0,req_len);
	header = (gb905_msg_header_t*)(req_data+1);
	
	gb905_build_header(header,EndianReverse16(MESSAGE_GET_PARAMS),EndianReverse16(msg_len));
	msg_data = (req_data+1+sizeof(gb905_msg_header_t));
	p_param_id = (unsigned short*)msg_data;
	header->msg_serial_number =0x0000;
	for(i=0;i<ARRAY_SIZE(AUTOREPORT_ID_LIST);i++)
	{
		*p_param_id = EndianReverse16(AUTOREPORT_ID_LIST[i]);
		p_param_id++;
	}
	
	gb905_get_params_treat(index,req_data,req_len);

}
#if 0
void report1_keyparam(unsigned char index){
    gb905_param1_autoreq_t autoreq;
	unsigned char* req_data=NULL;
	gb905_msg_header_t* header;
	unsigned int msg_len=0;
	unsigned char* msg_data=NULL;
	unsigned int req_len=0;
	int i=0;
	unsigned short* p_param_id;
	msg_len = sizeof(AUTOREPORT1_ID_LIST);
	req_len = sizeof(gb905_param1_autoreq_t);
	
	req_data = (unsigned char*)&autoreq;
	memset(req_data,0,req_len);
	header = (gb905_msg_header_t*)(req_data+1);
	
	gb905_build_header(header,EndianReverse16(MESSAGE_GET_PARAMS),EndianReverse16(msg_len));
	msg_data = (req_data+1+sizeof(gb905_msg_header_t));
	p_param_id = (unsigned short*)msg_data;
	header->msg_serial_number =0x0000;
	for(i=0;i<ARRAY_SIZE(AUTOREPORT1_ID_LIST);i++)
	{
		*p_param_id = EndianReverse16(AUTOREPORT1_ID_LIST[i]);
		p_param_id++;
	}
	
	gb905_get_params_treat(index,req_data,req_len);
}
#endif

static int get_pamams_id_len(unsigned char index,unsigned short id){
    int len;
	int c_i=0;
	int known_param =0;
    char temp_str[40]={0};

    params_setting_t * setting_params;

    DbgFuncEntry();

    for(c_i=0;c_i<ARRAY_SIZE(KNOWN_ID_LIST);c_i++)
	{
	//KNOWN_ID_LIST
		if(id ==KNOWN_ID_LIST[c_i])
		{
			known_param=1;
			break;
		}
	}
	if(!known_param)
	{
		return -1;
	}
	setting_params = get_setting_params();

    switch(id)
    {
        case PARAMS_ID_MAIN_SERVER_IPADDR:
			len = strlen((const char *)setting_params->main_server_ipaddr)+1;//+1
			break;
		case PARAMS_ID_VICE_SERVER_IPADDR:
			len = strlen((const char *)setting_params->vice_server_ipaddr)+1;//+1
			break;
        case PARAMS_ID_METER_OPERATION_COUNT_LIMIT:
            len = sizeof(setting_params->meter_operation_count_limit);
            break;
        case PARAMS_ID_METER_OPERATION_COUNT_TIME:
			len = sizeof(setting_params->meter_operation_count_time);
			break;
        case PARAMS_ID_TIME_IDLE_AFTER_ACCOFF:
			len = 2;
			break;
        case PARAMS_ID_HARDWARE_VERSION:
			len = strlen((const char *)HW_VERSION_APP)+1;
			break;
        case PARAMS_ID_TERMINAL_APP_VERSION:
            len =strlen((const char *)SW_VERSION_APP)+1;
            break;
        case PARAM_ID_MTDID:
            memset(temp_str,0,sizeof(temp_str));
			len = build_mtdid((unsigned char*)temp_str,sizeof(temp_str))+1;
			break;
        case PARAM_ID_SIM_ICCID:
            memset(temp_str,0,sizeof(temp_str));
			get_iccid(temp_str,sizeof(temp_str));
			len = strlen((const char *)temp_str)+1;
			break;
		case PARAM_ID_4G_IMEI:
            memset(temp_str,0,sizeof(temp_str));
			get_imei(temp_str,sizeof(temp_str));
			len = strlen((const char *)temp_str)+1;
			break;
        default:
            len = 4;
            break;
    }

	DbgWarn("params(0x%04x) len = %d\r\n",id,len);
	DbgFuncExit();

	return len;
}

static int gb905_build_id(unsigned char index,unsigned short id,unsigned char *buf){

    int len = 0;
    unsigned int temp;
    unsigned short temp1;
    params_setting_t * setting_params;
    char temp_str[40]={0};

    DbgFuncEntry();

    setting_params = get_setting_params();

    DbgPrintf("params id = 0x%04x\r\n",id);
    


    switch(id)
    {
        case PARAMS_ID_INTERVAL_HEARTBEAT:
            temp = EndianReverse32(setting_params->heartbeat_interval);
            len = sizeof(temp);
            memcpy(buf,&temp,len);
            break;
            
        case PARAMS_ID_TCP_MSG_ACK_TIMEOUT:
            temp = EndianReverse32(setting_params->tcp_msg_ack_timeout);
            len = sizeof(temp);
            memcpy(buf,&temp,len);
            break;
        case PARAMS_ID_TCP_MSG_RETRY_COUNT:
            temp = EndianReverse32(setting_params->tcp_msg_retry_count);
            len = sizeof(temp);
            memcpy(buf,&temp,len);
            break;
        case PARAMS_ID_MAIN_SERVER_IPADDR:
			len = strlen((const char *)setting_params->main_server_ipaddr)+1;//+1
		    strcpy((char *)buf,(const char *)setting_params->main_server_ipaddr);
			break;
		case PARAMS_ID_VICE_SERVER_IPADDR:
			len = strlen((const char *)setting_params->vice_server_ipaddr)+1;//+1
            strcpy((char *)buf,(const char *)setting_params->vice_server_ipaddr);
			break;
        case PARAMS_ID_MAIN_SERVER_TCP_PORT:
            temp = EndianReverse32(setting_params->main_server_tcp_port);
            len = sizeof(temp);
            memcpy(buf,&temp,len);
            break;
        case PARAMS_ID_VICE_SERVER_TCP_PORT:
            temp = EndianReverse32(setting_params->vice_server_tcp_port);
            len = sizeof(temp);
            memcpy(buf,&temp,len);
            break;
        case PARAMS_ID_LOCATION_REPORT_POLICY:
            temp = EndianReverse32(setting_params->location_report_policy);
            len = sizeof(temp);
            memcpy(buf,&temp,len);
            break;
        case PARAMS_ID_LOCATION_REPORT_SCHEME:
            temp = EndianReverse32(setting_params->location_report_scheme);
            len = sizeof(temp);
            memcpy(buf,&temp,len);
            break;
       	case PARAMS_ID_TIME_REPORT_ACCOFF_INTERVAL:
            temp = EndianReverse32(setting_params->time_report_accoff_interval);
            len = sizeof(temp);
            memcpy(buf,&temp,len);
            break;
        case PARAMS_ID_TIME_REPORT_ACCON_INTERVAL:
            temp = EndianReverse32(setting_params->time_report_accon_interval);
            len = sizeof(temp);
            memcpy(buf,&temp,len);
            break;
        case PARAMS_ID_MAX_SPEED:
            temp = EndianReverse32(setting_params->max_speed);
            len = sizeof(temp);
            memcpy(buf,&temp,len);
            break;
        case PARAMS_ID_OVER_SPEED_DURATION:
            temp = EndianReverse32(setting_params->over_speed_duration);
            len = sizeof(temp);
            memcpy(buf,&temp,len);
		    break;
        case PARAMS_ID_METER_OPERATION_COUNT_LIMIT:
            len = sizeof(setting_params->meter_operation_count_limit);
            memcpy(buf,&setting_params->meter_operation_count_limit,len);
            break;
        case PARAMS_ID_METER_OPERATION_COUNT_TIME:
			len = sizeof(setting_params->meter_operation_count_time);
            memcpy(buf,setting_params->meter_operation_count_time,len);
			break;
        case PARAMS_ID_TIME_IDLE_AFTER_ACCOFF:
			temp1 = EndianReverse16(setting_params->time_idle_after_accoff);
            len = sizeof(temp1);
            memcpy(buf,&temp1,len);
			break;
        case PARAMS_ID_HARDWARE_VERSION:
			len = strlen((const char *)HW_VERSION_APP)+1;
            memcpy(buf,HW_VERSION_APP,len);
			break;
        case PARAMS_ID_TERMINAL_APP_VERSION:
            len =strlen((const char *)SW_VERSION_APP)+1;
            memcpy(buf,SW_VERSION_APP,len);
            break;
        case PARAM_ID_MTDID:
            memset(temp_str,0,sizeof(temp_str));
			len = build_mtdid((unsigned char*)temp_str,sizeof(temp_str))+1;
            memcpy(buf,temp_str,len);
			break;
        case PARAM_ID_SIM_ICCID:
            memset(temp_str,0,sizeof(temp_str));
			get_iccid(temp_str,sizeof(temp_str));
			len = strlen((const char *)temp_str)+1;
            memcpy(buf,temp_str,len);
			break;
		case PARAM_ID_4G_IMEI:
            memset(temp_str,0,sizeof(temp_str));
			get_imei(temp_str,sizeof(temp_str));
			len = strlen((const char *)temp_str);
            memcpy(buf,temp_str,len);
			break;
        default:
            DbgWarn("param id is not supported!::id =0x%04x\r\n",id);
            break;
    }
    DbgFuncExit();

    return len;
}

static int gb905_build_params(unsigned char index,unsigned char *buf,int len,unsigned short *id_buf,int id_len,unsigned short msg_serial_number){
	gb905_msg_header_t * header;
	msg_params_t * msg_params;
	unsigned short msg_id;
	unsigned short msg_len;
		
	int offset;
	int size;
	int i;

	offset = 1;
	header = (gb905_msg_header_t *)&buf[offset];

	msg_id = MESSAGE_GET_PARAMS_ACK;
	msg_len = len - 3 - sizeof(gb905_msg_header_t);
	gb905_build_header(header,msg_id,msg_len);
	gb905_debug_header(header);

	offset += sizeof(gb905_msg_header_t);
	DbgPrintf("offset = %d\r\n",offset);

	*(unsigned short *)(&buf[offset]) = EndianReverse16(msg_serial_number);
	offset += 2;

    for(i=0;i<id_len;i++){
        msg_params = (msg_params_t *)&buf[offset];
		offset += sizeof(msg_params_t) - 4;
		if(offset >= len-2)
		{
			offset= offset-(sizeof(msg_params_t)-4);
			DbgError("build params predata error!\r\n");
			break;
		}
        size = get_pamams_id_len(index,id_buf[i]);
		if(size<=0||offset+size>len-2)
		{
			DbgWarn("param(0x%04X) size(%d) error!\r\n",id_buf[i],size);
			offset= offset-(sizeof(msg_params_t)-4);
			DbgPrintf("offset = %d\r\n",offset);
			continue;
		}
        size = gb905_build_id(index,id_buf[i],&buf[offset]);
        DbgGood("param(0x%04X) size(%d) OK!\r\n",id_buf[i],size);
				//DbgHexGood("paramdata:",&buf[offset],size);
        if(size<=0||offset+size>len-2)
		{
			offset= offset-(sizeof(msg_params_t)-4);
			DbgPrintf("offset = %d\r\n",offset);
			continue;
		}
		else
		{
			offset += size;
		}
		
		DbgPrintf("offset = %d\r\n",offset);
		
		msg_params->len = size;
		msg_params->id = EndianReverse16(id_buf[i]);

		if(offset > len-2)
		{
			DbgError("build params break!\r\n");
			break;
		}
    }
	
	DbgFuncExit();

	return offset+2;
}

static unsigned char gb905_params_single_treat(unsigned char index,unsigned short id,msg_params_t * msg_params){
    unsigned int temp;
	unsigned short temp1;
    unsigned char flag =0;
	params_setting_t * setting_params;

    DbgFuncEntry();

	setting_params = get_setting_params();
	DbgPrintf(" param id is %d\r\n",id);

    switch(id)
    {
        case PARAMS_ID_INTERVAL_HEARTBEAT:
        	temp = EndianReverse32(msg_params->content);
			if(temp != setting_params->heartbeat_interval)
			{
				setting_params->heartbeat_interval = temp;
				flag=1;
			}
            break;
		case PARAMS_ID_TCP_MSG_ACK_TIMEOUT:
			temp = EndianReverse32(msg_params->content);
			if(temp != setting_params->tcp_msg_ack_timeout)
			{
				setting_params->tcp_msg_ack_timeout = temp;
				flag=1;
			}
			break;
		case PARAMS_ID_TCP_MSG_RETRY_COUNT:
			temp = EndianReverse32(msg_params->content);
			if(temp != setting_params->tcp_msg_retry_count)
			{
				setting_params->tcp_msg_retry_count = temp;
				flag=1;
			}
			break;
        case PARAMS_ID_MAIN_SERVER_IPADDR:
			if(msg_params->len!=strlen((char *)setting_params->main_server_ipaddr)||
				0 != strncmp((char *)setting_params->main_server_ipaddr,(char *)&msg_params->content,msg_params->len))
			{
				memset(setting_params->main_server_ipaddr,0,sizeof(setting_params->main_server_ipaddr));
				strncpy((char *)setting_params->main_server_ipaddr,(const char *)&msg_params->content,msg_params->len);
				flag=1;
			}
			break;
        case PARAMS_ID_VICE_SERVER_IPADDR:
			if(msg_params->len!=strlen((char *)setting_params->vice_server_ipaddr)||
				0 != strncmp((char *)setting_params->vice_server_ipaddr,(char *)&msg_params->content,msg_params->len))
			{
				memset(setting_params->vice_server_ipaddr,0,sizeof(setting_params->vice_server_ipaddr));
				strncpy((char *)setting_params->vice_server_ipaddr,(const char *)&msg_params->content,msg_params->len);
				flag=1;
			}
			break;
        case PARAMS_ID_MAIN_SERVER_TCP_PORT:
			temp = EndianReverse32(msg_params->content);
			if(temp != setting_params->main_server_tcp_port)
			{
				setting_params->main_server_tcp_port = temp;
                flag=1;
			}
			break;
		case PARAMS_ID_VICE_SERVER_TCP_PORT:
			temp = EndianReverse32(msg_params->content);
			if(temp != setting_params->vice_server_tcp_port)
			{
				setting_params->vice_server_tcp_port = temp;
				flag=1;
			}
			break;
		case PARAMS_ID_LOCATION_REPORT_POLICY:
			temp = EndianReverse32(msg_params->content);
			if(temp != setting_params->location_report_policy)
			{
				setting_params->location_report_policy = temp;
				flag=1;
			}
			break;
		case PARAMS_ID_LOCATION_REPORT_SCHEME:
			temp = EndianReverse32(msg_params->content);
			if(temp != setting_params->location_report_scheme)
			{
				setting_params->location_report_scheme = temp;
				flag=1;
			}
			break;
		case PARAMS_ID_TIME_REPORT_ACCOFF_INTERVAL:
			temp = EndianReverse32(msg_params->content);
			if(temp != setting_params->time_report_accoff_interval)
			{
				setting_params->time_report_accoff_interval = temp;
				flag=1;
			}
			break;
		case PARAMS_ID_TIME_REPORT_ACCON_INTERVAL:
			temp = EndianReverse32(msg_params->content);
			if(temp != setting_params->time_report_accon_interval)
			{
				setting_params->time_report_accon_interval = temp;
				flag=1;
			}
			break;
		case PARAMS_ID_MAX_SPEED:
			temp = EndianReverse32(msg_params->content);
			if(temp != setting_params->max_speed)
			{
				setting_params->max_speed = temp;
				flag=1;
			}
			break;

		case PARAMS_ID_OVER_SPEED_DURATION:
			temp = EndianReverse32(msg_params->content);
			if(temp != setting_params->over_speed_duration)
			{
				setting_params->over_speed_duration = temp;
				flag=1;
			}
			break;
		case PARAMS_ID_METER_OPERATION_COUNT_LIMIT:
			if(0 != memcmp((void *)&setting_params->meter_operation_count_limit,(const void *)&msg_params->content,sizeof(setting_params->meter_operation_count_limit)))
			{
				memcpy((void *)&setting_params->meter_operation_count_limit,(const void *)&msg_params->content,sizeof(setting_params->meter_operation_count_limit));
				flag=1;
			}
			break;

		case PARAMS_ID_METER_OPERATION_COUNT_TIME:
			if(0 != memcmp((void *)setting_params->meter_operation_count_time,(const void *)&msg_params->content,sizeof(setting_params->meter_operation_count_time)))
			{
				memcpy((void *)setting_params->meter_operation_count_time,(const void *)&msg_params->content,sizeof(setting_params->meter_operation_count_time));
				flag=1;
			}
			break;
		case PARAMS_ID_TIME_IDLE_AFTER_ACCOFF:
			memcpy(&temp1,(unsigned char*)&msg_params->content,sizeof(temp1));			
			temp1 = EndianReverse16(temp1);
			if(temp1 != setting_params->time_idle_after_accoff)
			{
				setting_params->time_idle_after_accoff = temp1;
				flag=1;
			}
			break;
        default:
			DbgWarn("msg id is not supported!id = %d\r\n",id);
			break;
    }
    DbgFuncExit();
    return flag;
}
static unsigned char temp_buff[160]={0};
void gb905_get_params_treat(unsigned char index,unsigned char *buf,int len){

    int i;
	
	unsigned char * ack_buf;
	int ack_len = 0;

	unsigned short *id_list;
	int id_len;
	int vaild_count=0;
	int cur_param_len=0;

	gb905_msg_header_t * header;
	unsigned short sequence;

	DbgFuncEntry();

	header = (gb905_msg_header_t *)&buf[1];
	id_list = (unsigned short *)&buf[1 + sizeof(gb905_msg_header_t)];
	id_len = header->msg_len / 2;
	sequence = header->msg_serial_number;
	
	DbgPrintf("id_len = %d\r\n",id_len);
	DbgPrintf("sequence = %d\r\n",sequence);

	ack_len = sizeof(gb905_msg_header_t) + 5;	
	for(i=0;i<id_len;i++)
	{
		id_list[i] = EndianReverse16(id_list[i]);
		cur_param_len = get_pamams_id_len(index,id_list[i]);
        if(cur_param_len<=0)
		{
			continue;
		}
		else
		{
			vaild_count++;
			if(ack_len +(cur_param_len + sizeof(msg_params_t) - 4) >= sizeof(temp_buff)){
				id_len = i;
				break;
			}
			ack_len += cur_param_len + sizeof(msg_params_t) - 4;
			DbgGood("param(0x%04X) len=%d\r\n",id_list[i],cur_param_len);
		}
	}
	if(vaild_count==0)
	{
		DbgWarn("no known param for ack!\r\n");
		return;
	}
    
	
	DbgPrintf("ack_len = %d\r\n",ack_len);
    if(ack_len >= sizeof(temp_buff)){
        DbgWarn("param ack data ize overflow!\r\n");
        return;
    }
	
	ack_buf = (unsigned char *)&temp_buff[0];
	memset((void*)ack_buf, 0, ack_len);

	gb905_build_params(index,ack_buf,ack_len,id_list,id_len,sequence);

	gb905_send_data(index,ack_buf,ack_len);
	
	DbgFuncExit();

}
unsigned char gb905_set_params_treat(unsigned char index,unsigned char *buf,int len){

    unsigned short id;
    unsigned char save_flag=0;
	int offset = 0, lenght;
	msg_params_t * msg_params;
    unsigned char total_num;

    DbgFuncEntry();
	if(len<sizeof(msg_params_t)-4)
    {
        return GB905_RESULT_OK;//ack only
    }
	DbgPrintf("offset = %d\r\n",offset);
	DbgPrintf("len = %d\r\n",len);

    do{
		msg_params = (msg_params_t *)&buf[offset];
		id = EndianReverse16(msg_params->id);

		DbgPrintf("id = 0x%4x\r\n",id);
		DbgPrintf("params_len = 0x%2x\r\n",msg_params->len);

		if(gb905_params_single_treat(index,id,msg_params)>0){
            save_flag=1;
        }
	
        if(len < (sizeof(msg_params_t) - 4 + msg_params->len)){break;}
		offset += msg_params->len + sizeof(msg_params_t) - 4;
		len -= msg_params->len + sizeof(msg_params_t) - 4;

		DbgPrintf("offset = %d\r\n",offset);
		DbgPrintf("len = %d\r\n",len);
	}while(len > 0);
    
    if(save_flag){
        DbgWarn("update param save param!!!\r\n");
        set_setting_params();
    }
    
    DbgFuncExit();

	return GB905_RESULT_OK;
}