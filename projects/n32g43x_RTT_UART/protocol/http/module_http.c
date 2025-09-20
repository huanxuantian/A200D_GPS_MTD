
//#define		DEBUG_Y
#include <rtthread.h>

#include "utils.h"
#include <system_bsp.h>

#include "cip_sockmgr.h"

#include <protocol.h>
#include "board.h"
#include <business.h>



//默认的块大小
#define		DEFAULT_HTTP_BLOCK_SIZE		512

//默认接收数据超时
#define		DEFAULT_HTTP_RECV_TIMEOUT	30

//默认任务有效时间
#define		DEFAULT_HTTP_PERIOD			1200 //
//MAX_RCV_BUFF_SIZE必须大于等于MAX_PASER_BUFF_SIZE的两倍
//MAX_PASER_BUFF_SIZE必须大于底层最大单包数据大小
//接收BUFF的大小，单次接收的缓存，缓存会直接移交给CB_buff
#define MAX_RCV_BUFF_SIZE	(1024) //HTTP need 1k in less for head parse

#define MAX_PASER_BUFF_SIZE	(512)

//重试的超时时间
#define HTTP_UPDAET_WAIT_SOCKET_RECONNECT_GAP 60

//接收数据的超时
#define HTTP_UPDATE_MAX_SOCKET_SELECT_TIMEOUT 8

typedef struct{
	unsigned char * raw_buf;
	int raw_len;
	unsigned char * buf;
	int len;
}__packed http_buff_mgr_t;


static http_task_t system_http_task;

CircleBufferMngr update_read_buff;	
static unsigned char* recv_data_buff;//[MAX_RCV_BUFF_SIZE]={0};
static unsigned char parser_buff[MAX_PASER_BUFF_SIZE];


http_task_t* get_http_task()
{
	return &system_http_task;
}

int set_http_task(http_task_t* task)
{
	memcpy(&system_http_task,task,sizeof(http_task_t));
	//need save to file
	return (int)sizeof(http_task_t);
}


int http_update_info_save(http_task_t *task_info)
{
	//save info here
    return 1;
}

int clear_http_task()
{
    memset(&system_http_task,0,sizeof(http_task_t));
    http_update_info_save(&system_http_task);
    return 0;
}
#ifdef DEBUG_UPDATE
void handle_update_flag(){
	rt_size_t ret=0;
	spi_update_flag* ptr_update_flag;
	rt_device_t storage_dev;
	uint32_t addr=FLASH_UPDATE_START;
	uint8_t _buff[128];
	char version[36];
	memset(_buff,0,sizeof(_buff));
	memset(version,0,sizeof(version));
	storage_dev = rt_device_find("Flash");
	if(storage_dev!=RT_NULL){
		ret = rt_device_read(storage_dev,FLASH_UPDATE_START,_buff,sizeof(_buff));
		LOG_I("read update flag data:");
		debug_print_block_data(FLASH_UPDATE_START,_buff,sizeof(_buff));
		memset(_buff,0,sizeof(_buff));
		ret = rt_device_read(storage_dev,FLASH_UPDATE_WBACKUP_START,_buff,sizeof(_buff));
		LOG_I("read update back flag data:");
		debug_print_block_data(FLASH_UPDATE_WBACKUP_START,_buff,sizeof(_buff));
		ptr_update_flag = (spi_update_flag*)_buff;
		#ifdef TEST_UPDATE
		#define TEST_FILE_FUSE_SIZE 0x019080
		if(ptr_update_flag->start_flag==0xFFFF){
			ptr_update_flag->start_flag=UPDATA_START_FLAG_OK;
			ptr_update_flag->device_type =0;
			ptr_update_flag->update_type = 1;
			ptr_update_flag->file_size = TEST_FILE_FUSE_SIZE;
			rt_device_control(storage_dev, RT_CMD_W25_ERASE_SECTOR, &addr);
			rt_thread_mdelay(20);
			ret = rt_device_write(storage_dev,addr,_buff,sizeof(_buff));
			rt_thread_mdelay(20);
			ret = rt_device_read(storage_dev,FLASH_UPDATE_START,_buff,sizeof(_buff));
			debug_print_block_data(FLASH_UPDATE_START,_buff,sizeof(_buff));
		}
		#endif
		memset(_buff,0,sizeof(_buff));
		ret = rt_device_read(storage_dev,FLASH_PARAM_START,_buff,sizeof(_buff));
		LOG_I("read param data:(ret=%d)",ret);
		debug_print_block_data(FLASH_PARAM_START,_buff,sizeof(_buff));
		memset(_buff,0,sizeof(_buff));
		ret = rt_device_read(storage_dev,(FLASH_PARAM_START+0x3000),_buff,sizeof(_buff));
		LOG_I("read param backup data:(ret=%d)",ret);
		debug_print_block_data((FLASH_PARAM_START+0x3000),_buff,sizeof(_buff));

		ret = rt_device_read(storage_dev,FLASH_UPDATE_DATA_START+FLASH_VER_OFFSET,_buff,sizeof(_buff));
		memcpy(version,_buff,32);
		LOG_I("read update app flash VER:%s",version);
		debug_print_block_data(FLASH_UPDATE_DATA_START+FLASH_VER_OFFSET,_buff,sizeof(_buff));
		ret = rt_device_read(storage_dev,FLASH_UPDATE_START,_buff,sizeof(_buff));
		if(ret>0)
		{
			ptr_update_flag = (spi_update_flag*)_buff;
			if(ptr_update_flag->file_size >4096 && ptr_update_flag->file_size < FLASH_UPDATE_DATA_APP_SIZE)
			{
				memset(_buff,0,sizeof(_buff));
				ret = rt_device_read(storage_dev,FLASH_UPDATE_DATA_START+ptr_update_flag->file_size-sizeof(_buff),_buff,sizeof(_buff));
				debug_print_block_data(FLASH_UPDATE_DATA_START+ptr_update_flag->file_size-sizeof(_buff),_buff,sizeof(_buff));
			}
		}

		
	}

}
#endif

int http_update_info_init(void)
{
	#ifdef DEBUG_UPDATE
	handle_update_flag();
	#endif
	//init load and check_update info,call before update
	memset(&system_http_task,0,sizeof(http_task_t));
    return 0;
}
INIT_APP_EXPORT(http_update_info_init)

//通用初始化文件（删除并创建空文件,SPI-flash 按照预分配擦除对应的page）
static void initupdatedata(char* fname)
{
	uint32_t rAddress=0;
	uint32_t rSize=0;
	uint32_t sectoraddr;
	uint16_t sectorCount=0;
	int32_t i;
	rt_device_t storage_dev;
	u8 update_flag_buff[FLASH_UPDATE_LEN];
	spi_update_flag* ptr_update_flag;
	rt_size_t ret=0;
	//inin data only
	DbgWarn("init data for http download[FILE=%s]\r\n",fname);
	//check fname get download spi start address 
	//erase spi download area here
	if(rt_strcmp(UPDATE_SHINKI_APP_PATH,fname)==0){
		rAddress = FLASH_UPDATE_DATA_START;
		rSize = FLASH_UPDATE_DATA_APP_SIZE;
	}
	if(rSize==0||rAddress < FLASH_UPDATE_START_ADDRESS)
	{
		return;
	}
	storage_dev = rt_device_find("Flash");
	if(storage_dev==RT_NULL){
		DbgError("flash not found!\r\n");
		return;
	}
	sectoraddr = ((int)(rAddress/SECTOR_SIZE))*SECTOR_SIZE;
	sectorCount = (rSize/SECTOR_SIZE) +(rSize%SECTOR_SIZE?1:0);
	#if 1
	for(i=0;i<sectorCount;i++)
	{		
		rt_device_control(storage_dev, RT_CMD_W25_ERASE_SECTOR, &sectoraddr);
		sectoraddr+=SECTOR_SIZE;
	}
	rt_thread_mdelay(5);
	#endif
	//写入擦除更新升级标记区数据
	memset(update_flag_buff,0,sizeof(update_flag_buff));
	ptr_update_flag = (spi_update_flag*)update_flag_buff;
	ptr_update_flag->start_flag=UPDATA_START_FLAG_OK;
	ptr_update_flag->end_flag =0xFF;//default flag
	ptr_update_flag->sucess = 0xFF;//default flag
	ptr_update_flag->device_type =0;
	ptr_update_flag->update_type = 1;//0->IAP 1:APP_HTTP/APP_FTP
	ptr_update_flag->file_size = 0;
	//FLASH_UPDATE_START
	sectoraddr =  ((int)(FLASH_UPDATE_START/SECTOR_SIZE))*SECTOR_SIZE;
	rt_device_control(storage_dev, RT_CMD_W25_ERASE_SECTOR, &sectoraddr);
	rt_thread_mdelay(20);
	ret = rt_device_write(storage_dev,FLASH_UPDATE_START,update_flag_buff,sizeof(update_flag_buff));
	if(ret!=sizeof(update_flag_buff))
	{
		DbgError(" err flag for http download[FILE=%s]\r\n",fname);
	}
	DbgWarn("init flag for http download[FILE=%s]\r\n",fname);
}

//通用保存
static int saveupdatedata(char* fname,int offset,unsigned char* fdata,int datalen)
{
	uint32_t rAddress=0;
	uint32_t rSize=0;
	//uint32_t sectoraddr;
	rt_device_t storage_dev;
	rt_size_t ret=0;
    //save data for offset
	DbgWarn("save data for http download[FILE=%s],offset=%d,size=%d\r\n",fname,offset,datalen);
	//debug_print_block_data(offset,fdata,datalen);
	//check fname get download spi start address 
	//calc data address to check and write data to spi flash（not earse） here
	if(rt_strcmp(UPDATE_SHINKI_APP_PATH,fname)==0){
		rAddress = FLASH_UPDATE_DATA_START+offset;
		rSize = FLASH_UPDATE_DATA_APP_SIZE-offset;
	}
	if(rSize==0||rAddress < FLASH_UPDATE_START_ADDRESS|| rSize < datalen)
	{
		return 0;
	}
	storage_dev = rt_device_find("Flash");
	if(storage_dev==RT_NULL){
		DbgError("flash not found!\r\n");
		return -1;
	}
	#if 0
	if((offset/SECTOR_SIZE)!=(offset+datalen)/SECTOR_SIZE || 
	(offset%SECTOR_SIZE)==0 || (offset+datalen)%SECTOR_SIZE ==0 )
	{
		sectoraddr =  ((int)((FLASH_UPDATE_START+(offset+datalen))/SECTOR_SIZE))*SECTOR_SIZE;
		rt_device_control(storage_dev, RT_CMD_W25_ERASE_SECTOR, &sectoraddr);
		rt_thread_mdelay(20);
	}
	
	for(sectoraddr = FLASH_UPDATE_START+offset;sectoraddr < FLASH_UPDATE_START+(offset+datalen);sectoraddr++)
	{
		if(sectoraddr%SECTOR_SIZE==0){
			rt_device_control(storage_dev, RT_CMD_W25_ERASE_SECTOR, &sectoraddr);
		}
	}
	rt_thread_mdelay(20);
	#endif
	ret = rt_device_write(storage_dev,rAddress,fdata,datalen);
	if(ret<=0){return -2;}
    return ret;
}

int check_http_updata(){
    http_task_t* cur_task;

    cur_task = get_http_task();

    if(cur_task->state!=TASK_STATE_IDLE)
    {
        DbgWarn("have task process discard it\r\n");
        return 1;
    }
    return 0;
}

/*
 * NAME:new_add_http_update_task
 * PARAM:
*/
int new_add_http_update_task(unsigned char index,char* url,int save_block_size,char* save_path,int recv_data_timeout,int task_vaidity_period,int task_type)
{
	DbgFuncEntry();
	bool ret=0;
	
	time_t cur_time;
	http_task_t* cur_task;

	http_task_t new_task;

	cur_task = get_http_task();

	if(cur_task->state!=TASK_STATE_IDLE)
	{
		DbgWarn("have task process discard it\r\n");
		return -1;
	}
	if(strlen(url)>=sizeof(cur_task->url))
	{
		DbgWarn("url too long \r\n");
		return -2;
	}
	if(strstr(url,"http")==NULL)
	{
		DbgWarn("url error \r\n");
		return -2;	
	}
	if(!save_path||strlen(save_path)>sizeof(new_task.fname))
	{
		//file save info error
		return -3;
	}
	if(save_block_size<=0|| save_block_size > MIN(MAX_RCV_BUFF_SIZE,DEFAULT_HTTP_BLOCK_SIZE))
	{
		save_block_size = DEFAULT_HTTP_BLOCK_SIZE;//32k in default
	}
	if(recv_data_timeout<=0)
	{
		recv_data_timeout = DEFAULT_HTTP_RECV_TIMEOUT;//30 second in default
	}
	if(task_vaidity_period<=0)
	{
		task_vaidity_period = DEFAULT_HTTP_PERIOD;//1 day for task
	}
	

	memset(&new_task,0,sizeof(http_task_t));
	set_http_task(&new_task);//clear before
	memcpy(new_task.url,url,strlen(url));
	
	//time(&cur_time);
	cur_time = OSTimeGet();
	new_task.save_block_size = save_block_size;//
	memcpy(new_task.fname,save_path,strlen(save_path));
	new_task.state = TASK_STATE_WAIT_START;
	new_task.totallen =0;//wait for http to set totallen
	new_task.save_offset =0;//offset need zeor for new task
	new_task.task_period_time = cur_time + task_vaidity_period;

	new_task.fetch_timeout = recv_data_timeout;//
	
	new_task.task_type = task_type;
	new_task.task_index = index;

	set_http_task(&new_task);//set after
	http_update_info_save(&new_task);
	//mark http_task start here
	DbgFuncExit();
	return ret;
}



int send_http_req(http_task_t* task,http_task_state_t info)
{
	DbgFuncEntry();

	int ret;
	http_get_req_head_t req_head;
	//char send_buffer[2048]={0};
	char send_buffer[256];

	memset(send_buffer,0,sizeof(send_buffer));

	req_head.host_target = info.host;
	req_head.req_path =task->url;
	req_head.accect_patterns = "";
	req_head.connect_mode = NULL;
	req_head.user_agent = NULL;

	http_build_get_msg(req_head,send_buffer,task->save_offset,task->save_offset);
	//send to socket
	DbgWarn("REQ:%s",send_buffer);
	ret = new_http_update_socket_send((unsigned char*)send_buffer,strlen(send_buffer));
	if(!ret)
	{
		task->state = TASK_STATE_STARTED;
	}

	DbgFuncExit();
	return ret;
}

void http_mcu_update_finish(unsigned char index,char* fname,unsigned int fsize)
{
	#if 1
	rt_size_t ret=0;
	uint32_t sectoraddr;
	rt_device_t storage_dev;
	u8 update_flag_buff[FLASH_UPDATE_LEN];
	spi_update_flag* ptr_update_flag;
	memset(update_flag_buff,0,sizeof(update_flag_buff));
	storage_dev = rt_device_find("Flash");
	if(storage_dev==RT_NULL){
		DbgError("flash not found!\r\n");
		return;
	}
	ret = rt_device_read(storage_dev,FLASH_UPDATE_START,update_flag_buff,sizeof(update_flag_buff));
	ptr_update_flag = (spi_update_flag*)update_flag_buff;
	if(ptr_update_flag->start_flag!=UPDATA_START_FLAG_OK)
	{
		//init flag first
		ptr_update_flag->start_flag=UPDATA_START_FLAG_OK;
		ptr_update_flag->device_type =0;
		ptr_update_flag->update_type = 1;//0->IAP 1:APP_HTTP/APP_FTP
	}
	ptr_update_flag->file_size = fsize;
	ptr_update_flag->end_flag =UPDATA_END_FLAG_FI;//OK flag
	ptr_update_flag->sucess = 0xFF;//default flag

	sectoraddr =  ((int)(FLASH_UPDATE_START/SECTOR_SIZE))*SECTOR_SIZE;
	rt_device_control(storage_dev, RT_CMD_W25_ERASE_SECTOR, &sectoraddr);
	rt_thread_mdelay(20);
	ret = rt_device_write(storage_dev,FLASH_UPDATE_START,update_flag_buff,sizeof(update_flag_buff));
	if(ret!=sizeof(update_flag_buff))
	{
		DbgError(" err flag for http download[FILE=%s]\r\n",fname);
	}
	sectoraddr =  ((int)(FLASH_UPDATE_WBACKUP_START/SECTOR_SIZE))*SECTOR_SIZE;
	rt_device_control(storage_dev, RT_CMD_W25_ERASE_SECTOR, &sectoraddr);
	DbgWarn("update flag for http download[FILE=%s]\r\n",fname);
	#endif
	//reset system
	rt_system_reset();

}

int http_callback_util(http_task_t* task)
{
	DbgFuncEntry();
	//task will clear when return 0
 	DbgWarn("callback FINISH\r\nURL:%s\r\nFILE:%s,len:%d,type:%d\r\n",
  	task->url,task->fname,task->totallen,task->task_type);
	switch(task->task_type)
	{
		case UPDATE_SHINKI_APP:
			http_mcu_update_finish(task->task_index,task->fname,task->totallen);
			break;
		default:
			break;
	}
	
  DbgFuncExit();
  return 0;
}



static bool find_http_head_sta_str(unsigned char *buf, int len,unsigned char * offset)
{
    char* finder1;

    bool ret = false;
    *offset = 0;
    finder1=strstr((char*)buf,HTTP_HEAD_STA_STR);
    if(finder1)
    {
        *offset =(finder1-(char*)buf);
        ret = true;
    }
    return ret;
}



static bool find_http_head_end_str(unsigned char *buf, int len,unsigned char * offset)
{
    char* finder1;
    bool ret = false;
    *offset = 0;
    finder1=strstr((char*)buf,HTTP_HEAD_END_STR);
    if(finder1)
    {
        *offset =(finder1-(char*)buf);
        ret = true;
    }
    return ret;
}

static int get_a_full_http_head(http_buff_mgr_t *msg)
{
    bool ret;
	unsigned char head_offset = 0,tail_offset = 0;
	int offset;
	DbgFuncEntry();
	ret = find_http_head_sta_str(msg->raw_buf,msg->raw_len,&head_offset);
	if(ret && (msg->raw_len - head_offset > strlen(HTTP_HEAD_STA_STR)+strlen(HTTP_HEAD_END_STR)))
	{
		ret = find_http_head_end_str(msg->raw_buf + head_offset + strlen(HTTP_HEAD_STA_STR),msg->raw_len - head_offset,&tail_offset);
		tail_offset += head_offset + strlen(HTTP_HEAD_END_STR);
		if(ret)
		{
			DbgPrintf("head_offset = %d\r\n",head_offset);
			DbgPrintf("tail_offset = %d\r\n",tail_offset);

			msg->buf = (msg->raw_buf + head_offset);
			msg->len = tail_offset - head_offset + strlen(HTTP_HEAD_END_STR);
			offset = tail_offset + strlen(HTTP_HEAD_END_STR);
		}
		else
		{
			offset = head_offset;
		}
	}
	else
	{
		offset = head_offset;
	}
	DbgFuncExit();
	return offset;
}




static int new_get_http_content(http_pesponse_head_t* head,http_buff_mgr_t* msg)
{

	int offset =0;
	int writen_len=0;
	int datalen=0;
	http_task_t* task = NULL;
	DbgFuncEntry();
	task = get_http_task();
	if(head->status_code!=200&&head->status_code!=206)
	{
		return (head->content_offset+datalen);
	}
	unsigned char* data_pos = msg->buf+head->content_offset;
	if(task->save_offset==0)
	{
	initupdatedata(task->fname);
	}
	if(head->status_code==200)//200 ref file length,206 ref to pack_len 
	{
		task->totallen = head->content_length;
		http_update_info_save(task);
	}
	else if(head->status_code==206)
	{
		if(task->totallen != head->content_length)
		{
			//clear task here
			clear_http_task();
			return (head->content_offset+datalen);
		}
		//task->save_offset = head->content_offset;
	}
	
	

	

	if(task->save_offset+(msg->raw_len-(data_pos-msg->raw_buf))>=task->totallen)
	{
		//孤立包数据，完成接收，排除冗余数据
		datalen = task->totallen-task->save_offset;

	}
	else
	{
		//数据未结束，需要后续数据接收
		datalen = msg->raw_len-(data_pos-msg->raw_buf);
	}
	DbgWarn("[START]DOWNLOADED:%d/%d,LEN=%d\r\n",task->save_offset,task->totallen,datalen);
	DbgPrintf("data_offset = %d\r\n",head->content_offset);
	DbgPrintf("data_lens = %d\r\n",datalen);
	#if 0
	{
		int i;

		DbgPrintf("data_offset = %d\r\n",head->content_offset);
		DbgPrintf("data_lens = %d\r\n",datalen);

		for(i=0;i<datalen;i++)
		{
			DbgPrintf("data_buf[%d] = 0x%2x\r\n",i,data_pos[i]);
		}
	}
	#endif
	writen_len=saveupdatedata(task->fname,task->save_offset,data_pos,datalen);
	if(writen_len<datalen)
	{
		task->state =TASK_STATE_ERROR;
	}
	else
	{
		task->state =TASK_STATE_RECIVING;
		//http_update_info_save(task);
	}
	if(task->save_offset>=task->totallen)
	{
		task->state =TASK_STATE_FINISH;
		//push finsh handle should block in and reset current_task
		//http_update_info_save(task);
	}
	else
	{
		task->state =TASK_STATE_RECIVING;
		//http_update_info_save(task);
	}
	task->save_offset+=datalen;
	http_update_info_save(task);
	//check_downloadpresend();
	offset =head->content_offset+datalen;
	DbgFuncExit();
	return offset;
}

int new_pending_http_content(unsigned char* data_pos,int raw_len)
{
	int offset;
	int writen_len=0;
	int datalen=0;
	http_task_t* task;
	task = get_http_task();

	if(task->save_offset+raw_len>=task->totallen)
	{
		//孤立包数据，完成接收，排除冗余数据
		datalen = task->totallen-task->save_offset;

	}
	else
	{
		datalen = raw_len;//整包数据

	}
	DbgWarn("[CONTINUE]DOWNLOADED:%d/%d-LEN=%d\r\n",task->save_offset,task->totallen,datalen);
	#if 0
	{
		int i;
		DbgPrintf("data_lens = %d\r\n",datalen);

		for(i=0;i<datalen;i++)
		{
			DbgPrintf("data_buf[%d] = 0x%2x\r\n",i,data_pos[i]);
		}
	}
	#endif

	writen_len=saveupdatedata(task->fname,task->save_offset,data_pos,datalen);
	if(writen_len<datalen)
	{
		task->state =TASK_STATE_ERROR;
	}
	else
	{
		task->state =TASK_STATE_RECIVING;
		//http_update_info_save(task);
	}
	if(task->save_offset>=task->totallen)
	{
		task->state =TASK_STATE_FINISH;
		//push finsh handle should block in and reset current_task
		//http_update_info_save(task);
	}
	else
	{
		task->state =TASK_STATE_RECIVING;
		//http_update_info_save(task);
	}
	task->save_offset+=datalen;
	http_update_info_save(task);
	//check_downloadpresend();
	offset =datalen;
	return offset;
}



int new_http_protocol_ayalyze(unsigned char * buf,int len)
{
	unsigned char * msg_buf =NULL;
	int msg_len;
	int offset=0;
	http_buff_mgr_t msg;
	http_pesponse_head_t head;

	http_task_t* task=NULL;

	DbgFuncEntry();
	task = get_http_task();
	msg_buf = buf;
	msg_len = len;

	do{
		memset(&msg,0x00,sizeof(msg));

		msg.raw_buf = msg_buf;
		msg.raw_len = msg_len;
		
		
		if(task->state ==TASK_STATE_STARTED)
		{
			offset = get_a_full_http_head(&msg);

			if(offset && msg.len)
			{
				head = parse_http_header((const char *)msg.buf);
				#if 0
				{
					DbgPrintf("head_lens = %d\r\n",msg.len);

					char head_str[128]={0};
					memcpy(head_str,msg.buf,127);
					DbgGood(head_str);
				}
				#endif
				
				offset= new_get_http_content(&head,&msg);//
				//handle msg data
				//处理文件创建，第一包数据的保存和初始化接收设置
			}
		}
		else
		{
			//no save here

		}

		msg_buf += offset;
		msg_len -= offset;
	}while(offset && msg.raw_len && msg_len > 0);

	return (len - msg_len);
	DbgFuncExit();

}

int check_http_task()//main process for http
{
	bool ret;
	int ret_len;
	int size,len,free_len;
	int result=0;
	time_t cur_time;
	int delta;
	//unsigned char* buff;
	
	
	http_task_t* cur_task;
	CircleBufferMngr* read_buff = NULL;	
	uint8_t* recv_buffer;
	DbgFuncEntry();
	cur_task = get_http_task();
	DbgWarn("STATE:%d,TIME:%d,DOWNLOADED:%d/%d\r\n"
		,cur_task->state,cur_task->task_period_time
		,cur_task->save_offset,cur_task->totallen);
	if(cur_task->state <=TASK_STATE_IDLE)
	{
		return -1;
	}
	#ifdef DEBUG_UPDATE
	handle_update_flag();
	#endif
	stop_modbus_thread();
	if(cur_task->state ==TASK_STATE_FINISH)
	{
		//call back here
		if(http_callback_util(cur_task)==0)
		{
			clear_http_task();
		}
		//need clean task	
	}
	else
	{
		//chechk task time period
		//time(&cur_time);
		cur_time = OSTimeGet();
		DbgWarn("CUR_TIME :%d\r\n",cur_time);
		if(cur_time>cur_task->task_period_time)
		{
			clear_http_task();
			DbgWarn("TASKTIMEOUT :%d-%d\r\n",cur_time,cur_task->task_period_time);
			restart_modbus_thread();
			return 0;
		}
		
		http_task_state_t download_info={0};
		char orgin_name[128]={0};
		
		//download_info.state = cur_task->state;//set state before
		recv_data_buff = rt_malloc(MAX_RCV_BUFF_SIZE);
		if(recv_data_buff==RT_NULL){
			DbgWarn("update recv buffer alloc error!!!!\r\n");
			restart_modbus_thread();
			return -2;
		}
		memset(recv_data_buff,0,MAX_RCV_BUFF_SIZE);
		DbgWarn("update check URL:%s\r\n",cur_task->url);
		
		parse_url_ext(cur_task->url, download_info.host, &download_info.port, orgin_name,NULL);
		//connect
		DbgWarn("update check IP:%s,PORT:%d\r\n",download_info.host, download_info.port);
		ret=new_http_update_socket_init(download_info.host, (short)download_info.port);
		if(!ret)
		{
			new_http_update_socket_deinit();
			if(recv_data_buff!=RT_NULL)
			{
				rt_free(recv_data_buff);
				recv_data_buff=NULL;
			}
			rt_thread_mdelay(200);
			restart_modbus_thread();
			return -1;
		}
		//http req send
		result = send_http_req(cur_task,download_info);
		if(result)
		{
			new_http_update_socket_deinit();
			if(recv_data_buff!=RT_NULL)
			{
				rt_free(recv_data_buff);
				recv_data_buff=NULL;
			}
			restart_modbus_thread();
			return -2;
		}
		//req_time
		//time(&download_info.req_time);
		download_info.req_time = OSTimeGet();
		recv_buffer = rt_malloc(MAX(cur_task->save_block_size,2048));
		if(recv_buffer==RT_NULL)
		{
			new_http_update_socket_deinit();
			if(recv_data_buff!=RT_NULL)
			{
				rt_free(recv_data_buff);
				recv_data_buff=NULL;
			}
			restart_modbus_thread();
			return -2;
		}

		memset(recv_buffer,0,MAX(cur_task->save_block_size,2048));
		cb_init_static(&update_read_buff,recv_buffer,MAX(cur_task->save_block_size,2048));
		read_buff = &update_read_buff;
		do{
			while(1)
			{
				//recv data
				//
				ret_len = new_http_update_socket_recv(recv_data_buff,MIN(MAX_RCV_BUFF_SIZE,cur_task->save_block_size));
				if(ret_len<0)
				{	
					/*
					new_http_update_socket_deinit();
					return -2;
					*/
					break;
				}
				else
				{
					//parse data
					if(cur_task->totallen>0)
					{
						DbgWarn("DOWNLOADED:%d/%d| %03d%% downloaded.\r\n",cur_task->save_offset,cur_task->totallen,cur_task->save_offset*100/cur_task->totallen);
					}
					else
					{
						DbgWarn("DOWNLOADED:%d/%d\r\n",cur_task->save_offset,cur_task->totallen);
					}
					if(cur_task->totallen >=FLASH_UPDATE_DATA_APP_SIZE)
					{
						DbgWarn("BIN FILE SIZE TOO BIG FOR THIS DEVICE!!!");
						cur_task->state=TASK_STATE_ERROR;
						break;
					}
					if(cur_task->state ==TASK_STATE_STARTED)//wait head
					{
						if (cb_write(read_buff,recv_data_buff,ret_len) != ret_len)
						{
							DbgWarn("http update socket cb read buff is discard!\r\n");
							break;
						}
					
						len = cb_datalen(read_buff);
						
						//DbgPrintf("len = %d\r\n",len);//
						if(len>0)
						{
							memset(parser_buff,0,MAX_PASER_BUFF_SIZE);
							
							
							size = cb_read_no_offset(read_buff,parser_buff,MIN(len,MAX_PASER_BUFF_SIZE));
							if(size != MIN(len,MAX_PASER_BUFF_SIZE))
							{
								DbgError("http socket send size error!(size = %d,len = %d)\r\n",size,MIN(len,MAX_PASER_BUFF_SIZE));
							}
							
							size = new_http_protocol_ayalyze(recv_data_buff,ret_len);;//TODO::need fix ,CHECK::OK
						
							cb_read_move_offset(read_buff,size);
						
							//rt_free(buff);
						}
					}
					else if(cur_task->state ==TASK_STATE_RECIVING)
					{
						free_len = cb_freelen(read_buff);
						len = cb_datalen(read_buff);
						if(free_len<ret_len|| len>=MAX_PASER_BUFF_SIZE)
						{
							//save buffer data before full
							//如果进入数据接收模式，则不解析，直接数据提交存储，直到数据全部完成，通知外部一个存储位置
							len = cb_datalen(read_buff);

							memset(parser_buff,0,MAX_PASER_BUFF_SIZE);
							
							size = cb_read_no_offset(read_buff,parser_buff,MIN(len,MAX_PASER_BUFF_SIZE));
							if(size != MIN(len,MAX_PASER_BUFF_SIZE))
							{
								DbgError("http socket send size error!(size = %d,len = %d)\r\n",size,MIN(len,MAX_PASER_BUFF_SIZE));
							}
							
							size = new_pending_http_content(parser_buff,size);
							cb_read_move_offset(read_buff,size);
							//rt_free(buff);
						}
						len = cb_datalen(read_buff);
						if(cur_task->totallen>0&&cur_task->save_offset+len>=cur_task->totallen)
						{
							//only save this pack and finish
							do{
								memset(parser_buff,0,MAX_PASER_BUFF_SIZE);
								
								size = cb_read_no_offset(read_buff,parser_buff,MIN(cur_task->totallen-cur_task->save_offset,MAX_PASER_BUFF_SIZE));
								if(size != MIN(cur_task->totallen-cur_task->save_offset,MAX_PASER_BUFF_SIZE))
								{
									DbgError("http socket send size error!(size = %d,len = %d)\r\n",size,len);
								}
								
								size = new_pending_http_content(parser_buff,size);
								cb_read_move_offset(read_buff,size);
								len = cb_datalen(read_buff);
							}while(cur_task->save_offset<cur_task->totallen);
							//rt_free(buff);						
							//cb_read_move_offset(read_buff,size);
							LOG_W("last pack save1 detect");
							break;
						}
						else
						{
							//put data to buffer
							if (cb_write(read_buff,recv_data_buff,ret_len) != ret_len)
							{
								DbgWarn("http update socket cb read buff is discard!\r\n");
								break;
							}
							len = cb_datalen(read_buff);
							if(cur_task->totallen>0&&cur_task->save_offset+len>=cur_task->totallen)
							{
								//only save this pack and finish
								do{

									memset(parser_buff,0,MAX_PASER_BUFF_SIZE);
									
									size = cb_read_no_offset(read_buff,parser_buff,MIN(cur_task->totallen-cur_task->save_offset,MAX_PASER_BUFF_SIZE));
									if(size != MIN(cur_task->totallen-cur_task->save_offset,MAX_PASER_BUFF_SIZE))
									{
										DbgError("http socket send size error!(size = %d,len = %d)\r\n",size,len);
									}
									
									size = new_pending_http_content(parser_buff,size);
									cb_read_move_offset(read_buff,size);
									len = cb_datalen(read_buff);
								}while(cur_task->save_offset<cur_task->totallen);
								//rt_free(buff);						
								//cb_read_move_offset(read_buff,size);
								LOG_W("last pack save0 detect");
								break;
							}
						}
						
					}			
					//if timeout breaks
					//time(&cur_time);
					cur_time = OSTimeGet();
					if(ret_len!=0)
					{
						download_info.last_rcv_time=cur_time;
					}
					if(download_info.last_rcv_time)
					{
						delta = cur_time - download_info.last_rcv_time;//difftime(cur_time,download_info.last_rcv_time);
						if(delta<0)
						{
							delta =0-delta;
						}
						if(delta>cur_task->fetch_timeout)
						{
							//
							break;
						}
					}
					else
					{	
						//ever no data rcv here
						//delta = difftime(cur_time,download_info.req_time);
						delta = cur_time - download_info.req_time;
						if(delta<0)
						{
							delta =0-delta;
						}
						if(delta>cur_task->fetch_timeout)
						{
							//
							break;
						}	
					}

				}
				
				
			}
			if(cur_task->totallen&&cur_task->totallen<=cur_task->save_offset)
			{
				DbgWarn("DOWNLOADED:%d/%d\r\n",cur_task->save_offset,cur_task->totallen);
				cur_task->state=TASK_STATE_FINISH;
				if(http_callback_util(cur_task)==0)
				{
					clear_http_task();
				}
			}else{
				#if 0
				if(cur_task->totallen && cur_task->state ==TASK_STATE_RECIVING){
					result = send_http_req(cur_task,download_info);
					if(result)
					{
						new_http_update_socket_deinit();
						cur_task->state = TASK_STATE_ERROR;
					}
					rt_thread_mdelay(10);
				}
				#endif
			}
			if(cur_task->state==TASK_STATE_ERROR)
			{
				DbgWarn("task exit for error need clear!!!");
				clear_http_task();
			}
			
		}while(0);//while(cur_task->state!=TASK_STATE_FINISH && cur_task->state >= TASK_STATE_WAIT_START);
		//close connect
		new_http_update_socket_deinit();
		if(recv_data_buff!=RT_NULL)
		{
			rt_free(recv_data_buff);
		}
	}
	//cb_deinit(&read_buff);
	if(read_buff->bufptr!=RT_NULL){
		rt_free(read_buff->bufptr);
	}
	cb_deinit_static(read_buff);
	restart_modbus_thread();
	DbgFuncExit();
	return 0;
}


bool new_http_update_socket_init(char * server_ip,short port)
{
	//connect http here
	DbgWarn("[http]connect\r\n");
	http_socket_init(server_ip,port);
	return true;
}


void new_http_update_socket_deinit(void)
{
	DbgFuncEntry();
	//disconnect http here
	DbgWarn("[http]disconnect\r\n");
	http_socket_exit();
	DbgFuncExit();
}

int new_http_update_socket_send(unsigned char* temp_buff,int size)
{
	int ret = 0;
	//send data to http here
	DbgWarn("[http]send data !!!\r\n");
	http_socket_send(temp_buff,size);
	return ret;
}

int new_http_update_socket_recv(unsigned char* temp_buff,int max_len)
{
	int len=0;
	//recv data from http here

	len = http_sock_recv(temp_buff,max_len);
	DbgWarn("[http]recv data,LEN=%d!!!\r\n",len);
	//debug_print_block_data(0,temp_buff,len);
    return len;
}




void http_sys_update_module_poll(void)
{

	DbgFuncEntry();

	static uint32_t task_tick=0;
	http_task_t* cur_task;
	cur_task = get_http_task();
	if(task_tick==0||OSTimeGetMS() - task_tick >60*1000 || cur_task->state == TASK_STATE_WAIT_START)
	{
		task_tick = OSTimeGetMS();
		check_http_task();
	}
	

	DbgFuncExit();
}
