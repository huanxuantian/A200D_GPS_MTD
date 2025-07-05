#define DBG_LVL    DBG_WARNING
#define DBG_TAG    "GNIF"
#include <rtdbg.h>
#include <stdio.h>

#include "modbus_rt_platform_memory.h"

#include "gnss_port.h"
#include "gnss_if.h"

static int gnss_dev_init(gnss_device_t * pos_dev, void *data) {
        int ret = 0;
    if(NULL != (*pos_dev)) {
        return -GNSS_RT_EINVAL;
    }
        *pos_dev = modbus_rt_malloc(sizeof(gnss_device));
    if(NULL == (*pos_dev)) {
        return -GNSS_RT_ENOMEM;
    }
    gnss_device_t dev = *pos_dev;

    memset(dev,0,sizeof(gnss_device));
    dev->mode = GNSS_RSYNC;

    ret = modbus_rt_sem_init(&dev->sem);

    if(GNSS_RT_EOK != ret) {
        modbus_rt_free(dev);
        return ret;
    }
        ret = modbus_rt_mutex_init(&dev->mutex);
    if(GNSS_RT_EOK != ret) {
        modbus_rt_sem_destroy(&(dev->sem));
        modbus_rt_free(dev);
        return ret;
    } 
    dev->data = data;
    return GNSS_RT_EOK;
}

static gnss_ext_data_t  gnss_ext_data_create(void) {
    gnss_ext_data_t data = modbus_rt_calloc(1, sizeof(gnss_ext_data));
    if(NULL == data) {
        return NULL;
    }
    memset(data,0,sizeof(gnss_ext_data));
    return data;
}



gnss_device_t create_gnss_modify_buffer(int buffer_size)
{
    int ret = 0;
    gnss_device_t dev = NULL;

    gnss_ext_data_t data = gnss_ext_data_create();
    if(NULL == data) {
        return NULL;
    }
    ret = gnss_dev_init(&dev, data);
    data->ack_timeout = GNSS_TIME_OUT;
    dev->read_buf_size = buffer_size;
    dev->send_buf_size = buffer_size;
    //初始化执行指令的完成信号量
    ret = modbus_rt_sem_init(&(data->completion));
    if(GNSS_RT_EOK != ret) {
        modbus_rt_free(data);
        return NULL;
    }
    return dev;
}

gnss_device_t create_gnss()
{
    return create_gnss_modify_buffer(GNSS_MAX_ADU_LENGTH);
}

int gnss_set_serial(gnss_device_t dev, const char *devname, int baudrate, int bytesize, char parity, int stopbits, int xonxoff)
{
    int len = strlen(devname);
    if((GNSS_NAME_MAX <= len) || (5  > bytesize) || (8 < bytesize)) {
         return -GNSS_RT_EINVAL;
    }
    if(0 < dev->status) {
        return -GNSS_RT_ISOPEN;
    }

    memcpy(dev->serial_info.devname, devname, len);
    dev->serial_info.devname[len] = 0;
    dev->serial_info.baudrate = baudrate;
    dev->serial_info.bytesize = bytesize;
    dev->serial_info.parity = parity;
    dev->serial_info.stopbits = stopbits;
    dev->serial_info.xonxoff = xonxoff;
    return GNSS_RT_EOK;
}

static void gnss_data_entry(void *parameter) {
    gnss_device_t dev = (gnss_device_t)parameter;
    gnss_ext_data_t    data = (gnss_ext_data_t)dev->data;
    int send_len = dev->send_len;
    int read_len = dev->read_len;
    gnss_ctx_t ctx;
    memset(&ctx,0,sizeof(gnss_ctx_t));
    /* 启动线程运行标志 */
    modbus_rt_mutex_lock(&(dev->mutex));
    dev->thread_flag = 1;           //线程运行
    modbus_rt_mutex_unlock(&(dev->mutex));

    while(1) {
            //如果接收到关闭的命令
            if(0 == dev->thread_flag) {
                /* 结束线程运行标志 */
                modbus_rt_mutex_lock(&(dev->mutex));
                dev->thread_flag = -1;           //线程运行
                modbus_rt_mutex_unlock(&(dev->mutex));

                modbus_rt_sem_post(&(dev->sem));
                return ;
            }
            if(GNSS_SYNC== dev->mode){
                //check msg send and wait ack
                if(dev->send_len >0){
                    gnss_serial_send(dev->serial, dev->ctx_send_buf, dev->send_len);
					modbus_rt_mutex_lock(&(dev->mutex));
					dev->send_len=0;
					data->wait_ack = 1;
					modbus_rt_mutex_unlock(&(dev->mutex));
                    rt_thread_mdelay(10);
                }else{
                    rt_thread_mdelay(10);
                }
								memset(dev->ctx_read_buf,0,dev->read_buf_size);
                read_len = gnss_serial_receive(dev->serial, dev->ctx_read_buf, dev->read_buf_size,
                 data->ack_timeout > 100?data->ack_timeout:GNSS_TIME_OUT, dev->byte_timeout);
                if (read_len <= 0) {
                    rt_thread_mdelay(10);
									#if 0
                    if(0 < data->wait_ack){
                        if(data->util.done_callback!=NULL){
                            ctx.device = dev;
                            data->util.done_callback(&ctx,data->wait_ack,data->ret);
                        }
											modbus_rt_mutex_lock(&(dev->mutex));
											data->ret = GNSS_WAIT_TIMEOUT;
											data->wait_ack=0;
											modbus_rt_mutex_unlock(&(dev->mutex));
											modbus_rt_sem_post(&(data->completion));
                    }
									#endif
										read_len =0;
                    continue;
                }
								
                if(data->util.parse_callback!=NULL){
                    ctx.device = dev;
                    ctx.read_buf = dev->ctx_read_buf;
                    ctx.read_bufsz = dev->read_len;
                    ctx.send_buf = dev->ctx_send_buf;
                    ctx.send_bufsz = dev->send_len;
                    data->ret = data->util.parse_callback(&ctx,read_len,data->wait_ack);
                }
                if(data->wait_ack){
                    modbus_rt_mutex_lock(&(dev->mutex));
                    data->wait_ack=0;
                    dev->read_len = read_len;
                    modbus_rt_mutex_unlock(&(dev->mutex));
                    modbus_rt_sem_post(&(data->completion));
                    rt_thread_mdelay(50);
                }


            }else{
                read_len = gnss_serial_receive(dev->serial, dev->ctx_read_buf, dev->read_buf_size ,
                data->ack_timeout > 100?data->ack_timeout:GNSS_TIME_OUT, dev->byte_timeout);
                if (read_len <= 0) {
                    rt_thread_mdelay(10);
                    continue;
                }
                if(data->util.parse_callback!=NULL){
                    ctx.device = dev;
                    ctx.read_buf = dev->ctx_read_buf;
                    ctx.read_bufsz = dev->read_len;
                    if(dev->mode!=GNSS_RSYNC_READ_ONLY){
                        ctx.send_buf = dev->ctx_send_buf;
                        ctx.send_bufsz = dev->send_len;
                    }else{
                        ctx.send_buf = NULL;
                        ctx.send_bufsz = 0;
                    }

                    send_len = data->util.parse_callback(&ctx,read_len,0);
                    if (send_len > 0 && dev->mode!=GNSS_RSYNC_READ_ONLY) {
                        gnss_serial_send(dev->serial, dev->ctx_send_buf, send_len);
                    }
                }
            }
            if(0 < data->wait_ack){
                if(data->util.done_callback!=NULL){
                    ctx.device = dev;
                    data->util.done_callback(&ctx,data->wait_ack,data->ret);
                }
				if(data->wait_ack){
					modbus_rt_mutex_lock(&(dev->mutex));
					data->wait_ack=0;
					dev->read_len = read_len;
					modbus_rt_mutex_unlock(&(dev->mutex));
					modbus_rt_sem_post(&(data->completion));
				}
            }
    }
}

int gnss_open(gnss_device_t dev,gnssMode mode)
{
    int id_thread = 0;
    char str_name[GNSS_NAME_MAX] = {0};
    if((NULL == dev) || (NULL == dev->data)) {
        return -GNSS_RT_EINVAL;
    }
    if(0 < dev->status) {
        return -GNSS_RT_ISOPEN;
    }
    gnss_ext_data_t data_ext = NULL;
    data_ext = (gnss_ext_data_t)dev->data;
    (void)(data_ext);

    gnss_serial_info_t info = dev->serial_info;
    dev->byte_timeout = GNSS_BYTE_TIME_OUT_MIN;
    int serial = gnss_serial_open(info.devname, info.baudrate, info.bytesize, 
        info.parity, info.stopbits, info.xonxoff);
    if(0  > serial) {
        return serial;
    }
    id_thread = serial;
    dev->serial = serial;
    if(dev->read_buf_size <=0)
    {
        dev->read_buf_size = GNSS_MAX_ADU_LENGTH;
    }
    if(dev->send_buf_size<=0)
    {
        dev->send_buf_size = GNSS_MAX_ADU_LENGTH;
    }
    
    dev->send_len = 0;
    dev->read_len = 0;
    if(dev->mode!=mode){dev->mode=mode;}
    if(dev->mode!=GNSS_RSYNC_READ_ONLY){
        dev->ctx_send_buf = modbus_rt_calloc(1, dev->send_buf_size);
        if(NULL == dev->ctx_send_buf) {
            gnss_serial_close(dev->serial);
            dev->serial = 0;
            return -GNSS_RT_ENOMEM;
        }
    }

    dev->ctx_read_buf = modbus_rt_calloc(1, dev->read_buf_size);
    if(NULL == dev->ctx_read_buf) {
        gnss_serial_close(dev->serial);
        dev->serial = 0;
        return -GNSS_RT_ENOMEM;
    }
    if(GNSS_RSYNC_READ_ONLY==dev->mode){
        rt_snprintf(str_name,sizeof(str_name),"gnss_r%d",id_thread%0xff);
    }else{
        rt_snprintf(str_name,sizeof(str_name),"gnss_rt%d",id_thread%0xff);
    }
    dev->thread = modbus_rt_thread_init(str_name, gnss_data_entry, 
                dev, GNSS_THREAD_STACK_SIZE, GNSS_THREAD_PRIO, 10);


    if(NULL != dev->thread) {
        modbus_rt_thread_startup(dev->thread);
    } else {
			if(dev->mode!=GNSS_RSYNC_READ_ONLY){
        modbus_rt_free(dev->ctx_send_buf);
			}
        modbus_rt_free(dev->ctx_read_buf);
        gnss_serial_close(dev->serial);
        dev->serial = 0;
        return -GNSS_RT_EINVAL;
    }
        dev->status = 1;
    return GNSS_RT_EOK;
}

int gnss_open_default(gnss_device_t dev){
    return gnss_open(dev,GNSS_RSYNC_READ_ONLY);
}
int gnss_open_sync(gnss_device_t dev){
    return gnss_open(dev,GNSS_SYNC);
}
int gnss_open_rsync(gnss_device_t dev){
    return gnss_open(dev,GNSS_RSYNC);
}

int gnss_isopen(gnss_device_t dev)
{
    if ((NULL == dev)) { 
        return 0;
    }
    return dev->status;
}

int gnss_close(gnss_device_t dev)
{
    if(!gnss_isopen(dev))
    {
        return -GNSS_RT_ERROR;
    }
    if(1 == dev->thread_flag) {
         /* 尝试结束线程 */
        modbus_rt_mutex_lock(&(dev->mutex));
        dev->thread_flag = 0;           //尝试结束线程
        modbus_rt_mutex_unlock(&(dev->mutex));

        /* 等待设备线程结束 */
        modbus_rt_sem_wait(&(dev->sem));
    }
		if(dev->mode!=GNSS_RSYNC_READ_ONLY){
    modbus_rt_free(dev->ctx_send_buf);
		}
    modbus_rt_free(dev->ctx_read_buf);
    gnss_serial_close(dev->serial);
    dev->serial = 0;

    dev->status = 0;
    modbus_rt_thread_destroy(dev->thread);

    dev->thread = NULL;
    return GNSS_RT_EOK;

}

int gnss_destroy(gnss_device_t * pos_dev)
{
    if((NULL == pos_dev) || (NULL == (*pos_dev))) {
        return -GNSS_RT_EINVAL;
    }
    gnss_device_t dev = *pos_dev;

    if (NULL == dev) { 
        return -GNSS_RT_EINVAL;
    }  
    if(dev->thread != 0) {
        gnss_close(dev);
    }
    gnss_ext_data_t data = (gnss_ext_data_t)dev->data;  
    modbus_rt_sem_destroy(&(data->completion));
    modbus_rt_free(data);

    modbus_rt_mutex_destroy(&(dev->mutex));
    modbus_rt_sem_destroy(&(dev->sem));
    modbus_rt_free(dev);
    *pos_dev = NULL;
    return GNSS_RT_EOK;

}


int gnss_excuse(gnss_device_t dev,int ack_timeout,void *send_data,int send_len,void *recv_buff,int recv_buff_size,int* rlen)
{
    int recv_len;
		//int time_count=0;
    gnss_ext_data_t data = (gnss_ext_data_t)dev->data;  
    if((NULL == dev) || (NULL == dev->data) || (1 != dev->status)) {
        return -GNSS_RT_EINVAL;
    }
    data = (gnss_ext_data_t)dev->data;

    modbus_rt_mutex_lock(&(dev->mutex));
    data->ret=GNSS_WAIT_TIMEOUT;
	if(GNSS_SYNC!= dev->mode){
		data->wait_ack=1;
	}
    //set timeout here
    data->ack_timeout = ack_timeout;
    //handle build msg by send_data and recv_buff and wait ack;
    if(GNSS_SYNC== dev->mode){
        memcpy(dev->ctx_send_buf,send_data,send_len);
        dev->send_len = send_len;
    }else{
        gnss_serial_send(dev->serial, send_data,send_len);
    }
    modbus_rt_mutex_unlock(&(dev->mutex));

    modbus_rt_sem_wait_timeout(&(data->completion),ack_timeout);
		if(dev->read_len <= 0|| dev->ctx_read_buf[0]==0x00){
			//
            #if 0
			do{
				rt_thread_mdelay(100);
				time_count++;
				if(dev->read_len > 0 && dev->ctx_read_buf[0]!=0x00){
					LOG_D("AT recv OK");
					break;
				}
				if(time_count>150) break;
			}while(time_count<ack_timeout/100);
            #endif
            *rlen = 0;
            return 0;
		}
    *rlen = 0;
    if(dev->read_len>0 && dev->ctx_read_buf!=RT_NULL){
        LOG_D("AT recv COPY STA");
			recv_len = dev->read_len>=recv_buff_size?recv_buff_size:dev->read_len;
        memcpy(recv_buff,dev->ctx_read_buf,recv_len);
        *rlen = recv_len;
				data->ret=GNSS_ACK_OK;
                LOG_D("AT recv COPY FIN");
    }
    return data->ret;
}

int gnss_send_only(gnss_device_t dev,int ack_timeout,void *send_data,int send_len,void *recv_buff,int recv_buff_size,int* rlen)
{
    int recv_len;
		//int time_count=0;
    int i;
    int pack_count;
    int pack_len;
    int pack_offset=0;
    gnss_ext_data_t data = (gnss_ext_data_t)dev->data;  
    if((NULL == dev) || (NULL == dev->data) || (1 != dev->status)) {
        return -GNSS_RT_EINVAL;
    }
    data = (gnss_ext_data_t)dev->data;

    modbus_rt_mutex_lock(&(dev->mutex));
    data->ret=GNSS_WAIT_TIMEOUT;
	if(GNSS_SYNC!= dev->mode){
		data->wait_ack=1;
	}
    //set timeout here
    data->ack_timeout = ack_timeout;
    //handle build msg by send_data and recv_buff and wait ack;
    if(GNSS_SYNC== dev->mode && send_len < dev->send_buf_size){
        memcpy(dev->ctx_send_buf,send_data,send_len);
        dev->send_len = send_len;
    }else{
        pack_count = send_len/dev->send_buf_size + ((send_len%dev->send_buf_size>0)?1:0);
        for(i=0;i<pack_count;i++){
            pack_offset = (i*dev->send_buf_size);
            pack_len = (send_len-pack_offset)<=dev->send_buf_size?(send_len-pack_offset):dev->send_buf_size;
            gnss_serial_send(dev->serial, (void*)((unsigned char*)send_data+pack_offset),pack_len);
        }
        
    }
    modbus_rt_mutex_unlock(&(dev->mutex));

    modbus_rt_sem_wait_timeout(&(data->completion),ack_timeout);
		if(dev->read_len <= 0|| dev->ctx_read_buf[0]==0x00){
			//
            #if 0
			do{
				rt_thread_mdelay(100);
				time_count++;
				if(dev->read_len > 0 && dev->ctx_read_buf[0]!=0x00){
					LOG_I("AT recv OK");
					break;
				}
				if(time_count>150) break;
			}while(time_count<ack_timeout/100);
            #endif
            *rlen = 0;
            return 0;
		}
    *rlen = 0;
    if(dev->read_len>0 && dev->ctx_read_buf!=RT_NULL){
			recv_len = dev->read_len>=recv_buff_size?recv_buff_size:dev->read_len;
        memcpy(recv_buff,dev->ctx_read_buf,recv_len);
        *rlen = recv_len;
		data->ret=GNSS_ACK_OK;
    }
    return data->ret;
}


int gnss_set_parse_callback(gnss_device_t dev, int (*parse)(gnss_ctx_t *, int, int))
{
    if((NULL == dev) || (NULL == dev->data)) { 
        return -GNSS_RT_EINVAL;
    }
    if(0 < dev->status) {
        return -GNSS_RT_ISOPEN;
    }

    ((gnss_ext_data_t)dev->data)->util.parse_callback = parse;
    return GNSS_RT_EOK;
}