#define		DEBUG_Y

#include <rtthread.h>

#include "utils.h"
#include <protocol.h>
#include "board.h"

#include <stdbool.h>

#include <system_bsp.h>

static unsigned char mcu32_update_socket=0xFF;	//default 0xFF
unsigned char get_mcu32_update_socket(void)
{
	return mcu32_update_socket;
}
void set_mcu32_update_socket(unsigned char index)
{
	mcu32_update_socket=index;
}

//--------------------------------------------------------
static void gb905_debug_control(msg_control_t * control)
{
	DbgFuncEntry();

	DbgPrintf("cmd = %d\r\n",control->control_cmd);
	DbgPrintf("device_type = %d\r\n",control->device_info.device_type);
	DbgPrintf("vendor_id = 0x%x\r\n",control->device_info.vendor_id);
	DbgPrintf("hw_version = %d\r\n",control->device_info.hw_version);
	DbgPrintf("sw_version[0] = 0x%x\r\n",control->device_info.sw_version[0]);
	DbgPrintf("sw_version[1] = 0x%x\r\n",control->device_info.sw_version[1]);

	DbgPrintf("update_server_apn = %s\r\n",control->update_server_apn);
	DbgPrintf("update_server_username = %s\r\n",control->update_server_username);
	DbgPrintf("update_server_password = %s\r\n",control->update_server_password);
	DbgPrintf("update_server_ipaddr = %s\r\n",control->update_server_ipaddr);

	DbgPrintf("update_server_tcp_port = %d\r\n",control->update_server_tcp_port);

	DbgFuncExit();
}



static void gb905_build_update_result(msg_update_t * update_result,unsigned char result)
{
	update_msg_info_rep_t *update_info;
	DbgFuncEntry();
	#if 0
	update_info = get_update_info();

	update_result->device_info.device_type = update_info->base_req.device_type;
	update_result->device_info.vendor_id = update_info->base_req.vendor_id;
	update_result->device_info.hw_version = update_info->hw_version;
	update_result->device_info.sw_version[0] = update_info->sw_version[0];
	update_result->device_info.sw_version[1] = update_info->sw_version[1];

	update_result->result = result;
	#endif
	DbgFuncExit();
}


static unsigned short http_ack_seq=0;

unsigned short get_http_ack_seq()
{
	return http_ack_seq;
}

void gb905_update_treat(unsigned char index,gb905_msg_control_t * msg_control)
{
	msg_control_t control_info;
	int apn_len,username_len,password_len,ipaddr_len;
	unsigned char *apn,*username,*password,*ipaddr;
	unsigned short *port;
	gb905_full_diak_update_t full_disk_info;
	DbgFuncEntry();
	
	memset(&control_info,0x00,sizeof(msg_control_t));

	
	control_info.control_cmd = msg_control->control_cmd;
	control_info.device_info.device_type = msg_control->device_info.device_type;
	control_info.device_info.vendor_id= msg_control->device_info.vendor_id;
	control_info.device_info.hw_version= msg_control->device_info.hw_version;
	control_info.device_info.sw_version[0]= msg_control->device_info.sw_version[0];
	control_info.device_info.sw_version[1]= msg_control->device_info.sw_version[1];
	
	/*update  server  apn*/
	apn = (unsigned char *)&msg_control->content;
	apn_len = strlen((const char *)apn);
	memcpy(control_info.update_server_apn,apn,apn_len+1);
	
	/*update  server  username*/
	username = apn + apn_len +1;						//skip apn_len and '\0'
	username_len = strlen((const char *)username);
	memcpy(control_info.update_server_username,username,username_len+1);
	
	/*update  server  password*/
	password = username + username_len +1;				//skip username_len and '\0'
	password_len = strlen((const char *)password);
	memcpy(control_info.update_server_password,password,password_len+1);
	
	/*update  server  ipaddr*/
	ipaddr = password + password_len +1;				//skip password_len and '\0'
	ipaddr_len = strlen((const char *)ipaddr);
	if(ipaddr_len<=sizeof(control_info.update_server_ipaddr))
	{
		memcpy(control_info.update_server_ipaddr,ipaddr,ipaddr_len+1);
		/*update  server  udp  port*/
		port = (unsigned short*)(ipaddr + ipaddr_len +1);	//skip ipaddr_len and '\0'
		control_info.update_server_tcp_port = EndianReverse16(*port);
	}

	
	DbgError("device_type =0x%02x \r\n",control_info.device_info.device_type);
	gb905_debug_control(&control_info);
	//save_sw_version_info(msg_control->device_info.sw_version);

	//start with http:
	if(strstr((char*)ipaddr,"http:")==(char*)ipaddr)
	{
	DbgError("control_info.device_info.device_type QQfff =0x%02x \r\n",control_info.device_info.device_type);
		//demo_download_test();
		switch(control_info.device_info.device_type)
		{
			case UPDATE_SHINKI_APP:
				new_add_http_update_task(index,(char*)ipaddr,1024,UPDATE_SHINKI_APP_PATH,30,600,control_info.device_info.device_type);
				//parse_url(ipaddr,full_disk_info.host,&full_disk_info.port,full_disk_info.file_name);
				
				DbgPrintf("full disk update file path::%s\r\n",full_disk_info.file_name);
				//new_add_http_update_task(index,ipaddr,16*1024,full_disk_update_path,30,24*3600,control_info.device_info.device_type);
				break;
			case UPDATE_METER_APP:
				//SAVE_UPDATE_FILE
				//save_meter_sw_version_info(msg_control->device_info.sw_version);
				//new_add_http_update_task(index,ipaddr,16*1024,SAVE_UPDATE_FILE,30,24*3600,control_info.device_info.device_type);
				break;
			default:
				break;
		}
		
	}
	else
	{
		if(control_info.update_server_tcp_port>0)
		{
			
		}
		else
		{
			DbgError("udp info ip/port error!!! \r\n");
		}
	}
	DbgFuncExit();
}


unsigned char gb905_control_treat(unsigned char index,unsigned short seq,unsigned char *buf,int len)
{
	gb905_msg_control_t * msg_control;
	
	DbgFuncEntry();
    
    if(len==0)
    {
        return GB905_RESULT_OK;//ÉîÛÚ20Ïî²âÊÔ
    }
    	
	msg_control = (gb905_msg_control_t *)buf;
	
	DbgError("msg_control->control_cmd =0x%02x \r\n",msg_control->control_cmd);
	switch(msg_control->control_cmd)
	{
		case GB905_CONTROL_UPDATE:
			gb905_update_treat(index,msg_control);
		case GB905_CONTROL_POWER_OFF:
			//system_power_off();
			break;
		case GB905_CONTROL_RESET:
			rt_system_reset();
			break;
		case GB905_CONTROL_RECOVERY:
			//system_recovery();
			break;
		case GB905_CONTROL_OFF_SOCKET:
			break;
		case GB905_CONTROL_OFF_COMMUNICATION:
			break;
		default:
			DbgWarn("Don't support this control command(%d)\r\n",msg_control->control_cmd);
			break;
			
	}

	DbgFuncExit();
    
	return GB905_RESULT_OK;
}


void send_update_result(unsigned char index,unsigned char result)
{
	int len;
	gb905_msg_update_t msg_update;

	DbgFuncEntry();
	
	len = sizeof(msg_update_t);
	
	gb905_build_header(&msg_update.header,MESSAGE_UPDATE_RESULT,len);

	gb905_debug_header(&msg_update.header);

	gb905_build_update_result(&msg_update.update_result,result);

	//gb905_send_data(MAIN_SOCKET,(unsigned char *)&msg_update,sizeof(gb905_msg_update_t));
	gb905_send_data(index,(unsigned char *)&msg_update,sizeof(gb905_msg_update_t));

	//clear_update_info();
	//clear_update_socket_info();
	
	DbgFuncExit();
}