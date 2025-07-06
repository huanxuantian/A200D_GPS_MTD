#define DBG_LVL    DBG_WARNING
#define DBG_TAG    "CIP"
#include <rtdbg.h>
#include <rtthread.h>

#include <utils.h>
#include <MicroNMEA.h>
#include "cip_sockmgr.h"

extern int gateway_handle(int index,void* data,int data_size);

#define MAX_TRY_COUNT 3
#define MAX_PACK_SIZE 500

uint8_t cclk_fail=0;
gnss_device_t cip_dev=NULL;
CircleBufferMngr gsm_recvbuff_mgr[MAX_SOCK_USE];

static uint8_t socket_flag[MAX_SOCK_USE]={0};
static uint32_t socket_err[MAX_SOCK_USE]={0};
static uint8_t socket_recv_flag[MAX_SOCK_USE]={0};

ALIGN(RT_ALIGN_SIZE)
static char at_buff[256];
static char at_recv[768];

static unsigned char cSocketRecvBuf[SOCKET_BUF_SIZE];

uint8_t paser_buffer[GNSS_MAX_PASERBUFF_SIZE];
uint8_t paser_buffer1[GNSS_MAX_PASERBUFF_SIZE];
ALIGN(1)

static int send_AT_DATA(gnss_device_t gsm_dev,unsigned char* send_data,int data_len,char* match,char* fmatch,int mstimeout);
static void recv_data_handle(int index,void* data,int data_size);

static int cip_sock_recv(gnss_device_t gsm_dev,int index,void* buffer,int buffer_size);
static int cip_sock_send(gnss_device_t gsm_dev,int index,void* data,int data_size);
static void parse_cclk(char* str);

static int cip_sock_select(gnss_device_t gsm_dev,sock_set_t read_set,int mstimeout);

void init_cb_buffer(){
    memset((void*)&gsm_recvbuff_mgr[0],0,sizeof(gsm_recvbuff_mgr));
	cb_init_static(&gsm_recvbuff_mgr[0],paser_buffer,sizeof(paser_buffer));
    cb_init_static(&gsm_recvbuff_mgr[1],paser_buffer1,sizeof(paser_buffer1));
}

void init_cip_dev(gnss_device_t dev)
{
    if(cip_dev==NULL)cip_dev = dev;
}

uint8_t get_socket_flag(int index)
{
    if(index<0||index>=MAX_SOCK_USE){return 0;}
    return socket_flag[index];
}

void set_socket_flag(int index,uint8_t value)
{
    if(index<0||index>=MAX_SOCK_USE){return;}
    //socket_flag[index] = value;
}

uint32_t get_socket_err(int index)
{
    if(index<0||index>=MAX_SOCK_USE){return 0;}
    return socket_err[index];
}

void set_socket_err(int index,uint32_t value)
{
    if(index<0||index>=MAX_SOCK_USE){return;}
    socket_err[index] = value;
}
uint32_t inc_socket_err(int index)
{
    if(index<0||index>=MAX_SOCK_USE){return 0;}
    return socket_err[index]++;
}


char* get_at_recv_ptr()
{
    return at_recv;
}

int at_cmdres_keyword_matching(char *keyword){
    if(keyword==NULL) return 0;
    if(strstr(at_recv,keyword)!=NULL) {
        return 1;
    }
    return 0;
}
static int at_urc_keyword_matching(unsigned char* data,char *keyword){
    if(keyword==NULL) return 0;
    if(strstr((char*)data,keyword)!=NULL) {
        return 1;
    }
    return 0;
}

int send_AT_CMD(gnss_device_t gsm_dev,char* cmd,char* match,int mstimeout)
{
    int ret;
	int rlen;
	int i=0;
	int print_flag=1;
	int match_ok=0;
	unsigned int start_ms_tick=0;
    memset(at_buff,0,sizeof(at_buff));
		memset(at_recv,0,sizeof(at_recv));
    rt_sprintf(at_buff,"%s\r\n",cmd);
	if(at_urc_keyword_matching((unsigned char*)at_buff,"+CIPRXGET=4")<=0){
			LOG_W("at send :[%s]",at_buff);
	}
		start_ms_tick = OSTimeGetMS();
    ret = gnss_excuse(gsm_dev,mstimeout,at_buff,strlen(at_buff),at_recv,sizeof(at_recv)-1,&rlen);
    
	if(ret==0){
		if(OSTimeGetMS()-start_ms_tick <mstimeout){
			while(OSTimeGetMS()-start_ms_tick <mstimeout){
				
				//check match here
				if(at_urc_keyword_matching((unsigned char*)at_recv,match)){
					match_ok =1;
                    LOG_W(" check match OK EXT");
				}
				rt_thread_mdelay(50);
			}
			
		}
		if(match_ok==0){LOG_W(" check match NOK"); return 0;}
	}
	if(at_urc_keyword_matching((unsigned char*)at_recv,"+CIPRXGET: 4,")||
		at_urc_keyword_matching((unsigned char*)at_recv,"+CIPRXGET: 2,"))
	{
		print_flag = 0;
	}
	if(print_flag){
		LOG_W("at ret=%d,rlen=%d",ret,rlen);
		for(i=0;i<rlen;i++){
			if(i==0){LOG_W("at recv size=%d[",rlen);}
			LOG_RAW("%c",*(at_recv+i));
			if(i==(rlen-1)){LOG_W("]");}
		}
	}
	return 1;
}

static int send_AT_DATA(gnss_device_t gsm_dev,unsigned char* send_data,int data_len,char* match,char* fmatch,int mstimeout){
    int ret;
	int rlen;
	int i=0;
		int match_ok=0;
	unsigned int start_ms_tick=0;
		start_ms_tick = OSTimeGetMS();
		LOG_I("at send data,len=%d",data_len);
		//debug_print_block_data(0,send_data,data_len);
    ret = gnss_send_only(gsm_dev,mstimeout,send_data,data_len,at_recv,sizeof(at_recv)-1,&rlen);
	
    LOG_I("at data ret=%d,rlen=%d",ret,rlen);
    if(ret==0){ 
					if(OSTimeGetMS()-start_ms_tick <mstimeout){
			while(OSTimeGetMS()-start_ms_tick <mstimeout){
				rt_thread_mdelay(50);
				//check match here
				if(at_urc_keyword_matching((unsigned char*)at_recv,match)){
					match_ok =1;
				}
			}
			
		}
		while(OSTimeGetMS()-start_ms_tick <mstimeout){
				rt_thread_mdelay(50);
				//check match here
				if(at_urc_keyword_matching((unsigned char*)at_recv,match)||
					at_urc_keyword_matching((unsigned char*)at_recv,fmatch)){
					match_ok =1;
				}
			}
		if(match_ok==0){return 0;}
		}
     for(i=0;i<rlen;i++){
        if(i==0){LOG_W("at recv size=%d[",rlen);}
        LOG_RAW("%c",*(at_recv+i));
        if(i==(rlen-1)){LOG_W("]");}
    }
    return 1;
}

static void recv_data_handle(int index,void* data,int data_size){
	CircleBufferMngr* mgr;
	uint8_t ring_buff[GNSS_MAX_PASERBUFF_SIZE];
	int len,size;
	//index =0 only
	mgr = &gsm_recvbuff_mgr[index%MAX_SOCK_USE];
	if(index==2){
		LOG_W("raw data recv ok for auth sock!!!");
		//debug_print_block_data(0,data,data_size);
	}
    if(cb_is_vaild(mgr)){
        cb_write(mgr,data,data_size);
        
        memset(ring_buff,0,sizeof(ring_buff));
        
        len = cb_datalen(mgr);
        if(len>0){
            size = cb_read_no_offset(mgr,ring_buff,MIN(len,sizeof(ring_buff)));
            
            size = gateway_handle(index,ring_buff,size);
            cb_read_move_offset(mgr,size);
        }	   
    }
}
int recv_data_select(gnss_device_t gsm_dev)
{
    int ret=0;
    struct sock_set set_data;
    sock_set_t set_ptr=&set_data;
    memset(set_ptr->sock_array,0xff,sizeof(set_ptr->sock_array));
    sock_set_add(set_ptr,0,0);
    sock_set_add(set_ptr,1,1);
    set_ptr->sock_size =2;

    ret = cip_sock_select(gsm_dev,set_ptr,0);
    
    if(ret>0){
        LOG_W("sock selct OK,value=%d!",ret);
        sock_clr_flag(set_ptr);
    }
    if(socket_recv_flag[0]==1){
        socket_recv_flag[0]=0;
    }
    if(socket_recv_flag[1]==1){
        socket_recv_flag[1]=0;
    }
    return ret;
}
int recv_data_check(gnss_device_t gsm_dev,int index)
{
    int ret=0;
    ret = cip_sock_recv(gsm_dev,index,cSocketRecvBuf,sizeof(cSocketRecvBuf));
    if(ret>0){
        recv_data_handle(index,cSocketRecvBuf,ret);
    }
    if(socket_recv_flag[index]==1){
        socket_recv_flag[index]=0;
    }
    return ret;
}
/**
 * @description: 截取socket接收缓存区的数据长度
 * @param cmdres:命令响应
 * @return socket接收缓存区的数据长度
 */
static int match_recv_len(int index,char *cmdres)
{
    int length = 0;
	char match[20]={0};
    char tmp[5] = {0};
    char *p = NULL, *q = NULL;
		rt_sprintf(match,"+CIPRXGET: 4,%d,",index%MAX_SOCK_USE);
    p = strstr(cmdres, match);
    if (p == NULL)
    {
        return 0;
    }
    p = p + strlen(match);
    q = p;
    while (*p >= 0x30 && *p <= 0x39)
    {
        p++;
    }
    memcpy(tmp, q, p - q);
    length = atoi(tmp);
    return length;
}

/**
 * @description: 截取接收到的有效负载
 * @param cmdres：命令响应数据
 * @param recvbuf：socket接收缓存
 * @return 接收数据的长度
 */
static int match_recv_data(int index,char *cmdres,int cmdsize,unsigned char *recvbuf,int buf_size)
{
    int length = 0;
		char match[20]={0};
    char tmp[5] = {0};
    char *p = NULL, *q = NULL;
		rt_sprintf(match,"+CIPRXGET: 2,%d,",index%MAX_SOCK_USE);
    p = strstr(cmdres, match);
    if (p == NULL)
    {
        return 0;
    }
    p = p + strlen(match);
    q = p;
    while (*p >= 0x30 && *p <= 0x39)
    {
        p++;
    }
    if(p - q >=5){
        return 0;
    }
    memcpy(tmp, q, p - q);
    length = atoi(tmp);

    p = strstr(p, "\r\n");
    if(p==NULL) return 0;
    p = p + 2;
    if(length<=0) return 0;
    if(length > cmdsize - (p-cmdres)) return 0;

    memset(recvbuf, 0, buf_size);

    memcpy(recvbuf, p, MIN(length,buf_size));

    return MIN(length,buf_size);
}


int cip_sock_open(gnss_device_t gsm_dev,int index,char* host,int port){
    int try_count=0;
    char send_cmd[64];
    char ack_match[20];
	char ack_match1[20];
    char ack_match2[20];
    rt_sprintf(send_cmd,"AT+CIPOPEN=%d,\"TCP\",\"%s\",%d",index%MAX_SOCK_USE,host,port%65535);
    rt_sprintf(ack_match1,"+CIPOPEN: %d,0",index%MAX_SOCK_USE);
	rt_sprintf(ack_match,"+CIPOPEN: %d,4",index%MAX_SOCK_USE);//4 as connected more then once
    rt_sprintf(ack_match2,"+CIPOPEN: %d,1",index%MAX_SOCK_USE);
    do{
        if(socket_flag[index%MAX_SOCK_USE]==1){
            return index;
        }
        if (send_AT_CMD(gsm_dev,send_cmd,ack_match1, 15000))
        {
            
            if (at_cmdres_keyword_matching(ack_match1))
            {
                return index;
            }
            
        }
        if(at_cmdres_keyword_matching(ack_match))
        {
            LOG_D("open for  sock %d CONTINUE",index%MAX_SOCK_USE);
        }
        if(at_cmdres_keyword_matching(ack_match2)){
                    break;//connect not OK (not timeout)
        }
        if(socket_flag[index%MAX_SOCK_USE]==1){
            return index;
        }
        rt_thread_mdelay(500);
        if(socket_flag[index%MAX_SOCK_USE]==2){
            socket_flag[index%MAX_SOCK_USE]=0;
            break;//connect not OK (not timeout)
        }
        try_count++;
    }while(try_count<MAX_TRY_COUNT);
    if(socket_flag[index%MAX_SOCK_USE]==1){
        return index;
    }
		LOG_E("open for  sock %d error",index%MAX_SOCK_USE);
    return -1;
}

int cip_sock_close(gnss_device_t gsm_dev,int index){
    int try_count=0;
    char send_cmd[24];
    char ack_match[20];
    rt_sprintf(send_cmd,"AT+CICLOSE=%d",index%MAX_SOCK_USE);
    rt_sprintf(ack_match,"+CIPCLOSE: %d,0",index%MAX_SOCK_USE);
    do{
				if(socket_flag[index%MAX_SOCK_USE]==0){
					return index;
				}
				if (send_AT_CMD(gsm_dev,send_cmd,ack_match, 5000))
        {
            
            if (at_cmdres_keyword_matching(ack_match))
            {
                return index;
            }
        }
				if(socket_flag[index%MAX_SOCK_USE]==0){
					return index;
				}
        rt_thread_mdelay(500);
        try_count++;
    }while(try_count<=MAX_TRY_COUNT);
		LOG_E("close for  sock %d error",index%MAX_SOCK_USE);
    return -1;
}

/* 
 * dev for send at(should be mgr self)
 * param -
 *  index: 0-3 for cip index
 *  data: data to send 
 *  data_size: size of data to send
 * return -
 *  < 0: error(now not define detail),failed or not ack in case
 *  > 0: size sended success send data
 */ 
static int cip_sock_send(gnss_device_t gsm_dev,int index,void* data,int data_size){
    int try_count=0;
		int data_send_count=0;
    int can_send_state = 0;
    char send_cmd[24];
		char match[24];
		rt_sprintf(match,"+IPCLOSE: %d,",index);
    rt_sprintf(send_cmd,"AT+CIPSEND=%d,%d",index%MAX_SOCK_USE,data_size);
    do{
        if (send_AT_CMD(gsm_dev,send_cmd,">", 6000))
        {
            //收到>
            if (at_cmdres_keyword_matching(">"))
            {
                can_send_state=1;
            }
        }
        if (at_cmdres_keyword_matching(match)||socket_flag[index%MAX_SOCK_USE]==0)
        {
            LOG_E("cip disconnected,not try more send");
            break;
        }
        if(can_send_state >0){
			do{
                if(send_AT_DATA(gsm_dev,data,data_size,"+CIPSEND: ","+CIPERROR:",15000))
                {
                    //收到OK,重新跳至查询缓存是否有数据
                    if (at_cmdres_keyword_matching("OK"))
                    {
                        if(socket_flag[index%MAX_SOCK_USE]==0){
                            //socket_flag[index%MAX_SOCK_USE]=1;
                        }
                        return data_size;
                    }else if (at_cmdres_keyword_matching("+CIPERROR:"))
                    {
                        LOG_E("cip send error,need reconnect");
                        socket_flag[index%MAX_SOCK_USE]=0;
                        break;
                    }
                }else{
                            
                }
                LOG_E("cip send noack,need resend data");
                data_send_count++;
                rt_thread_mdelay(500);
                    
            }while(data_send_count<=MAX_TRY_COUNT);
        }else{
            LOG_E("cip send step,need retry data");
        }
        rt_thread_mdelay(500);
        try_count++;
    }while(try_count<=MAX_TRY_COUNT);
		//socket_flag[index%MAX_SOCK_USE]=0;
		LOG_E("send data for  sock %d error",index%MAX_SOCK_USE);
    return -1;
}

int cip_sock_read_select(gnss_device_t gsm_dev,int index,int mstimeout){
    int ret=0;
    char send_cmd[24];
    char ack_match[20];
    rt_sprintf(send_cmd,"AT+CIPRXGET=4,%d",index%MAX_SOCK_USE);
    rt_sprintf(ack_match,"+CIPRXGET: 4,%d,",index%MAX_SOCK_USE);
    if (send_AT_CMD(gsm_dev,send_cmd,ack_match, mstimeout))
    {
                    //收到+CIPRXGET: 4,为正确应答，如果逗号后的数字大于0即为有数据
        if (at_cmdres_keyword_matching(ack_match))
        {
                            //截取接收的数据长度
            ret = match_recv_len(index,at_recv);
            if (ret > 0)
            {
                //check ok
            }else if(ret ==0){
                return 0;
            }
        }
        else
        {
            ret =-1;
        }
    }
    return ret;
}

int cip_sock_select_recv(gnss_device_t gsm_dev,int index,void* buffer,int buffer_size,int read_size){
    int ret=0;
    int can_read_bytes=read_size;
    int real_read_size=0;
	int try_recv_count=0;
    char send_cmd[32];
    char ack_match[20];
        if(can_read_bytes >0){
        if(can_read_bytes < buffer_size)
        {
            real_read_size = can_read_bytes;
        }else{
            if((MAX_PACK_SIZE-20)<buffer_size){
                real_read_size = MAX_PACK_SIZE-20;
            }else{
                real_read_size = buffer_size;
            }
        }
        rt_sprintf(send_cmd,"AT+CIPRXGET=2,%d,%d",index%MAX_SOCK_USE,real_read_size);
        rt_sprintf(ack_match,"+CIPRXGET: 2,%d,",index%MAX_SOCK_USE);
				do{
        if (send_AT_CMD(gsm_dev,send_cmd,ack_match, 1000))
        {
						//收到+CIPRXGET为正确应答
			if (at_cmdres_keyword_matching(ack_match))
            {
				//截取接收的数据
                ret = match_recv_data(index,at_recv,sizeof(at_recv), buffer,buffer_size);
                if (ret < 0)
                {
					LOG_E("recv buffer data(paser) for  sock %d error",index%MAX_SOCK_USE);
                    ret =-1;
                }else{break;}
            }else{
							LOG_E("recv buffer data(match) for sock %d error",index%MAX_SOCK_USE);
							ret =-1;
            }
        }
				try_recv_count++;
				rt_thread_mdelay(100);
			}while(try_recv_count<=MAX_TRY_COUNT);
    }
    return ret;

}

#define SELECT_INT_MSTIME   1500
static int cip_sock_select(gnss_device_t gsm_dev,sock_set_t read_set,int mstimeout){
    int ret =0;
    int data_ret=0;
    int j,sock_index;
    if(read_set->sock_size >MAX_SOCK_USE) read_set->sock_size=MAX_SOCK_USE;
    ret =0;
    sock_clr_flag(read_set);
    for(j=0;j<read_set->sock_size;j++){
        sock_index = read_set->sock_array[j];
        if(sock_index<0||sock_index >=MAX_SOCK_USE) continue;
        #if 1
        data_ret = cip_sock_read_select(gsm_dev,sock_index,SELECT_INT_MSTIME);
        if(data_ret>0){
            read_set->sock_rlen[j] = data_ret;
            sock_set_enable_flag(read_set,j);
            data_ret = cip_sock_select_recv(gsm_dev,sock_index,cSocketRecvBuf,sizeof(cSocketRecvBuf),data_ret);
            if(data_ret>0)
            {
                recv_data_handle(sock_index,cSocketRecvBuf,data_ret);
            }
        }else{
            read_set->sock_rlen[j] = 0;
            sock_set_disable_flag(read_set,j);
        }
        #else
        data_ret = cip_sock_recv(gsm_dev,sock_index,cSocketRecvBuf,sizeof(cSocketRecvBuf));
        if(data_ret>0)
        {
            read_set->sock_rlen[j] = len;
            sock_set_enable_flag(read_set,j);
            recv_data_handle(sock_index,cSocketRecvBuf,data_ret);
        }else{
            read_set->sock_rlen[j] = 0;
            sock_set_disable_flag(read_set,j);
        }
        #endif
    }
    if(is_socket_set_cond(read_set)){
        ret = read_set->read_flag;
        
    }
    if(data_ret<0) return -1;
    return ret;
}


static int cip_sock_recv(gnss_device_t gsm_dev,int index,void* buffer,int buffer_size){
    int ret=0;
    int can_read_bytes=0;
    int real_read_size=0;
	int try_recv_count=0;
    char send_cmd[32];
    char ack_match[20];
    rt_sprintf(send_cmd,"AT+CIPRXGET=4,%d",index%MAX_SOCK_USE);
    rt_sprintf(ack_match,"+CIPRXGET: 4,%d,",index%MAX_SOCK_USE);
    if (send_AT_CMD(gsm_dev,send_cmd,ack_match, 3000))
    {
                    //收到+CIPRXGET: 4,为正确应答，如果逗号后的数字大于0即为有数据
        if (at_cmdres_keyword_matching(ack_match))
        {
                            //截取接收的数据长度
            ret = match_recv_len(index,at_recv);
            if (ret > 0)
            {
                can_read_bytes = ret;
            }else if(ret ==0){
                return 0;
            }
        }
        else
        {
						LOG_E("recv buffer data len for  sock %d error",index%MAX_SOCK_USE);
            ret =-1;
        }
    }
    if(can_read_bytes >0){
        if(can_read_bytes < buffer_size)
        {
            real_read_size = can_read_bytes;
        }else{
            if((MAX_PACK_SIZE-20)<buffer_size){
                real_read_size = MAX_PACK_SIZE-20;
            }else{
                real_read_size = buffer_size;
            }
        }
        rt_sprintf(send_cmd,"AT+CIPRXGET=2,%d,%d",index%MAX_SOCK_USE,real_read_size);
        rt_sprintf(ack_match,"+CIPRXGET: 2,%d,",index%MAX_SOCK_USE);
				do{
        if (send_AT_CMD(gsm_dev,send_cmd,ack_match, 1000))
        {
						//收到+CIPRXGET为正确应答
			if (at_cmdres_keyword_matching(ack_match))
            {
				//截取接收的数据
                ret = match_recv_data(index,at_recv,sizeof(at_recv), buffer,buffer_size);
                if (ret < 0)
                {
					LOG_E("recv buffer data(paser) for  sock %d error",index%MAX_SOCK_USE);
                    ret =-1;
                }else{break;}
            }else{
							LOG_E("recv buffer data(match) for sock %d error",index%MAX_SOCK_USE);
							ret =-1;
            }
        }
				try_recv_count++;
				rt_thread_mdelay(100);
			}while(try_recv_count<=MAX_TRY_COUNT);
    }
    return ret;
}

int cip_paser_callback(gnss_ctx_t *ctx,int read_len,int ack_flag){
	int print_flag =1;
	int i=0;
	(void)(ctx);
	if(ack_flag>0){
		memcpy(at_recv,ctx->read_buf,read_len);
	}
	if(at_urc_keyword_matching(ctx->read_buf,"+CIPRXGET: 2,")||
		at_urc_keyword_matching((unsigned char*)at_recv,"+CIPRXGET: 2,")||
		at_urc_keyword_matching(ctx->read_buf,"+CIPRXGET: 4,")||
		at_urc_keyword_matching((unsigned char*)at_recv,"+CIPRXGET: 4,"))
	{
		print_flag = 0;
	}
	if(print_flag){
		LOG_W("parse gsm req rec_len=%d,ack_flag=%d",read_len,ack_flag);
		for(i=0;i<read_len;i++){
			if(i==0){LOG_W("data recv size=%d[",read_len);}
			LOG_RAW("%c",*(ctx->read_buf+i));
			if(i==(read_len-1)){LOG_W("]");}
		}
	}
	//+IPCLOSE: 0,
	if(at_urc_keyword_matching(ctx->read_buf,"+IPCLOSE: 0,")||
		at_urc_keyword_matching((unsigned char*)at_recv,"+IPCLOSE: 0,"))
	{
			LOG_W("socket 0 link disconnect");
			socket_flag[0] =0;
	}
	if(at_urc_keyword_matching(ctx->read_buf,"+CIPOPEN: 0,0")||
		at_urc_keyword_matching((unsigned char*)at_recv,"+CIPOPEN: 0,0")){
			LOG_W("socket 0 link connected");
			socket_flag[0] =1;			
	}else if(at_urc_keyword_matching(ctx->read_buf,"+CIPOPEN: 0,1")||
		at_urc_keyword_matching((unsigned char*)at_recv,"+CIPOPEN: 0,1"))
        {
            LOG_W("socket 0 link connected failed");
            socket_flag[0] =2;
        }
    //+IPCLOSE: 1,
    if(at_urc_keyword_matching(ctx->read_buf,"+IPCLOSE: 1,")||
		at_urc_keyword_matching((unsigned char*)at_recv,"+IPCLOSE: 1,"))
	{
			LOG_W("socket 1 link disconnect");
			socket_flag[1] =0;
	}
	if(at_urc_keyword_matching(ctx->read_buf,"+CIPOPEN: 1,0")||
		at_urc_keyword_matching((unsigned char*)at_recv,"+CIPOPEN: 1,0")){
			LOG_W("socket 1 link connected");
			socket_flag[1] =1;			
	}else if(at_urc_keyword_matching(ctx->read_buf,"+CIPOPEN: 1,1")||
		at_urc_keyword_matching((unsigned char*)at_recv,"+CIPOPEN: 1,1"))
        {
            LOG_W("socket 1 link connected failed");
            socket_flag[1] =2;
        }
    //+IPCLOSE: 2,
    if(at_urc_keyword_matching(ctx->read_buf,"+IPCLOSE: 2,")||
		at_urc_keyword_matching((unsigned char*)at_recv,"+IPCLOSE: 2,"))
	{
			LOG_W("socket 2 link disconnect");
			socket_flag[2] =0;
	}
	if(at_urc_keyword_matching(ctx->read_buf,"+CIPOPEN: 2,0")||
		at_urc_keyword_matching((unsigned char*)at_recv,"+CIPOPEN: 2,0")){
			LOG_W("socket 2 link connected");
			socket_flag[2] =1;			
	}else if(at_urc_keyword_matching(ctx->read_buf,"+CIPOPEN: 2,1")||
		at_urc_keyword_matching((unsigned char*)at_recv,"+CIPOPEN: 2,1"))
        {
            LOG_W("socket 2 link connected failed");
            socket_flag[2] =2;
        }
    if(at_urc_keyword_matching(ctx->read_buf,"+CIPRXGET: 1,0"))
    {
        //check data recv for paser
        socket_recv_flag[0]=1;
    }else if(at_urc_keyword_matching(ctx->read_buf,"+CIPRXGET: 1,1"))
    {
        //check data recv for paser
        socket_recv_flag[1]=1;
    }else if(at_urc_keyword_matching(ctx->read_buf,"+CIPRXGET: 1,2"))
    {
        //check data recv for paser
        socket_recv_flag[2]=1;
    }
    
	if(at_urc_keyword_matching(ctx->read_buf,"+CCLK: "))
		{
			parse_cclk((char*)ctx->read_buf);
		}else if(at_urc_keyword_matching((unsigned char*)at_recv,"+CCLK: ")){
			parse_cclk(at_recv);
		}

	return GNSS_ACK_OK;
}


//用户API
int sys_get_socket_id(int index){
    if(socket_flag[index%MAX_SOCK_USE] >1) return 0;
    return socket_flag[index%MAX_SOCK_USE];
}
int sys_socket_send(int index,void* data,int data_size){
    if(cip_dev==NULL){ return 0;}
	LOG_I("gb905 send data,len=%d",data_size);
	//debug_print_block_data(0,data,data_size);
    return cip_sock_send(cip_dev,index,data,data_size);
}
#define AUTH_SOCK_INDEX 2
void auth_socket_init(char* host,int port){
    cb_init_static(&gsm_recvbuff_mgr[AUTH_SOCK_INDEX],paser_buffer,sizeof(paser_buffer));
    cip_sock_open(cip_dev,AUTH_SOCK_INDEX,host,port);
}

int auth_socket_send(void* data,int data_size){
    if(cip_dev==NULL){ return 0;}
    LOG_I("auth send data,len=%d",data_size);
    debug_print_block_data(0,data,data_size);
    return cip_sock_send(cip_dev,AUTH_SOCK_INDEX,data,data_size);
}

void auth_socket_exit(){
    cb_deinit_static(&gsm_recvbuff_mgr[2]);
}

void check_auth_socket_reconnect(char* host,int port){
    if(cip_dev==NULL){ return;}
    if(socket_flag[2]<=0){
        cip_sock_close(cip_dev,AUTH_SOCK_INDEX);
        rt_thread_mdelay(150);
        cip_sock_open(cip_dev,AUTH_SOCK_INDEX,host,port);
		rt_thread_mdelay(500);
    }
}

int auth_recv_check(unsigned int timeoutms)
{
    int ret=0;
    unsigned int start_ms_tick=0;
    if(cip_dev==NULL){ return 0;}
    start_ms_tick = OSTimeGetMS();
    do{
    ret = cip_sock_recv(cip_dev,AUTH_SOCK_INDEX,cSocketRecvBuf,sizeof(cSocketRecvBuf));
		if(ret>0){
					recv_data_handle(AUTH_SOCK_INDEX,cSocketRecvBuf,ret);
					rt_thread_mdelay(50);
		}else if(ret ==0){
					rt_thread_mdelay(100);
			}else{
					rt_thread_mdelay(250);
			}
    }while(OSTimeGetMS()-start_ms_tick<=timeoutms);

    return ret;
}

int auth_protocol_anaylze(unsigned char index,unsigned char * buf,int len)
{
    return len;
}

#define HTTP_SOCK_INDEX 2
void http_socket_init(char * host,short port){
    cip_sock_open(cip_dev,HTTP_SOCK_INDEX,host,port);
}

int http_socket_send(void* data,int data_size){
    if(cip_dev==NULL){ return 0;}
    LOG_I("http send data,len=%d",data_size);
    debug_print_block_data(0,data,data_size);
    return cip_sock_send(cip_dev,HTTP_SOCK_INDEX,data,data_size);
}

int http_sock_recv(unsigned char* temp_buff,int max_len)
{
    int ret=0;
    unsigned int start_ms_tick=0;
    int timeoutms = 10000;
    if(cip_dev==NULL){ return 0;}
    start_ms_tick = OSTimeGetMS();
    do{
        ret = cip_sock_recv(cip_dev,HTTP_SOCK_INDEX,cSocketRecvBuf,MIN(sizeof(cSocketRecvBuf),max_len));
        if(ret>0){
            break;
        }else if(ret ==0){
			rt_thread_mdelay(10);
        }else{
                rt_thread_mdelay(50);
        }
     }while(OSTimeGetMS()-start_ms_tick<=timeoutms);
		if(ret>0){
			memcpy(temp_buff,cSocketRecvBuf,MIN(ret,max_len));
		}
    return MIN(ret,max_len);
}

void http_socket_exit(){
    cip_sock_close(cip_dev,HTTP_SOCK_INDEX);
}

static void ntp_retry(gnss_device_t gsm_dev){	
		//send_AT_CMD(gsm_dev,"AT+CNTP=\"202.120.2.101\",32","OK", 3000);
	send_AT_CMD(gsm_dev,"AT+CNTP=\"ntp.aliyun.com\",32","OK", 3000);
		send_AT_CMD(gsm_dev,"AT+CNTP","OK", 1000);
}


void update_rtc(unsigned long utc_time){
		rt_device_t rtc_dev;
		static uint8_t time_fixed_flag=0;
		time_t now;
		rtc_dev = rt_device_find("rtc");
		rt_device_control(rtc_dev, RT_DEVICE_CTRL_RTC_GET_TIME, &now);
		LOG_W("read from rtc:");
		debug_utc(now);
		LOG_W("read from net:");
		debug_utc(utc_time);
		if(time_fixed_flag==0||(utc_time >now && utc_time - now >= 30)||
			(now > utc_time && now - utc_time >=30 )){
				//debug_utc(utc_time);
				if(time_fixed_flag==0){
					LOG_W("first sync from net:");
					time_fixed_flag=1;
				}else{
					LOG_W("sync from net:");
				}
				rt_device_control(rtc_dev, RT_DEVICE_CTRL_RTC_SET_TIME, &utc_time);	
				rt_thread_mdelay(20);
				rt_device_control(rtc_dev, RT_DEVICE_CTRL_RTC_GET_TIME, &now);
				LOG_W("read from rtc:");
				debug_utc(now);
			}
}

//+CCLK: "24/06/07,15:42:57+32"
static void parse_cclk(char* str)
{
		char time_buf[24];
		char tmp[5]={0};
		char* p=NULL;char* q =NULL;
		int time_len=0;
		int zone;
        unsigned long utc_net=0;
        DateTime datetime_net;
		memset(tmp,0,sizeof(tmp));
		
		p = strstr(str,"+CCLK: ");
		if(p==NULL){return;}
		p= p+7;
		q=p;
		p = strstr(q,"\"");
        if(p==NULL){return;}
		p=p+1;
		q=p+1;
		q = strstr(q,"\"");
        if(p==NULL){return;}
		time_len = q-p;
		if(time_len<sizeof(time_buf)){
			memcpy(time_buf,p,time_len);
			time_buf[time_len]='\0';
		}
		p=time_buf;
		q = strstr(p,"/");
        if(q==NULL){return;}
		memcpy(tmp,p,q-p);
		datetime_net._year = atoi(tmp)+2000;
		memset(tmp,0,sizeof(tmp));
		p = q+1;
		q=strstr(p,"/");
        if(q==NULL){return;}
		memcpy(tmp,p,q-p);
		datetime_net._month = atoi(tmp);
		memset(tmp,0,sizeof(tmp));
		p = q+1;
		q=strstr(p,",");
        if(q==NULL){return;}
		memcpy(tmp,p,q-p);
		datetime_net._day = atoi(tmp);
		memset(tmp,0,sizeof(tmp));
		p = q+1;
		q=strstr(p,":");
        if(q==NULL){return;}
		memcpy(tmp,p,q-p);
		datetime_net._hour = atoi(tmp);
		memset(tmp,0,sizeof(tmp));
		p = q+1;
		q=strstr(p,":");
        if(q==NULL){return;}
		memcpy(tmp,p,q-p);
		datetime_net._minute = atoi(tmp);
		memset(tmp,0,sizeof(tmp));
		p = q+1;
		q=strstr(p,"+");
		if(q==NULL){q=strstr(p,"-");}
        if(q==NULL){return;}
		memcpy(tmp,p,q-p);
		datetime_net._second = atoi(tmp);
		memset(tmp,0,sizeof(tmp));
		p = q+1;
		memcpy(tmp,p,2);
		
		zone = atoi(tmp);

		if(zone!=0|| (datetime_net._year>2020 && datetime_net._year!=2070)){
			//set localtime
			utc_net = makeDatetimeUTC(datetime_net);
			update_rtc(utc_net);
		}else{
			cclk_fail++;
		}
}

void try_check_ntp(gnss_device_t gsm_dev,uint8_t flag)
{
    if(flag||cclk_fail>3){
        cclk_fail=0;
        ntp_retry(gsm_dev);
        rt_thread_mdelay(50);
    }else{
        send_AT_CMD(gsm_dev,"AT+CCLK?","OK", 1000);
        rt_thread_mdelay(50);
    }
}

