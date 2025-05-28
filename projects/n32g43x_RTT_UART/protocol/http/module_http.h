#ifndef _MODULE_HTTP_H_
#define _MODULE_HTTP_H_
#include <utils.h>
typedef struct{
	char url[256];
	int  save_block_size;// 
	char fname[128];//下载的文件名

	char    state;//等待，已请求成功，请求失败，正在接收数据，接收数据完成，接收中断（需要续传）
	
	int 	totallen;//需要下载的文件数据总量
	int		save_offset;//文件保存的偏移量
	unsigned int task_period_time;//

	int fetch_timeout;//
	short task_type;//
	short task_index;//source form socket 
	unsigned short crc; 
}__packed http_task_t;

typedef enum {
	TASK_STATE_ERROR=-1,//请求出错，存在返回码，或无响应，保存异常等状态
	TASK_STATE_IDLE=0,//目前没有任务执行，表示任务空闲
	TASK_STATE_WAIT_START,//目前任务已创建，但未开始请求
	TASK_STATE_STARTED,//目前任务已启动，开始请求，未进入传输模式，需要解析头
	TASK_STATE_RECIVING,//目前任务正在传输中，已去除头，数据直接保存
	TASK_STATE_BREAKEN,//目前任务已被中断（外部网络中断）目前未使用
	TASK_STATE_WAITING,//等待网络恢复（等待下一包数据的超时前）目前未使用
	TASK_STATE_FINISH//任务已完成，但未回调完成。一般在回调后需要清除保存结构和状态
} HTTP_TASK_STATE;


typedef struct{
	char    state;//等待，已请求成功，请求失败，正在接收数据，接收数据完成，接收中断（需要续传），目前过程的状态不做判别
	char    mode;//支持断点，非断点，调试
	char	presend_interval;//下载百分比汇报的百分比间隔，5%时设为5，建议2-10。
	char host[64];//主机名或IP
	char ip_addr[16];//IP,目前仅作保留
	int    port;//端口
	unsigned int  req_time;//请求时间，每次请求后记录，用于协助last_rcv_time来判别接收数据超时
	unsigned int  last_rcv_time;//接收数据超时时间，在此时间内没有任何数据，或最后一次收到数据，而目前没有收到数据时判别
	unsigned char* data_save_buff;//目前保留
	int data_buff_len;//目前保留

}http_task_state_t;

void handle_update_flag();
int http_update_info_init(void);
void http_sys_update_module_poll(void);
//初始化http的socket
bool new_http_update_socket_init(char * server_ip,short port);

//关闭和处理socket结构
void new_http_update_socket_deinit(void);

//往http socket发送数据
int new_http_update_socket_send(unsigned char* temp_buff,int size);

//从http socket接收数据
int new_http_update_socket_recv(unsigned char* temp_buff,int max_len);

//新添加任务
//index
//url：http地址
//save_block_size：保存的块大小，预测块数据准备溢出前保存。
//save_path：下载的文件保存的本地路径
//recv_data_timeout：接收数据超时，当在该时间以上没有收到数据，则退出传输过程
//task_vaidity_period：任务有效期，自任务启动后，达到该时间区间后，强制退出任务。（单位：秒）
//task_type：回调时需要回调的类型，用于区分业务类型，在传输过程不会改变
//在添加后任务状态为等待请求状态TASK_STATE_WAIT_START
int new_add_http_update_task(unsigned char index,char* url,
	int save_block_size,char* save_path,
	int recv_data_timeout,int task_vaidity_period,int task_type);
//查询当前是否有HTTP在下载（1：running,0:idle）
int check_http_updata();
//将数据内容保存到文件的处理
int new_pending_http_content(unsigned char* data_pos,int raw_len);

//解析HTTP头的函数，在解析完成后变成TASK_STATE_RECIVING，单次传输，后续仅保存数据，不再解析头。
int new_http_protocol_ayalyze(unsigned char * buf,int len);

//创建扫描线程的模块初始化，在添加任务前必须初始化
void http_sys_update_module_init(void);

void mcu_full_disk_update_finish(unsigned char index);

void save_android_update_path(unsigned char*path);

void get_android_update_path(unsigned char*path);
#endif
