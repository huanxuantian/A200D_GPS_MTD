#ifndef __PKG_GNSS_IF_H_
#define __PKG_GNSS_IF_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "modbus_rt_platform_thread.h"

#define GNSS_NAME_MAX 12
#define GNSS_THREAD_STACK_SIZE                    (1024)
#define GNSS_THREAD_PRIO                          (8)

#define GNSS_MAX_ADU_LENGTH 512
#define GNSS_MAX_PASERBUFF_SIZE 512

#define     GNSS_TIME_OUT   (1000)
#define     GNSS_BYTE_TIME_OUT_MIN   (25)


typedef enum{
    GNSS_RSYNC=0,
    GNSS_RSYNC_READ_ONLY,
    GNSS_SYNC,
}gnssMode;

typedef struct {
    char devname[GNSS_NAME_MAX];  //串口设备的名称
    int baudrate;                       //串口波特率
    int bytesize;                       //串口数据位
    char parity;                        //串口校验位
    int stopbits;                       //串口停止位
    int xonxoff;                        //串口流控制
} gnss_serial_info_t;

/**
 * @brief   modbus rtu通信结构体
 */
typedef struct {
    int serial;                                 //绑定的串口设备句柄
    gnss_serial_info_t serial_info;   //串口信息
    gnssMode mode;
    int status;                                 //设备状态，是否已经打开
    int send_len;                               //打包需要发送数据的长度
    int read_len;                               //串口接收到的数据的长度
    uint8_t *ctx_send_buf;                      //存储发送缓冲区的数据
    uint8_t *ctx_read_buf;                      //存储接收缓冲区的数据
    int send_buf_size;
    int read_buf_size;
    modbus_rt_thread_t *thread;                 //运行的线程标志
    modbus_rt_mutex_t mutex;                    //线程间数据访问的互斥量
    int thread_flag;                            //线程运行标志
    int byte_timeout;                           //接受超时，可通过宏设置，目前默认值
    modbus_rt_sem_t   sem;                      //控制线程任务完成的的信号量
    void * data;                                //
} gnss_device;
typedef gnss_device * gnss_device_t;

typedef struct {
    gnss_device_t device;
    uint8_t *send_buf; /**< 发送缓冲区 */
    int send_bufsz;    /**< 发送缓冲区大小 */
    uint8_t *read_buf; /**< 接收缓冲区 */
    int read_bufsz;    /**< 接收缓冲区大小 */
}gnss_ctx_t;
/**< gnss 结构体 */

typedef struct gnss_dev_util {
    int (*parse_callback)(gnss_ctx_t *ctx,int read_len,int ack_flag);//parse handle callback
    int (*done_callback)(gnss_ctx_t *ctx, int ack_flag,int ack_flag1);//done handle callback
} gnss_dev_util_t;

typedef struct  {
    //所以这里有需要一个线程检测server断开，所以不能用dev中的完成量作为执行完成标志
    modbus_rt_sem_t   completion;    //fuction是否执行完成的完成量
    gnss_dev_util_t util;
    int wait_ack;
    int ack_timeout;//ms timeout
    int ret;//0：timeout，1：ack_ok,2:ack_failed
}gnss_ext_data;
typedef gnss_ext_data * gnss_ext_data_t;

enum {
    GNSS_WAIT_TIMEOUT=0,
    GNSS_ACK_OK=1,
    GNSS_ACK_NOK=2
};
/**
 * @brief   gnss API函数
 */
gnss_device_t create_gnss_modify_buffer(int buffer_size);
gnss_device_t create_gnss();
int gnss_set_serial(gnss_device_t dev, const char *devname, int baudrate, int bytesize, char parity, int stopbits, int xonxoff);
int gnss_open(gnss_device_t dev,gnssMode mode);
int gnss_open_default(gnss_device_t dev);
int gnss_open_sync(gnss_device_t dev);
int gnss_open_rsync(gnss_device_t dev);
int gnss_isopen(gnss_device_t dev);
int gnss_close(gnss_device_t dev);
int gnss_destroy(gnss_device_t * pos_dev);
//send cmd
int gnss_excuse(gnss_device_t dev,int ack_timeout,void *send_data,int send_len,void *recv_buff,int recv_buff_size,int* rlen);
//send large data 
int gnss_send_only(gnss_device_t dev,int ack_timeout,void *send_data,int send_len,void *recv_buff,int recv_buff_size,int* rlen);
//parse callback(ctx,ack_flag,ack_flag1)
int gnss_set_parse_callback(gnss_device_t dev, int (*parse)(gnss_ctx_t *, int, int));


#ifdef __cplusplus
}
#endif

#endif /* __PKG_GNSS_SK_H_ */