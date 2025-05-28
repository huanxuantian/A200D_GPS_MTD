
#include <rtthread.h>
#define DBG_LEVEL           DBG_INFO
#define DBG_SECTION_NAME    "MBT"
#include <rtdbg.h>
#include "board.h"
#include "utils.h"
#include "setting.h"

#include "modbus_m.h"

#include "serial.h"
#ifdef USE_MODBUS_RTU_MASTER
#include "modbus_rtu.h"
#ifdef MODBUS_DATA_TRANS_ENABLE
#include "modbus_data_trans.h"
#endif
#else
#include "gnss_if.h"
#include "gnss_port.h"
#endif

ALIGN(RT_ALIGN_SIZE)
#if USE_MODBUS_THREAD_TASK
static rt_uint8_t modbus_master_stack[ 512 ];
static struct rt_thread modbus_master_thread;
#endif


#ifdef USE_MODBUS_RTU_MASTER
static rtu_modbus_device_t rtu_master1 = RT_NULL;
static rtu_modbus_device_t rtu_master2 = RT_NULL;
static rtu_modbus_device_t rtu_master3 = RT_NULL;
#else
static gnss_device_t rtu_dev1=NULL;
static gnss_device_t rtu_dev2=NULL;
static gnss_device_t rtu_dev3=NULL;
#endif

static volatile uint8_t modbus_start_flag=0;
static volatile uint8_t modbus_clear_flag=0;

#define SENSOR1_BUND	9600
#define	SENSOR1_DATA_REG_ADDR	2
#define	SENSOR1_DATA_REG_COUNT	2
#define INPUT_BLOCK_SENSOR1_OFFSET 6
#define INPUT_BLOCK_SENSOR1_COUNT 2

#define SENSOR2_BUND	9600
#define	SENSOR2_DATA_REG_ADDR	0
#define	SENSOR2_DATA_REG_COUNT	2
#define INPUT_BLOCK_SENSOR2_OFFSET 4
#define INPUT_BLOCK_SENSOR2_COUNT 2

#define SENSOR3_BUND	9600
#define	SENSOR3_DATA_REG_ADDR	0
#define	SENSOR3_DATA_REG_COUNT	8
#define INPUT_BLOCK_SENSOR3_OFFSET 8
#define INPUT_BLOCK_SENSOR3_COUNT 8

#if USE_INPUT_BLOCK
#define SENSOR_DATA_OFFET 4
#define SENSOR_DATA_ALL_COUNT  (INPUT_BLOCK_SENSOR1_COUNT+INPUT_BLOCK_SENSOR2_COUNT+INPUT_BLOCK_SENSOR3_COUNT)
#define SENSOR_DATA_SIZE	(SENSOR_DATA_ALL_COUNT*2)


uint16_t input_block[16];
void init_block_data(){
	memset((unsigned char*)&input_block,0,sizeof(input_block));
}


int build_input_data(unsigned char* buff,int buff_size)
{
	int real_size = SENSOR_DATA_SIZE;
	if(real_size >sizeof(input_block)-SENSOR_DATA_OFFET){
		LOG_W("warning!!!input_block setting overflow!!!!");
		real_size = sizeof(input_block)-SENSOR_DATA_OFFET;
	}
	if(buff_size<real_size){
		LOG_W("warning!!!should not all sensor_data copy to buff size too small!!!");
		real_size = SENSOR_DATA_SIZE;
	}

	if(real_size>0){
		memcpy(buff,&input_block[SENSOR_DATA_OFFET],real_size);
	}
	return real_size;
}
#endif
#ifndef USE_MODBUS_RTU_MASTER 
typedef struct{
    unsigned char dev_address;
    unsigned char function_code;
} __packed  modbus_msg_head_t;

typedef struct{
    unsigned short lcrc16;
} __packed  modbus_msg_tail_t;

typedef struct{
    unsigned short start_address;
    unsigned short address_len;
} __packed  modbus_reg_req_t;

typedef struct{
    modbus_msg_head_t head;//code=03/04
    modbus_reg_req_t body;
    modbus_msg_tail_t tail;
} __packed  modbus_msg_reg_read_t;

typedef struct{
    modbus_msg_head_t head;//code=03/04|83/84
    unsigned char pdu_bytes;
}__packed modbus_msg_reg_read_ack_head_t;
//
typedef struct{
    modbus_msg_reg_read_ack_head_t ack_head;
    unsigned int reg_data;
    modbus_msg_tail_t ack_tail;
}__packed modbus_msg_reg_read_lux_ack_t;

typedef struct{
    modbus_msg_reg_read_ack_head_t ack_head;
    unsigned short reg_data[2];
    modbus_msg_tail_t ack_tail;
}__packed modbus_msg_reg_read_thm_ack_t;

typedef struct{
    modbus_msg_reg_read_ack_head_t ack_head;
    unsigned short reg_data[8];
    modbus_msg_tail_t ack_tail;
}__packed modbus_msg_reg_read_soil_ack_t;


#define SIZEOF_HEAD (sizeof(modbus_msg_head_t))
#define SIZEOF_TAIL (sizeof(modbus_msg_tail_t))
#define MIN_PROTOCOL_SIZE (sizeof(modbus_msg_head_t)+sizeof(modbus_msg_tail_t))//2+2

int check_msg_data(uint8_t* read_buf,int read_len)
{
	modbus_msg_tail_t* tail;
	unsigned short lcrc16;
	if(MIN_PROTOCOL_SIZE < read_len){
		tail = (modbus_msg_tail_t*)(read_buf+(read_len-SIZEOF_TAIL));
		lcrc16 = usMBCRC16(read_buf,(read_len-SIZEOF_TAIL));
		if(lcrc16== tail->lcrc16) {
			return 1;
		}
	}
	return 0;
}
#if USE_RTU_MASTER1
static int modbus1_paser_callback(gnss_ctx_t *ctx,int read_len,int ack_flag){

	LOG_W("parse modbus1 req rec_len=%d,ack_flag=%d",read_len,ack_flag);
	debug_print_block_data(0,ctx->read_buf,read_len);
	if(read_len < sizeof(modbus_msg_reg_read_lux_ack_t)) return 0;
	if(check_msg_data(ctx->read_buf,read_len))
	{
		modbus_msg_reg_read_lux_ack_t* data = (modbus_msg_reg_read_lux_ack_t*) ctx->read_buf;
		lux_sensor_data_t lux_data;
		lux_data.lux = data->reg_data;
		update_lux_data(lux_data);
		LOG_I("lux sensor lux %d", EndianReverse32(lux_data.lux));

	}
	return 1;
}
#endif
#if USE_RTU_MASTER2
static int modbus2_paser_callback(gnss_ctx_t *ctx,int read_len,int ack_flag){
	LOG_W("parse modbus2 req rec_len=%d,ack_flag=%d",read_len,ack_flag);
	debug_print_block_data(0,ctx->read_buf,read_len);
	if(read_len < sizeof(modbus_msg_reg_read_thm_ack_t)) return 0;
	if(check_msg_data(ctx->read_buf,read_len))
	{
		modbus_msg_reg_read_thm_ack_t* data = (modbus_msg_reg_read_thm_ack_t*) ctx->read_buf;
		thm_sensor_data_t thm_data;
		thm_data.temperature = data->reg_data[0];
		thm_data.humidity = data->reg_data[1];
		update_thm_data(thm_data);

	}
	return 1;
}
#endif
#if USE_RTU_MASTER3
static int modbus3_paser_callback(gnss_ctx_t *ctx,int read_len,int ack_flag){
	LOG_W("parse modbus3 req rec_len=%d,ack_flag=%d",read_len,ack_flag);
	debug_print_block_data(0,ctx->read_buf,read_len);
	if(read_len < sizeof(modbus_msg_reg_read_soil_ack_t)) return 0;
	if(check_msg_data(ctx->read_buf,read_len))
	{
		modbus_msg_reg_read_soil_ack_t* data = (modbus_msg_reg_read_soil_ack_t*) ctx->read_buf;
		
		soil_sensor_data_t soil_data;
		memcpy((unsigned char*)&soil_data,(unsigned char*)&data->reg_data[0],sizeof(soil_sensor_data_t));
		update_soil_data(soil_data);
	}
	return 1;
}
#endif
int send_req_msg(gnss_device_t dev,unsigned char addr,unsigned char func,unsigned short data_reg_addr,unsigned short data_reg_size)
{
	unsigned char data_buffer[24];
	int rlen=0;
	modbus_msg_reg_read_t msg;
	memset(&msg,0,sizeof(modbus_msg_reg_read_t));
	msg.head.dev_address = addr;
    msg.head.function_code = func;
    msg.body.start_address = data_reg_addr;
    msg.body.address_len = data_reg_size;
    msg.body.start_address = EndianReverse16(msg.body.start_address);
    msg.body.address_len = EndianReverse16(msg.body.address_len);
    msg.tail.lcrc16 = usMBCRC16((unsigned char*)&msg,sizeof(modbus_msg_reg_read_t)-SIZEOF_TAIL);

	gnss_excuse(dev,1000,(unsigned char*)&msg,sizeof(modbus_msg_reg_read_t),
		data_buffer,sizeof(data_buffer),&rlen);
	if(rlen>0)
	{
		LOG_D("modbus ack !!!");
		//debug_print_block_data(0,data_buffer,rlen);
	}
	return rlen;
}

void modbus_init()
{
	int ret = GNSS_RT_EOK;
		#if USE_RTU_MASTER1
		if(rtu_dev1==NULL){
			rtu_dev1 =  create_gnss_modify_buffer(128);
		}
		if(rtu_dev1==NULL){
			LOG_E("modbus1 init master error");
			return;
		}
		ret = gnss_set_serial(rtu_dev1,MASTER1_SERIAL_NAME,SENSOR1_BUND,8,'N',STOP_BITS_1,0);
		if(GNSS_RT_EOK != ret ){
			LOG_E("modbus1 init master uart error");
			return;
		}
		gnss_set_parse_callback(rtu_dev1,modbus1_paser_callback);
		#endif
		#if USE_RTU_MASTER2
		if(rtu_dev2==RT_NULL){
			rtu_dev2 = create_gnss_modify_buffer(128);
		}
		if(rtu_dev2==NULL){
			LOG_E("modbus2 init master error");
			return;
		}
		ret = gnss_set_serial(rtu_dev2,MASTER2_SERIAL_NAME,SENSOR2_BUND,8,'N',STOP_BITS_1,0);
		if(GNSS_RT_EOK != ret ){
			LOG_E("modbus2 init master uart error");
			return;
		}
		gnss_set_parse_callback(rtu_dev2,modbus2_paser_callback);
		#endif
		#if USE_RTU_MASTER3
		if(rtu_dev3==RT_NULL){
			rtu_dev3 = create_gnss_modify_buffer(128);
		}
		if(rtu_dev3==NULL){
			LOG_E("modbus3 init master error");
			return;
		}
		ret = gnss_set_serial(rtu_dev3,MASTER3_SERIAL_NAME,SENSOR3_BUND,8,'N',STOP_BITS_1,0);
		if(GNSS_RT_EOK != ret ){
			LOG_E("modbus3 init master uart error");
			return;
		}
		gnss_set_parse_callback(rtu_dev3,modbus3_paser_callback);
		#endif
		#if USE_RTU_MASTER1
		ret = gnss_open_sync(rtu_dev1);
		if(GNSS_RT_EOK != ret ){
			LOG_E("modbus1 open master uart error");
			return;
		}
		#endif
		#if USE_RTU_MASTER2
		ret = gnss_open_sync(rtu_dev2);
		if(GNSS_RT_EOK != ret ){
			LOG_E("modbus2 open master uart error");
			return;
		}
		#endif
		#if USE_RTU_MASTER3
		ret = gnss_open_sync(rtu_dev3);
		if(GNSS_RT_EOK != ret ){
			LOG_E("modbus3 open master uart error");
			return;
		}
		#endif
		modbus_start_flag=1;
}

void handle_modbus_task()
{
	int ret = 0;
    if(modbus_start_flag)
    {
		{
			#if USE_RTU_MASTER1
			if(rtu_dev1!=RT_NULL && gnss_isopen(rtu_dev1))
			{
				ret = send_req_msg(rtu_dev1,1,0x04,SENSOR1_DATA_REG_ADDR,SENSOR1_DATA_REG_COUNT);
				if(ret<=0){
					LOG_E("modbus1 req master data timeout");
				}
			
			}else{
				LOG_W("master1 not init or close in case");
			}
			#endif
			#if USE_RTU_MASTER2
			if(rtu_dev2!=RT_NULL&& gnss_isopen(rtu_dev2))
			{
				ret = send_req_msg(rtu_dev2,1,0x04,SENSOR2_DATA_REG_ADDR,SENSOR2_DATA_REG_COUNT);
				if(ret<=0){
					LOG_E("modbus2 req master data timeout");
				}
			
			}else{
				LOG_W("master2 not init or close in case");
			}
			#endif
			#if USE_RTU_MASTER3
			if(rtu_dev3!=RT_NULL && gnss_isopen(rtu_dev3))
			{
				ret = send_req_msg(rtu_dev3,1,0x04,SENSOR3_DATA_REG_ADDR,SENSOR3_DATA_REG_COUNT);
				if(ret<=0){
					LOG_E("modbus3 req master data timeout");
				}
			
			}else{
				LOG_W("master3 not init or close in case");
			}
			#endif
		}
	}

}
#else
void modbus_init()
{
	int ret = MODBUS_RT_EOK;

	#if USE_RTU_MASTER1
	if(rtu_master1==RT_NULL)
	{
		rtu_master1 = modbus_rtu(MODBUS_MASTER);
	}
	if(rtu_master1==NULL){
		LOG_E("modbus1 init master error");
		return;
	}
	ret = modbus_rtu_set_serial(rtu_master1,MASTER1_SERIAL_NAME,SENSOR1_BUND,8,'N',STOP_BITS_1,0);
	if(MODBUS_RT_EOK != ret ){
		LOG_E("modbus1 init master uart error");
		return;
	}
	#endif
	#if USE_RTU_MASTER2
	if(rtu_master2==RT_NULL){
		rtu_master2 = modbus_rtu(MODBUS_MASTER);
	}
	if(rtu_master2==NULL){
		LOG_E("modbus2 init master error");
		return;
	}
	ret = modbus_rtu_set_serial(rtu_master2,MASTER2_SERIAL_NAME,SENSOR2_BUND,8,'N',STOP_BITS_1,0);
	if(MODBUS_RT_EOK != ret ){
		LOG_E("modbus2 init master uart error");
		return;
	}
	#endif
	#if USE_RTU_MASTER3
	if(rtu_master3==RT_NULL){
		rtu_master3 = modbus_rtu(MODBUS_MASTER);
	}
	if(rtu_master3==NULL){
		LOG_E("modbus3 init master error");
		return;
	}
	ret = modbus_rtu_set_serial(rtu_master3,MASTER3_SERIAL_NAME,SENSOR3_BUND,8,'N',STOP_BITS_1,0);
	if(MODBUS_RT_EOK != ret ){
		LOG_E("modbus3 init master uart error");
		return;
	}
	#endif
	
	#if USE_RTU_MASTER1
	ret = modbus_rtu_open(rtu_master1);
	if(MODBUS_RT_EOK != ret ){
		LOG_E("modbus1 open master uart error");
		return;
	}
	#endif
	#if USE_RTU_MASTER2
	ret = modbus_rtu_open(rtu_master2);
	if(MODBUS_RT_EOK != ret ){
		LOG_E("modbus2 open master uart error");
		//return;
	}
	#endif
	#if USE_RTU_MASTER3
	ret = modbus_rtu_open(rtu_master3);
	if(MODBUS_RT_EOK != ret ){
		LOG_E("modbus3 open master uart error");
		//return;
	}
	#endif
	modbus_start_flag =1;

}
void handle_modbus_task()
{
	int ret = MODBUS_RT_EOK;
int i;

uint8_t reg_data[20];
uint16_t regs[8];
uint16_t* ptr_reg=&regs[0];
memset(reg_data,0,sizeof(reg_data));
memset(&regs,0,sizeof(regs));
	#if USE_RTU_MASTER1
	if(rtu_master1!=RT_NULL)
	{
		memset(reg_data,0,sizeof(reg_data));
		memset(&regs,0,sizeof(regs));
		ret = modbus_rtu_excuse(rtu_master1,1,AGILE_MODBUS_FC_READ_INPUT_REGISTERS,
					SENSOR1_DATA_REG_ADDR,SENSOR1_DATA_REG_COUNT,&reg_data);
		if(MODBUS_RT_EOK != ret ){
			LOG_E("modbus1 req master data timeout");
		}else{
			#ifdef MODBUS_DATA_TRANS_ENABLE
			modbus_data_bytes2regs(BIG_ENDIAL,ptr_reg,reg_data,SENSOR1_DATA_REG_COUNT*2);
			#else
			for(i=0;i<SENSOR1_DATA_REG_COUNT;i++){
				*(ptr_reg+i) =  (reg_data[2*i+1]<<8)|(reg_data[2*i]&0xFF);
			}
			#endif
			for(i=0;i<MIN(SENSOR1_DATA_REG_COUNT,INPUT_BLOCK_SENSOR1_COUNT);i++){
				LOG_I("modbus1 data ack reg[%d]= 0x%04X(dec=%d)",i,regs[i],regs[i]);
				#if USE_INPUT_BLOCK
				input_block[INPUT_BLOCK_SENSOR1_OFFSET+i] = regs[i];
				#endif
				regs[i] = EndianReverse16(regs[i]);
			}
			lux_sensor_data_t* lux_data;
			lux_data = (lux_sensor_data_t*)&regs[0];
			update_lux_data(*lux_data);
			LOG_I("lux sensor lux %d", EndianReverse32(lux_data->lux));
		}		
	}else{
		LOG_W("master1 not init or close in case");
	}
	#endif
	#if USE_RTU_MASTER2
	if(rtu_master2!=RT_NULL)
	{
		memset(reg_data,0,sizeof(reg_data));
		memset(&regs,0,sizeof(regs));
		ret = modbus_rtu_excuse(rtu_master2,1,AGILE_MODBUS_FC_READ_INPUT_REGISTERS,
					SENSOR2_DATA_REG_ADDR,SENSOR2_DATA_REG_COUNT,&reg_data);
		if(MODBUS_RT_EOK != ret ){
			LOG_E("modbus2 req master data timeout");
		}else{
			
			#ifdef MODBUS_DATA_TRANS_ENABLE
			modbus_data_bytes2regs(BIG_ENDIAL,ptr_reg,reg_data,4);
			#else
			for(i=0;i<2;i++){
				*(ptr_reg+i) =  (reg_data[2*i+1]<<8)|(reg_data[2*i]&0xFF);
			}
			#endif
			for(i=0;i<MIN(SENSOR2_DATA_REG_COUNT,INPUT_BLOCK_SENSOR2_COUNT);i++){
				LOG_I("modbus2 data ack reg[%d]= 0x%04X(dec=%d)",i,regs[i],regs[i]);
				#if USE_INPUT_BLOCK
				input_block[INPUT_BLOCK_SENSOR2_OFFSET+i] = regs[i];
				#endif
				regs[i] = EndianReverse16(regs[i]);
			}
		thm_sensor_data_t* thm_data;
			thm_data = (thm_sensor_data_t*)&regs[0];
			update_thm_data(*thm_data);
		}
	}else{
		LOG_W("master2 not init or close in case");
	}
	#endif
	#if USE_RTU_MASTER3
	if(rtu_master3!=RT_NULL)
	{
		memset(reg_data,0,sizeof(reg_data));
		memset(&regs,0,sizeof(regs));
		ret = modbus_rtu_excuse(rtu_master3,1,AGILE_MODBUS_FC_READ_INPUT_REGISTERS,
					SENSOR3_DATA_REG_ADDR,SENSOR3_DATA_REG_COUNT,&reg_data);
		if(MODBUS_RT_EOK != ret ){
			LOG_E("modbus3 req master data timeout");
		}else{
			#ifdef MODBUS_DATA_TRANS_ENABLE
			modbus_data_bytes2regs(BIG_ENDIAL,ptr_reg,reg_data,16);
			#else
			for(i=0;i<8;i++){
				*(ptr_reg+i) =  (reg_data[2*i+1]<<8)|(reg_data[2*i]&0xFF);
			}
			#endif
			for(i=0;i<MIN(SENSOR3_DATA_REG_COUNT,INPUT_BLOCK_SENSOR3_COUNT);i++){
				LOG_I("modbus3 data ack reg[%d]= 0x%04X(dec=%d)",i,regs[i],regs[i]);
				#if USE_INPUT_BLOCK
				input_block[INPUT_BLOCK_SENSOR3_OFFSET+i] = regs[i];
				#endif
				regs[i] = EndianReverse16(regs[i]);
			}
			soil_sensor_data_t* soil_data;
			soil_data = (soil_sensor_data_t*)&regs[0];
			update_soil_data(*soil_data);
		}
	}else{
		LOG_W("master3 not init or close in case");
	}
	#endif
}
#endif
#if USE_MODBUS_THREAD_TASK
/**
 * @brief  modbus1 thread entry
 */
static void modbus_master_thread_entry(void* parameter)
{
		uint32_t last_check_sensor_time=0;

		modbus_init();

		modbus_clear_flag =0;
		last_check_sensor_time = OSTimeGet();
		
    while(modbus_start_flag)
    {
		rt_thread_mdelay(20);
        if(OSTimeGet() - last_check_sensor_time > 10)
		{
			last_check_sensor_time = OSTimeGet();
			handle_modbus_task();
		}
	}
	LOG_W("master thread exit in case");
	modbus_clear_flag =1;
}

void start_modbus_thread()
{
	rt_err_t result;
    /* init modbus master thread */
    result = rt_thread_init(&modbus_master_thread, "mbm", modbus_master_thread_entry, RT_NULL, (rt_uint8_t*)&modbus_master_stack[0], sizeof(modbus_master_stack), 5, 5);
    if (result == RT_EOK)
    {
        rt_thread_startup(&modbus_master_thread);
    }
}

#else
void start_modbus_thread()
{

}
#endif
//def USE_MODBUS_RTU_MASTER
void stop_modbus_thread()
{
	int try_count=0;
	
	modbus_start_flag=0;
	modbus_clear_flag=0;
	#if USE_MODBUS_THREAD_TASK
	do{
		rt_thread_mdelay(10);
		if(modbus_clear_flag){
			//rt_thread_delete(&modbus_master_thread);
			memset(&modbus_master_thread,0,sizeof(modbus_master_thread));
			break;
		}
		try_count++;
	}while(try_count<1500);
	if(try_count==1500|| modbus_clear_flag ==0){
		LOG_E("modbus thread maybe not stop");
	}
	#endif
	#ifdef USE_MODBUS_RTU_MASTER
	if(rtu_master1!=RT_NULL)
	{
		if(modbus_rtu_isopen(rtu_master1)){//IsOpen need close
			modbus_rtu_close(rtu_master1);
			rt_thread_mdelay(20);
		}
	}
	if(rtu_master2!=RT_NULL)
	{
		if(modbus_rtu_isopen(rtu_master2)){//IsOpen need close
			modbus_rtu_close(rtu_master2);
			rt_thread_mdelay(20);
		}
	}
	if(rtu_master3!=RT_NULL)
	{
		if(modbus_rtu_isopen(rtu_master3)){//IsOpen need close
			modbus_rtu_close(rtu_master3);
			rt_thread_mdelay(20);
		}
	}
	#else
	if(rtu_dev1!=RT_NULL)
	{
		if(gnss_isopen(rtu_dev1))
		{
			gnss_close(rtu_dev1);
			rt_thread_mdelay(20);
		}
	}
	if(rtu_dev2!=RT_NULL)
	{
		if(gnss_isopen(rtu_dev2))
		{
			gnss_close(rtu_dev2);
			rt_thread_mdelay(20);
		}
	}
	if(rtu_dev3!=RT_NULL)
	{
		if(gnss_isopen(rtu_dev3))
		{
			gnss_close(rtu_dev3);
			rt_thread_mdelay(20);
		}
	}
	#endif
	rt_thread_mdelay(20);
}

void restart_modbus_thread()
{
	//start_modbus_thread();
	rt_system_reset();
}
