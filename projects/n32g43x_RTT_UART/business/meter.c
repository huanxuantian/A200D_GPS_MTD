
#include <rtthread.h>
#define DBG_LEVEL           DBG_INFO
#define DBG_SECTION_NAME    "METER"
#include <rtdbg.h>
#include "board.h"
#include "utils.h"
#include "setting.h"

#include "business.h"
#include "meter.h"

//meter port function
#include "serial.h"
#include "gnss_if.h"
#include "gnss_port.h"

#include "protocol.h"


static gnss_device_t meter_dev=NULL;
static volatile uint8_t meter_start_flag=0;

CircleBufferMngr uart_cb_buffer;

ALIGN(RT_ALIGN_SIZE)
uint8_t uart_paser_buffer[320];
ALIGN(1)

#define METER_UART 2
#define METER_BUND	9600



void fleety_uart_send_meter(int index,unsigned char *buf, int len)
{
	unsigned char data_buffer[2];
	int rlen;
	if(index==METER_UART)
	{
			int i;
			DbgPrintf("SEND:");
			for(i=0;i<len;i++)
			{
				LOG_RAW("%02x ",buf[i]);
			}
			LOG_RAW("\r\n");
		gnss_send_only(meter_dev,200,buf,len,
		data_buffer,sizeof(data_buffer),&rlen);
	}

}

static bool find_meter_magic_char(unsigned char *buf, int len,unsigned int * offset)
{
	unsigned short * p_magic;
	unsigned short magic;
	
	int index=0;
	bool ret = false;
	
	//gb905_meter_header_t * header;
	
	DbgFuncEntry();
	//header = (gb905_meter_header_t *)buf;
	
	*offset = 0;
	
	while(index < len)
	{
		p_magic = (unsigned short *)(buf+index);
		magic = EndianReverse16(*p_magic);
		
		if(magic == METER_MAGIC_NUM)
		{
			ret = true;
			*offset = index;
			break;
		}
		
		index++;
	}
	
	DbgFuncExit();
	return ret;
}


static int get_a_full_meter_msg(double_buff_mgr_t *msg)
{
	bool ret;
	unsigned int head_offset = 0,tail_offset = 0;
	int offset=0;
	DbgFuncEntry();
	
	ret = find_meter_magic_char(msg->raw.buf,msg->raw.len,&head_offset);
	if(ret && (msg->raw.len - head_offset >= sizeof(gb905_meter_header_t) + sizeof(gb905_meter_tail_t)))
	{
		ret = find_meter_magic_char(msg->raw.buf + head_offset + 2,msg->raw.len - head_offset - 2,&tail_offset);
		tail_offset += head_offset + 2;
		if(ret)
		{
			if(tail_offset - head_offset + 2 >= sizeof(gb905_meter_header_t) + sizeof(gb905_meter_tail_t))
			{
			
				msg->now.buf = (msg->raw.buf + head_offset);
				msg->now.len = tail_offset - head_offset + 2;
				offset = tail_offset + 2;
			}
			else
			{
				msg->now.buf = NULL; 
				msg->now.len = 0;
				offset = tail_offset;//too short ,reserve tail as next head
			}
		}
		else
		{
			msg->now.buf = NULL; 
			msg->now.len = 0;
			offset = head_offset;
		}
	}
	else
	{
		msg->now.buf = NULL; 
		msg->now.len = 0;
		if(!ret)
		{
			offset = MAX(msg->raw.len-2,0);
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


void gb905_meter_build_header(gb905_meter_header_t* header,unsigned short cmd,unsigned short len)
{
	header->magic_start = EndianReverse16(METER_MAGIC_NUM);
	header->len = EndianReverse16(len);
	header->type = METER_TYPE_ID;
	header->vendor = METER_VENDOR_ID;
	header->cmd = EndianReverse16(cmd);
}

 void gb905_meter_build_tail(gb905_meter_tail_t * tail,unsigned char xor)
{
	tail->xor = xor;
	tail->magic_end = EndianReverse16(METER_MAGIC_NUM);
}

static int gb905_meter_common_ack(unsigned short command,unsigned char result)
{
	gb905_meter_common_ack_msg_t ack_msg;
	unsigned char ack_len;
	
	unsigned char xor;
	DbgFuncEntry();
	ack_len = sizeof(gb905_meter_common_ack_msg_t);
	
	gb905_meter_build_header(&ack_msg.header,command,sizeof(unsigned char) + sizeof(gb905_meter_header_t) - offsetof(gb905_meter_header_t,type));

	ack_msg.result = result;
	
	
	xor = xor8_computer(((unsigned char*)&ack_msg) + offsetof(gb905_meter_header_t,len),sizeof(unsigned char) + sizeof(gb905_meter_header_t) - offsetof(gb905_meter_header_t,len));
	gb905_meter_build_tail(&ack_msg.tail,xor);

	fleety_uart_send_meter(METER_UART,((unsigned char*)&ack_msg),ack_len);
	
	DbgFuncExit();
	
	return 0;
}


static unsigned char gb905_meter_occupied(unsigned char * buf,unsigned short len)
{
    gb905_meter_bcd_timestamp_t * occupied_timestamp;
    
	DbgFuncEntry();
	if(len>=sizeof(gb905_meter_bcd_timestamp_t))
	{
		occupied_timestamp = (gb905_meter_bcd_timestamp_t *)buf;
		DbgWarn("occupied time: %02x%02x%02x%02x%02x%02x%02x",occupied_timestamp->bcd[0],occupied_timestamp->bcd[1],occupied_timestamp->bcd[2],
			occupied_timestamp->bcd[3],occupied_timestamp->bcd[4],occupied_timestamp->bcd[5],occupied_timestamp->bcd[6]);
	}
    update_loading_state(1);
	gb905_build_pre_operate_start();
	
    DbgFuncExit();
	return METER_RESULT_OK;
}


static unsigned char gb905_2014_meter_empty(unsigned short cmd,unsigned char * buf,unsigned short len)
{
    gb2014_meter_operation_t * meter_operation;
    DbgFuncEntry();
	meter_operation = (gb2014_meter_operation_t *)buf;
    //构建默认营运数据上报消息

    update_loading_state(0);
	gb905_send_operate(meter_operation);
	
    DbgFuncExit();
	return METER_RESULT_OK;
}
static uint8_t wait_ack_flag=0;
static void gb905_2014_meter_parse_heart_beat(unsigned char *buf,unsigned short len)
{
    static int last_stop_state=-1;
    gb2014_meter_heart_beat_t * meter_heart_beat;
	unsigned short soft_lock_subtype;
	unsigned char soft_lock_cmd;
	unsigned char new_soft_lock_flag;

    DbgFuncEntry();
	meter_heart_beat = (gb2014_meter_heart_beat_t *)buf;
    //检查空重车和锁定状态，变化时上报。
    if(last_stop_state==-1||meter_heart_beat->meter_state.flag.stop!=last_stop_state)
    {
        last_stop_state = meter_heart_beat->meter_state.flag.stop;
        //notify state change event here
    }
    if(get_loading_state()!=meter_heart_beat->meter_state.flag.loading)
    {
        if(meter_heart_beat->meter_state.flag.loading!=0){
            update_loading_state(1);
        }else{
            update_loading_state(0);
        }
        
    }
	//检测需要应答标志
	new_soft_lock_flag=get_meter_soft_new_lock_flag();
	
	if(new_soft_lock_flag&& wait_ack_flag){
		DbgWarn("[wait flag]wait meter state flag checkout!");
		wait_ack_flag=0;
		soft_lock_subtype = get_meter_soft_lock_subtype();
		soft_lock_cmd=get_meter_soft_new_lock_status_subtype(soft_lock_subtype&0xFF);

		if(soft_lock_cmd){
			if(meter_heart_beat->meter_state.flag.stop){
					unsigned short seq = get_meter_soft_lock_seq();
					fleety_remote_new_meter_lock_ack(METER_SOFT_LOCK_ACK_OK,seq,soft_lock_subtype);
					clear_meter_soft_lock_seq();
					DbgWarn("meter lock check ,now ack soft lock!!!\r\n");
					clr_meter_soft_new_lock_flag();//all clear 
			}else{
					unsigned short seq = get_meter_soft_lock_seq();
					fleety_remote_new_meter_lock_ack(METER_SOFT_LOCK_ACK_FAIL,seq,soft_lock_subtype);
					clear_meter_soft_lock_seq();
					DbgError("meter lock check ,now ack soft lock[fail]!!!\r\n");
					clr_meter_soft_new_lock_flag();//all clear 
			}
			
		}else{
			if(meter_heart_beat->meter_state.flag.stop==0)
			{
				//soft unlock finish need ack 0x0c24
				unsigned short seq = get_meter_soft_lock_seq();
				fleety_remote_new_meter_lock_ack(METER_SOFT_UNLOCK_ACK_OK,seq,soft_lock_subtype);
				clear_meter_soft_lock_seq();
				DbgWarn("meter lock check ,now ack soft unlock!!!\r\n");
				clr_meter_soft_new_lock_flag();//all clear 
			}else{
				unsigned short seq = get_meter_soft_lock_seq();
				fleety_remote_new_meter_lock_ack(METER_SOFT_UNLOCK_ACK_FAIL,seq,soft_lock_subtype);
				clear_meter_soft_lock_seq();
				DbgError("meter lock check ,now ack soft unlock[fail]!!!\r\n");
				clr_meter_soft_new_lock_flag();//all clear 
			}
		}

	}
	

	

    DbgFuncExit();
}

void build_meter_force_lock_time(limit_lock_timestamp_t* limit_time)
{
	decimal2bcd(1970,&limit_time->bcd[0],2);//年
	decimal2bcd(1,&limit_time->bcd[2],1);//月
	decimal2bcd(1,&limit_time->bcd[3],1);//日
	decimal2bcd(0,&limit_time->bcd[4],3);//时分秒
}


static void gb2014_meter_build_heart_beat_ack(gb905_meter_heart_beat_ack_t * heart_beat_ack)
{
	static uint8_t last_flag=-1;
	unsigned char new_soft_lock_flag;
	meter_limit_lock_info_t* limit_lock_info = get_meter_limit_lock_info();

	if(limit_lock_info->lock_flag){
		build_meter_force_lock_time(&limit_lock_info->limit_time);
		memcpy(&heart_beat_ack->time_threhold,&limit_lock_info->limit_time,sizeof(heart_beat_ack->time_threhold));
	}
	if(last_flag!=limit_lock_info->lock_flag)
	{
		new_soft_lock_flag=get_meter_soft_new_lock_flag();
		if(last_flag!=-1 && new_soft_lock_flag){
			wait_ack_flag=1;
			DbgWarn("[wait flag]wait meter next state flag now!");
		}
		last_flag = limit_lock_info->lock_flag;
	}
}

static int gb2014_meter_heart_beat_ack(void)
{
	gb905_meter_heart_beat_ack_msg_t ack_msg;
	unsigned short ack_len;
	
	unsigned char xor;
	DbgFuncEntry();
	ack_len = sizeof(gb905_meter_heart_beat_ack_msg_t);

	memset(&ack_msg,0,ack_len);
	
	gb905_meter_build_header(&ack_msg.header,METER_COMMAND_HEART_BEAT,sizeof(gb905_meter_heart_beat_ack_t) + sizeof(gb905_meter_header_t) - offsetof(gb905_meter_header_t,type));
	
	gb2014_meter_build_heart_beat_ack(&ack_msg.ack);
	
	xor = xor8_computer(((unsigned char*)&ack_msg) + offsetof(gb905_meter_header_t,len),sizeof(gb905_meter_heart_beat_ack_t) + sizeof(gb905_meter_header_t) - offsetof(gb905_meter_header_t,len));

	gb905_meter_build_tail(&ack_msg.tail,xor);
	fleety_uart_send_meter(METER_UART,((unsigned char*)&ack_msg),ack_len);
	
	DbgFuncExit();
	return 0;
}
void gb2014_meter_heart_beat_send()
{
	gb2014_meter_heart_beat_ack();
}


static bool meter_parse_header(double_buff_mgr_t * msg)
{
	bool ret = true;
	gb905_meter_header_t * header;
	DbgFuncEntry();
	header = (gb905_meter_header_t *)(msg->now.buf);
	header->len = EndianReverse16(header->len);
	header->cmd = EndianReverse16(header->cmd);
	
	
	// len : start from device_info
	if(header->len != msg->now.len - (int)offsetof(gb905_meter_header_t,type) - sizeof(gb905_meter_tail_t))
	{
		DbgError("msg len is invaild!\r\n");
				
		ret = false;
		return ret;
	}
	
	if(header->type != METER_TYPE_ID)
	{
		DbgError("device type error!\r\n");
				
		ret = false;
		return ret;
	}
	
	if( header->vendor != METER_VENDOR_ID )
	{
		DbgError("vendor id is %d!\r\n",header->vendor);
		//for jintan
		//ret = false;
		//return ret;
	}
	
	DbgFuncExit();
	
	return ret;
}

static int meter_parse_protocol(double_buff_mgr_t * msg)
{
    int ret = 0;
    unsigned char result = METER_RESULT_OK;
    gb905_meter_header_t * header;
	unsigned char * meter_data;
	unsigned short meter_data_len;

    DbgFuncEntry();
    header = (gb905_meter_header_t *)(msg->now.buf);
	meter_data = (unsigned char *)(msg->now.buf + sizeof(gb905_meter_header_t));
	meter_data_len = header->len - 4;

	DbgWarn(" Meter receive cmd = 0x%04x\r\n",header->cmd);

	#if 0
	DbgPrintf("meter paser buf::\r\n");
	int i=0;
	for(i=0;i<msg->now.len;i++)
	{
		LOG_RAW("%02x ",msg->now.buf[i]);
	}
	LOG_RAW("\r\n");
	#endif
	#if 0
	DbgPrintf("meter data buf::\r\n");
	int i=0;
	for(i=0;i<meter_data_len;i++)
	{
		LOG_RAW("%02x ",meter_data[i]);
	}
	LOG_RAW("\r\n");
	#endif

    switch(header->cmd)
    {
        case METER_COMMAND_HEART_BEAT:
            gb905_2014_meter_parse_heart_beat(meter_data,meter_data_len);
            gb2014_meter_heart_beat_ack();
            break;
        case METER_COMMAND_OPERATION_OCCUPIED:
            result = gb905_meter_occupied(meter_data,meter_data_len);
            gb905_meter_common_ack(header->cmd,result);
            break;
        case METER_COMMAND_OPERATION_EMPTY:
            result = gb905_2014_meter_empty(header->cmd,meter_data,meter_data_len);
            gb905_meter_common_ack(header->cmd,result);
            break;
		case METER_COMMAND_TEST_UNLOCK:
			clear_all_meter_lock_status();
			gb905_meter_common_ack(header->cmd,result);
			break;
		case METER_COMMAND_TEST_LOCK:
			set_test_lock();
			gb905_meter_common_ack(header->cmd,result);
			break;
        default:
            DbgWarn("don't support this meter command id= %04x!\r\n",header->cmd);
			ret = -1;
            break;
    }
    if(result!=METER_RESULT_OK){
        DbgWarn("meter msg [%04x],ack:[%d]",header->cmd,result);
    }
    return ret;
}

/*
* 分析一条消息命令
* @msg： 存放完整消息命令的buf
* @header : 解析出协议的头信息
* @return：解析出协议，需要偏移的长度
*/
static int parse_a_full_meter_msg(double_buff_mgr_t *msg)
{
	int ret;
	DbgFuncEntry();
	ret = meter_parse_header(msg);//jintan 905
	DbgWarn(" ret = %d\r\n",ret);
	if(!ret)
	{
		return ret;
	}
	ret = meter_parse_protocol(msg);//jintan 905
	
	DbgFuncExit();
	return ret;
}

int meter_protocol_anaylze(unsigned char * buf,int len)
{
	unsigned char * msg_buf;
	int msg_len;
	
	int offset;
	
	double_buff_mgr_t meter_buf;
	DbgFuncEntry();
	msg_buf = buf;
	msg_len = len;
	
	do{
		memset(&meter_buf,0,sizeof(double_buff_mgr_t));
		
		meter_buf.raw.buf = msg_buf;
		meter_buf.raw.len = msg_len;
		
		offset = get_a_full_meter_msg(&meter_buf);
#if 1
		if(meter_buf.now.buf)
		{
			
				
			DbgPrintf("offset = %d\r\n",offset);
			DbgPrintf("meter_lens = %d\r\n",meter_buf.now.len);
			
			//DbgHexGood("meter rcv buf::\r\n", meter_buf.now.buf, meter_buf.now.len);
			#if 0
			DbgPrintf("meter rcv buf::\r\n");
			int i=0;
			for(i=0;i<len;i++)
			{
				LOG_RAW("%02x ",buf[i]);
			}
			LOG_RAW("\r\n");
			#endif
		}
#endif
		if(meter_buf.now.buf && offset>2 && meter_buf.now.len>= sizeof(gb905_meter_header_t) + sizeof(gb905_meter_tail_t))
		{
			// 3:magic_end + xor				5: magic_start+magic_end+xor
			if(meter_buf.now.buf[meter_buf.now.len - 3] != xor8_computer(meter_buf.now.buf + 2,meter_buf.now.len - 5))
			{
				DbgError("xor verify failed!\r\n");
				//return -1;
				offset -=2;
			}
			else
			{
				parse_a_full_meter_msg(&meter_buf);
			}
		}
		msg_buf += offset;
		msg_len -= offset;
	}while(offset && meter_buf.raw.len && msg_len > 0);
	
	return (len - msg_len);
	DbgFuncExit();
}

static uint8_t ring_buff[250];
//business function
static int meter_paser_callback(gnss_ctx_t *ctx,int read_len,int ack_flag)
{
	CircleBufferMngr* mgr;
	int len,size;
	mgr = &uart_cb_buffer;
	DbgWarn("recv data from meter");
	debug_print_block_data(0,ctx->read_buf,read_len);
	if(cb_is_vaild(mgr)){
		cb_write(mgr,ctx->read_buf,read_len);
		memset(ring_buff,0,sizeof(ring_buff));
        
        len = cb_datalen(mgr);
        if(len>0){
            size = cb_read_no_offset(mgr,ring_buff,MIN(len,sizeof(ring_buff)));
            
            size = meter_protocol_anaylze(ring_buff,size);
            cb_read_move_offset(mgr,size);
        }	
	}
    return 0;
}

void meter_init()
{
    int ret = GNSS_RT_EOK;
    meter_dev =  create_gnss_modify_buffer(200);
    if(meter_dev==NULL){
        LOG_E("meter_dev init error");
        return;
    }
    ret = gnss_set_serial(meter_dev,USE_METER_UART,METER_BUND,8,'N',STOP_BITS_1,0);
    if(GNSS_RT_EOK != ret ){
        LOG_E("meter_dev init uart error");
        return;
    }
    gnss_set_parse_callback(meter_dev,meter_paser_callback);

    ret = gnss_open_default(meter_dev);
    if(GNSS_RT_EOK != ret ){
        LOG_E("meter_dev open uart error");
        return;
    }
	memset((void*)&uart_cb_buffer,0,sizeof(uart_cb_buffer));
	cb_init_static(&uart_cb_buffer,uart_paser_buffer,sizeof(uart_paser_buffer));

    meter_start_flag=1;

}

static uint32_t last_check_time=0;

void handle_meter_task()
{
    if(meter_start_flag)
    {
        //check status for device here
		if(OSTimeGet() - last_check_time > 30)
		{
			last_check_time = OSTimeGet();
			LOG_E("meter task tick!");
		}
		
    }
}

void stop_meter_task()
{
    meter_start_flag=0;
    if(meter_dev!=RT_NULL)
	{
		if(gnss_isopen(meter_dev))
		{
			gnss_close(meter_dev);
			rt_thread_mdelay(20);
            meter_dev =NULL;
		}
	}

}
