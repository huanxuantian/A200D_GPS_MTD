#ifndef __CIP_SOCKMGR_H_
#define __CIP_SOCKMGR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "gnss_if.h"

#define MAX_SOCK_USE 4

#define SOCKET_BUF_SIZE 512

struct sock_set{
    int sock_array[MAX_SOCK_USE];
    int sock_rlen[MAX_SOCK_USE];
    int sock_size;
    unsigned long read_flag;//bit map  for sock read flag
};
typedef struct sock_set* sock_set_t;

#define is_sock_set_flag(set,index) (set->read_flag&(1<<(index%32)))
#define sock_set_enable_flag(set,index) do{set->read_flag|=(1<<(index%32));}while(0)
#define sock_set_disable_flag(set,index) do{set->read_flag&=(~(1<<(index%32)));}while(0)
#define sock_set_add(set,index,dst)   do{set->sock_array[index%MAX_SOCK_USE] = dst;}while(0)
#define sock_clr_flag(set) do{set->read_flag =0;}while(0)
#define is_socket_set_cond(set)    (set->read_flag!=0)

#define set_read_flag(read_flag,index) do{read_flag|=(1<<(index%32));}while(0)
#define is_read_flag_set(read_flag,index) (read_flag&(1<<(index%32)))

void init_cb_buffer();
void init_cip_dev(gnss_device_t dev);

uint8_t get_socket_flag(int index);
void set_socket_flag(int index,uint8_t value);
uint32_t get_socket_err(int index);
void set_socket_err(int index,uint32_t value);
uint32_t inc_socket_err(int index);

int recv_data_select(gnss_device_t gsm_dev);
int recv_data_check(gnss_device_t gsm_dev,int index);
void try_check_ntp(gnss_device_t gsm_dev,uint8_t flag);

int send_AT_CMD(gnss_device_t gsm_dev,char* cmd,char* match,int mstimeout);
char* get_at_recv_ptr();

int cip_sock_open(gnss_device_t gsm_dev,int index,char* host,int port);
int cip_sock_close(gnss_device_t gsm_dev,int index);

int at_cmdres_keyword_matching(char *keyword);
int cip_paser_callback(gnss_ctx_t *ctx,int read_len,int ack_flag);

int sys_get_socket_id(int index);
int sys_socket_send(int index,void* data,int data_size);

void auth_socket_init(void);
int auth_socket_send(void* data,int data_size);
void auth_socket_exit();
void check_auth_socket_reconnect();
int auth_recv_check(unsigned int timeoutms);
int auth_protocol_anaylze(unsigned char index,unsigned char * buf,int len);
//http port
void http_socket_init(char * host,short port);
int http_socket_send(void* data,int data_size);
int http_sock_recv(unsigned char* temp_buff,int max_len);
void http_socket_exit();




#ifdef __cplusplus
}
#endif

#endif /* __CIP_SOCKMGR_H_ */