#include <rtthread.h>

#include <system_bsp.h>
#include "gps.h"
#include "gnss_sk.h"
#include "cip_sockmgr.h"

#include "protocol.h"
#include <business.h>

//0x7D ==> 0x7D,0x01;	0x7E ==> 0x7D,0x02
#define		ESCAPE_CHAR			0x7D
#define		MAGIC_CHAR			0x7E

#define		TEMINAL_ID			0x10

int gb905_escape(unsigned char *dest, unsigned char *src, int len)
{
	int src_offset,dest_offset;
	
	if(!dest || !src || !len)
	{
		DbgError("params error!");
		return -1;
	}
	
	for(src_offset=0,dest_offset=0; src_offset<len; src_offset++)
	{
		if(src[src_offset] == ESCAPE_CHAR)
		{
			dest[dest_offset++] = ESCAPE_CHAR;
			dest[dest_offset++] = 0x01;
		}
		else if(src[src_offset] == MAGIC_CHAR)
		{
			dest[dest_offset++] = ESCAPE_CHAR;
			dest[dest_offset++] = 0x02;
		}
		else
		{
			dest[dest_offset++]=src[src_offset];
		}
	}
	
	return dest_offset;
}

int gb905_unescape(unsigned char *dest, unsigned char *src, int len)
{
	int src_offset,dest_offset;

	if( !dest || !src || !len )
	{
		DbgError("params error!");
		return -1;
	}
	
	for(src_offset=0,dest_offset=0; src_offset<len; src_offset++)
	{
		//if(src[src_offset] == MAGIC_CHAR)
		//{
		//	DbgError("magic number error!\r\n");
		//	return -1;
		//}
		
		if(src[src_offset]== ESCAPE_CHAR)
		{
			src_offset++;
			if(src_offset == len)
			{
				DbgError("not unescape number error!");
				return -1;
			}
			else if(src[src_offset] == 0x01)
			{
				dest[dest_offset++] = ESCAPE_CHAR;
			}
			else if(src[src_offset]==0x02)
			{
				dest[dest_offset++] = MAGIC_CHAR;
			}
			else
			{
				DbgError("unescape number error!");
				return -1;
			}
		}
		else
		{
			dest[dest_offset++]=src[src_offset];
		}
	}
	
	return dest_offset;
}



static void gb905_common_ack_treat(unsigned char index,unsigned char *buf,int len)
{
    msg_general_ack_t * ack;

    ack = (msg_general_ack_t *)(&buf[1 + sizeof(gb905_msg_header_t)]);

    ack->seq = EndianReverse16(ack->seq);
	ack->id = EndianReverse16(ack->id);
	DbgWarn("gb905 socket %d common ack->id = 0x%04x,seq=0x%04x,ack_flag=%d",index,ack->id,ack->seq,ack->result);
	
}

static bool find_magic_char(unsigned char *buf, int len,unsigned int * offset)
{
	int index=0;
	bool ret = false;

	*offset = 0;
	
	while(index < len)
	{
		if(buf[index] == MAGIC_CHAR)
		{
			ret = true;
			*offset = index;
			break;
		}
		
		index++;
	}

	return ret;
}

int gb905_find_magic_char(unsigned char *buf, int len,unsigned int * offset)
{
    int i = 0;
    if(find_magic_char(buf, len, offset) == true) {
        i = 1;
    }
    return i;
}


int gb905_get_a_full_msg(unsigned char *buf,int len,gb905_msg_t *msg)
{
	bool ret;
	unsigned int head_offset = 0,tail_offset = 0;
	int offset;

	DbgFuncEntry();
	
	ret = find_magic_char(buf,len,&head_offset);
	if(ret && (len - head_offset >= sizeof(gb905_msg_header_t) + 3))
	{
		ret = find_magic_char(buf + head_offset + 1,len - head_offset - 1,&tail_offset);
		if(ret)
		{
			tail_offset += head_offset + 1;
			//|7e [head][xor] |7e
			//head_offset  tail_offset
			//len = (tail_offset+1)-head_offset
			if((tail_offset + 1) - head_offset < sizeof(gb905_msg_header_t) + 3)
			{
				DbgPrintf("Warning: Recevie message with invalid size, ignore it!!!");
				offset = tail_offset;//too short ,reserve tail as next head
			}
			else
			{
				msg->raw_msg_buf = (buf + head_offset);
				msg->raw_msg_len = tail_offset - head_offset + 1;
				offset = tail_offset + 1;
			}

			
		}
		else
		{
			offset = head_offset;
		}
	}
	else
	{
		msg->raw_msg_buf = NULL; 
		msg->raw_msg_len = 0;
		if(!ret)
		{
			offset = MAX(len-1,0);
		}
		else
		{
			offset=head_offset;
		}
	}

	//DbgPrintf("offset = %d\r\n",offset);
	
	DbgFuncExit();
	return offset;	
}
static int gb905_parse_header(gb905_msg_t * msg)
 {
	int ret = 1;
	//unsigned int mtd_id;
	gb905_msg_header_t * header;

	header = (gb905_msg_header_t *)(msg->msg_buf + 1);



	if( EndianReverse16(header->msg_len) != msg->msg_len - sizeof(gb905_msg_header_t) - 3)
	{
		ret = 0;

		DbgError("msg len is invaild!");
		return ret;
	}
	header->msg_id = EndianReverse16(header->msg_id);
	header->msg_len = EndianReverse16(header->msg_len);
	header->msg_serial_number = EndianReverse16(header->msg_serial_number);
	//uid match is option in this case

	return ret;

 }
static int parse_protocol(unsigned char index,gb905_msg_t * msg)
{
	gb905_msg_header_t * header;
	unsigned short msg_no;
	unsigned char result;

	DbgFuncEntry();

	header = (gb905_msg_header_t *)(msg->msg_buf + 1);

	msg_no = header->msg_id;

	DbgWarn("gb905 socket %d parse message,msg_id:0x%04x,seq:0x%04x,body_data_len:0x%04x",
		index,header->msg_id,header->msg_serial_number,header->msg_len);
	switch (msg_no)
	{
		case MESSAGE_GENERAL_DOWN_ACK:
			gb905_common_ack_treat(index,msg->msg_buf,msg->msg_len);
			break;
		case MESSAGE_SET_PARAMS:
			result = gb905_set_params_treat(index,msg->msg_buf + 1 + sizeof(gb905_msg_header_t),header->msg_len);
			gb905_send_ack(index,header,result);
			break;
		case MESSAGE_GET_PARAMS:
			gb905_get_params_treat(index, msg->msg_buf, msg->msg_len);
			break;
		case MESSAGE_CTRL_TERMINAL:
			result = gb905_control_treat(index,header->msg_serial_number,msg->msg_buf + 1 + sizeof(gb905_msg_header_t),header->msg_len);		
			gb905_send_ack(index,header,result);
			break;
		case MESSAGE_REMOTE_CTRL:
			result = gb905_remote_control_treat(index,header->msg_serial_number,msg->msg_buf + 1 + sizeof(gb905_msg_header_t),header->msg_len);
			gb905_send_ack(index,header,result);
			break;
		case MESSGAE_FT_NEW_SOFT_LOCK:
			result = fleety_remote_new_meter_lock_treat(header->msg_serial_number,msg->msg_buf + 1 + sizeof(gb905_msg_header_t),header->msg_len);
			gb905_send_ack(index,header, result);
		default:
			break;
	}
	return 0;
}



static int parse_a_full_msg(unsigned char index,gb905_msg_t *msg)
{
	int ret;

	DbgFuncEntry();

	ret = gb905_parse_header(msg);
	if( ret <= 0)
	{
		return ret;
	}

	ret = parse_protocol(index,msg);

	DbgFuncExit();

	return ret;
}

int gb905_protocol_anaylze(unsigned char index,unsigned char * buf,int len)
{
	unsigned char x_msg_buf[GNSS_MAX_PASERBUFF_SIZE];
	unsigned char * msg_buf;
	int msg_len;
	int offset;
	int bre_count = 0;
	gb905_msg_t msg;

	DbgFuncEntry();

	msg_buf = buf;
	msg_len = len;
	
	do{
		if(10 == bre_count)
		{
			break;
		}
		memset(&msg,0x00,sizeof(msg));
		
		offset = gb905_get_a_full_msg(msg_buf,msg_len,&msg);

		#if 0
		{
			int i;
				
			DbgPrintf("offset = %d\r\n",offset);
			DbgPrintf("raw_msg_lens = %d\r\n",msg.raw_msg_len);
			
			for(i=0;i<msg.raw_msg_len;i++)
			{
				DbgPrintf("msg_raw_buf[%d] = 0x%2x\r\n",i,msg.raw_msg_buf[i]);
			}
		}
		#endif

		if(offset && msg.raw_msg_len)
		{	
			memset(x_msg_buf,0,sizeof(x_msg_buf));
			msg.msg_buf = x_msg_buf;

			msg.msg_len = gb905_unescape(msg.msg_buf,msg.raw_msg_buf,msg.raw_msg_len);

			#if 0
			{
				int i;
				
				DbgPrintf("msg_len = %d\r\n",msg.msg_len);
				DbgHexGood("main protocol:",msg.msg_buf,msg.msg_len);
			}
			#endif
			if(msg.msg_len>2)
			{
				if(msg.msg_buf[msg.msg_len - 2] != xor8_computer(msg.msg_buf + 1,msg.msg_len - 3))
				{
					DbgError("xor verify failed!");
				}
				else
				{
					parse_a_full_msg(index,&msg);
				}
			}

		}

		msg_buf += offset;
		msg_len -= offset;
		bre_count++;
	}while(offset && msg.raw_msg_len && msg_len > 0);
	
	DbgFuncExit();
	
	return (len - msg_len);


}




void gb905_build_ack(msg_general_ack_t * ack,gb905_msg_header_t * header,unsigned char result)
{
	ack->seq = EndianReverse16(header->msg_serial_number);
	ack->id = EndianReverse16(header->msg_id);
	ack->result = result;
}

void gb905_send_ack(unsigned char index,gb905_msg_header_t * header,unsigned char result)
{
	gb905_general_ack_t ack;

	DbgFuncEntry();
	
	gb905_build_header(&ack.header,MESSAGE_GENERAL_UP_ACK,sizeof(msg_general_ack_t));
	gb905_build_ack(&ack.ack,header,result);
	//DbgWarn("socket %d(%s) send ack,msg_id:0x%04x,seq:0x%04x,ack:0x%02x\r\n",index,get_socket_name(index),header->msg_id,header->msg_serial_number,result);
	DbgWarn("socket %d send ack,msg_id:0x%04x,seq:0x%04x,ack:0x%02x",index,header->msg_id,header->msg_serial_number,result);
	gb905_send_data(index,(unsigned char *)&ack,sizeof(gb905_general_ack_t));

	DbgFuncExit();
}

unsigned int  build_mtdid(unsigned char* buff,int buff_size)
{
	uint8_t uid_buf[20];
	int imei_len=0;
	int imei_offset=0;
	if(buff_size <=12) return 0;
	memset(&buff[0],0,buff_size);
	memset(&uid_buf[0],0,sizeof(uid_buf));
	//build_uuid(&uid_buf[0],sizeof(uid_buf));
	get_imei((char*)&uid_buf[0],sizeof(uid_buf));

	imei_len = strlen((char*)uid_buf);
	if(imei_len>6){imei_offset = imei_len-6;}
	memcpy(&buff[0],"100200",6);
	memcpy(&buff[6],&uid_buf[imei_offset],6);
	buff[12]='\0';
	return strlen((char*)buff);
}

void build_report_id(gb905_msg_header_t* header)
{
	uint8_t uid_buf[20];
	int imei_len=0;
	int imei_offset=0;
	memset(&uid_buf[0],0,sizeof(uid_buf));
	//build_uuid(&uid_buf[0],sizeof(uid_buf));
	get_imei((char*)&uid_buf[0],sizeof(uid_buf));
	
	header->terminal_id[0]=TEMINAL_ID;
	#if 0
	header->terminal_id[1]=PROTOCOL_VENDOR_ID;
	header->terminal_id[2]=0x00;
	header->terminal_id[3]=0x20;
	header->terminal_id[4]=0x90;
	header->terminal_id[5]=0x99;
	#else
	header->terminal_id[1]=PROTOCOL_VENDOR_ID;
	header->terminal_id[2]=0x00;
	imei_len = strlen((char*)uid_buf);
	if(imei_len>6){imei_offset = imei_len-6;}
	str2BCDByteArray((char*)&uid_buf[imei_offset],&header->terminal_id[3]);
	#endif
}
void gb905_build_timestamp(gb905_bcd_timestamp_t * timestamp)
{
	time_t now;
	DateTime datetime_utc;
    rt_device_t rtc_dev;

	DbgFuncEntry();

    rtc_dev = rt_device_find("rtc");
	if(rtc_dev!=RT_NULL){
		rt_device_control(rtc_dev, RT_DEVICE_CTRL_RTC_GET_TIME, &now);
	}
	
	if(get_gps_status()>0){//gps fix use gps time for report
		datetime_utc = makeDateTime();
	}
	else{
		datetime_utc = makeDateTime_utc_zone(now,0);
	}

	decimal2bcd((datetime_utc._year-2000>0?datetime_utc._year-2000:0),&timestamp->bcd[0],1);
	decimal2bcd((datetime_utc._month>0?datetime_utc._month:1),&timestamp->bcd[1],1);
	decimal2bcd((datetime_utc._day>0?datetime_utc._day:1),&timestamp->bcd[2],1);
	decimal2bcd(datetime_utc._hour,&timestamp->bcd[3],1);
	decimal2bcd(datetime_utc._minute,&timestamp->bcd[4],1);
	decimal2bcd(datetime_utc._second,&timestamp->bcd[5],1);

	DbgFuncExit();
}

unsigned int gb905_build_new_timestamp_id(void)
{
	time_t now;
	gb905_timestamp_id_t timestamp_id;
	DateTime datetime_utc;
    rt_device_t rtc_dev;

    rtc_dev = rt_device_find("rtc");
	if(rtc_dev!=RT_NULL){
		rt_device_control(rtc_dev, RT_DEVICE_CTRL_RTC_GET_TIME, &now);
	}
	
	datetime_utc = makeDateTime_utc_zone(now,0);

	timestamp_id.timestamp.sec = datetime_utc._second;
	timestamp_id.timestamp.min = datetime_utc._minute;
	timestamp_id.timestamp.hour = datetime_utc._hour;
	timestamp_id.timestamp.mday = (datetime_utc._day>0?datetime_utc._day:1);
	timestamp_id.timestamp.mon = (datetime_utc._month>0?datetime_utc._month:1);
	
	timestamp_id.timestamp.year = (datetime_utc._year-2010>0?datetime_utc._year-2010:0);

	return timestamp_id.id; 
}

void gb905_build_header(gb905_msg_header_t * header, unsigned short msg_id, unsigned short msg_len)
{

	static 	unsigned short msg_seq_number = 0;
	
	DbgFuncEntry();

	memset(header,0x00,sizeof(gb905_msg_header_t));
	header->msg_id = EndianReverse16(msg_id);
	header->msg_len = EndianReverse16(msg_len);

	build_report_id(header);
	
	header->msg_serial_number = EndianReverse16(msg_seq_number);
	msg_seq_number++;

	DbgFuncExit();
}
static unsigned char temp_buff[512]={0};
int gb905_send_data(unsigned char socket_index,unsigned char * buf, int len)
{
	int size, socket;
	unsigned char * new_buf;
	int new_len;
	unsigned char val;
	
    
	DbgFuncEntry();

	socket = sys_get_socket_id(socket_index);
	if(socket <= 0)
	{
		//DbgError("socket %d(%s)  invaild when send\r\n",socket_index,get_socket_name(socket_index));
		DbgError("from socket %d invaild when send",socket_index);
		return 0;
	}
	
	if(len<3+sizeof(gb905_msg_header_t)||len>255)
	{
		return 0;
	}
	//id fix here

	
	buf[0] = buf[len-1] = MAGIC_CHAR;

	val = xor8_computer((unsigned char *)&buf[1],len - 3);
	buf[len-2] = val;
	memset(temp_buff,0,sizeof(temp_buff));
	new_buf = temp_buff;
	if(sizeof(temp_buff)<len*2)
	{
		DbgError("gb905 Message too large for protocol!");
		return -1;
	}

	new_buf[0] = buf[0];
	new_len = gb905_escape(&new_buf[1],&buf[1],len-2);
	new_len++;
	new_buf[new_len] = buf[len-1];
	new_len++;

	
	size = sys_socket_send(socket_index,new_buf,new_len);

	//free(new_buf);
	
	DbgFuncExit();

	return size;
}

//-----------------------------------------------------------
void gb905_debug_header(gb905_msg_header_t * header)
{
	return;
	DbgFuncEntry();

	DbgPrintf("msg_id = 0x%x",header->msg_id);
	DbgPrintf("msg_len = 0x%x",header->msg_len);
	
	DbgPrintf("termina_id[0] = 0x%02x",header->terminal_id[0]);
	DbgPrintf("termina_id[1] = 0x%02x",header->terminal_id[1]);
	DbgPrintf("termina_id[2] = 0x%02x",header->terminal_id[2]);
	DbgPrintf("termina_id[3] = 0x%02x",header->terminal_id[3]);
	DbgPrintf("termina_id[4] = 0x%02x",header->terminal_id[4]);
	DbgPrintf("termina_id[5] = 0x%02x",header->terminal_id[5]);

	DbgPrintf("msg_serial_number = %d",header->msg_serial_number);
	
	DbgFuncExit();
}

void gb905_debug_ack(unsigned char index,msg_general_ack_t * ack)
{
	DbgFuncEntry();
	//DbgPrintf("from socket %d(%s)\r\n",index,get_socket_name(index));
	DbgPrintf("from socket %d",index);
	DbgPrintf("seq = 0x%x",ack->seq);
	DbgPrintf("id = 0x%x",ack->id);
	DbgPrintf("result = 0x%x",ack->result);
	
	DbgFuncExit();
}
unsigned int get_heat_beat1_period(void){
	function_control_param_t* cur_func_param;
	cur_func_param = get_func_param();
    return cur_func_param->heartbeat_interval;
}
unsigned int get_heat_beat_period(void){
    return 60;
}



typedef  struct
{
    unsigned char start_magic_id;
    gb905_msg_header_t header;
    unsigned char _xor;
    unsigned char end_magic_id;
} __packed gb905_heart_beat_t;

void send_heart_beat(int index)
{
	int len;
	gb905_heart_beat_t heart_beat;

	DbgFuncEntry();
	
	len = 0;
	
	gb905_build_header(&heart_beat.header,MESSAGE_HEART_BEAT,len);

	//gb905_debug_header(&heart_beat.header);

	gb905_send_data(index, (unsigned char *)&heart_beat, sizeof(gb905_heart_beat_t));
	//sys_socket_send(index, (unsigned char *)&heart_beat, sizeof(gb905_heart_beat_t));
	
	DbgFuncExit();
}


