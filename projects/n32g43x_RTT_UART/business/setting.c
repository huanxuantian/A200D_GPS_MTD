#define DBG_LEVEL           DBG_INFO
#define DBG_SECTION_NAME    "SETTING"
#include <rtdbg.h>

#include <string.h>
#include <stdlib.h>

#include "setting.h"
#include "status.h"
#include <system_bsp.h>

#include "s2j.h"

static spi_file_s param_file;

static spi_file_fd param_fd=NULL;
#define PARAM_FILE_ADDRESS FLASH_PARAM_START
#define PARAM_FILE_MAX_SIZE 4096

#define PARAM_FILE_BAK_ADDRESS (FLASH_PARAM_START+0x3000)

#define FUNC_PARAM_FILE_ADDRESS (FLASH_PARAM_START+0X1000)
#define FUNC_PARAM_FILE_MAX_SIZE 4096

#define FUNC_PARAM_FILE_BAK_ADDRESS (PARAM_FILE_BAK_ADDRESS+0x1000)

#define FUNC_STATE_FILE_ADDRESS (FLASH_PARAM_START+0X2000)
#define FUNC_STATE_FILE_MAX_SIZE 4096

static meter_sensor_data_t sensor_data;
static params_setting_t setting_params;
static function_control_param_t func_param;



static int setting_params_from_file(spi_file_fd fd,params_setting_t * info);
static int setting_params_to_file(spi_file_fd fd,params_setting_t * info);
static int params_from_file(spi_file_fd fd,unsigned char* info,int read_szie);
static int params_to_file(spi_file_fd fd,unsigned char * info,int write_szie);

unsigned short crc16_compute(const unsigned char * p_data, unsigned int size, const unsigned short * p_crc)
{
    unsigned int i;
    unsigned short crc = (p_crc == NULL) ? 0xffff : *p_crc;

    for (i = 0; i < size; i++)
    {
        crc = (unsigned char)(crc >> 8) | (crc << 8);
        crc ^= p_data[i];
        crc ^= (unsigned char)(crc & 0xff) >> 4;
        crc ^= (crc << 8) << 4;
        crc ^= ((crc & 0xff) << 4) << 1;
    }
    
    return crc;
}


void update_soil_data(soil_sensor_data_t data){
    sensor_data.soil_data = data;
}
void update_thm_data(thm_sensor_data_t data){
    sensor_data.thm_data = data;
}
void update_lux_data(lux_sensor_data_t data){
    sensor_data.lux_data = data;
}

void init_sensor_data(){
	memset((unsigned char*)&sensor_data,0,sizeof(sensor_data));
}

void build_sensor_data(unsigned char* buff,int buff_len){

    if(buff_len<sizeof(meter_sensor_data_t)){
        DbgWarn("buffer in too small for sensor data!");
    }
    memcpy(buff,&sensor_data,sizeof(meter_sensor_data_t));
}

function_control_param_t* get_func_param(void){
	return &func_param;
}

void set_func_params(void)
{
	func_param.crc = crc16_compute((unsigned char *)&func_param,sizeof(function_control_param_t)-2,NULL);
    if(param_fd==NULL){
        param_fd = &param_file;
    }
	build_spi_fs_fd(param_fd,FUNC_PARAM_FILE_ADDRESS,MIN(sizeof(function_control_param_t),FUNC_PARAM_FILE_MAX_SIZE));
	params_to_file(param_fd,(unsigned char*)&func_param,sizeof(function_control_param_t));
	build_spi_fs_fd(param_fd,FUNC_PARAM_FILE_BAK_ADDRESS,MIN(sizeof(function_control_param_t),FUNC_PARAM_FILE_MAX_SIZE));
	params_to_file(param_fd,(unsigned char*)&func_param,sizeof(function_control_param_t));
	
}

void reset_func_param(){
	memset((unsigned char*)&func_param,0,sizeof(function_control_param_t));

	func_param.heartbeat_interval=90;
	func_param.link_check_interval=60;
	func_param.link_continue_interval=300;
	func_param.time_report_accon_interval = 120;
	func_param.time_report_accoff_interval = 600;
}

void init_func_params(void){
	int ret;
	int flag=0;
	function_control_param_t func_param_tmp;
	memset((unsigned char*)&func_param_tmp,0,sizeof(function_control_param_t));
	if(param_fd==NULL){
        param_fd = &param_file;
    }
	DbgFuncEntry();
    build_spi_fs_fd(param_fd,FUNC_PARAM_FILE_ADDRESS,MIN(sizeof(function_control_param_t),FUNC_PARAM_FILE_MAX_SIZE));
	ret = params_from_file(param_fd,(unsigned char*)&func_param_tmp,sizeof(function_control_param_t));
	DbgWarn("SHINKI::%s:ret=[%x]",__FUNCTION__, ret);
	if(ret ==0 ){
		build_spi_fs_fd(param_fd,FUNC_PARAM_FILE_BAK_ADDRESS,MIN(sizeof(function_control_param_t),FUNC_PARAM_FILE_MAX_SIZE));
		ret = params_from_file(param_fd,(unsigned char*)&func_param_tmp,sizeof(function_control_param_t));
		DbgWarn("SHINKI::%s:ret1=[%x]",__FUNCTION__, ret);
		if(ret>0)
		{
			DbgWarn("FUNC PARAM USING BACKUP FILE NOW!!!");
		}
	}
	flag = ret;
	if(flag==0){
		//init func param here
		reset_func_param();
		set_func_params();
	}else{
		memcpy((void *)&func_param,(void *)&func_param_tmp,sizeof(function_control_param_t));
	}
	//check over param	
	if(func_param.update_policy >=3){ func_param.update_policy = 0;}
	if(func_param.update_scheme >3){ func_param.update_scheme = 0;}
	if(func_param.link_policy >3){ func_param.link_policy = 0;}
	if(func_param.heartbeat_interval <30){ func_param.heartbeat_interval = 60;}
	if(func_param.link_check_interval <10 || func_param.link_check_interval >300){ func_param.link_check_interval = 30;}
	if(func_param.link_continue_interval <180){ func_param.link_continue_interval = 180;}
	if(func_param.time_report_accoff_interval <30){ func_param.time_report_accoff_interval = 60;}
	if(func_param.time_report_accon_interval <30){ func_param.time_report_accon_interval = 60;}

	if(func_param.update_scheme == 1 && strlen((char*)func_param.update_host0_spec)==0){func_param.update_scheme =0;}
	if(func_param.update_scheme == 2 && strlen((char*)func_param.update_host1_spec)==0){func_param.update_scheme =0;}
	if(func_param.update_scheme == 3 && strlen((char*)func_param.update_host1_spec)==0 
		&& strlen((char*)func_param.update_host0_spec)==0)
	{
		func_param.update_scheme =0;
	}
}

static void debug_setting_params(void){

}

params_setting_t *  get_setting_params(void){
    return &setting_params;
}

void update_io_data(int offset,unsigned char flag){
	if(setting_params.io_data[offset]!=flag){
		setting_params.io_data[offset]=flag;
		set_setting_params();
	}
}

unsigned char get_io_data(int offset){
	return setting_params.io_data[offset];
}
#define OUT_IO_OFFSET	0
unsigned char output_flag[2]={0};
int set_output_state(unsigned char chn,unsigned char flag){
	if(chn<=0|| chn >2){return -1;}
	output_flag[chn-1] = flag;
	update_io_data(OUT_IO_OFFSET+(chn-1),flag);
	return 0;
}

int get_ouput_state(unsigned char chn)
{
	if(chn<=0|| chn >2){return -1;}
	output_flag[chn-1] = get_io_data(OUT_IO_OFFSET+(chn-1));
	return output_flag[chn-1]&0xFF;
}

void init_io_setting(){
	init_update_output(1,get_io_data(OUT_IO_OFFSET));
	init_update_output(2,get_io_data(OUT_IO_OFFSET+1));
}

void get_device_info(device_info_t * info)
{
	memset((unsigned char*)info,0,sizeof(device_info_t));
	info->mtd_id = atoi((char*)setting_params.mtdid);
	rt_strncpy((char*)info->imei_id,(char*)setting_params.imei,MAX_IMEI_CHARS_SIZE);
}

void clr_device_info(){
	if(strlen((char*)setting_params.mtdid)>0){
		memset((unsigned char *)&setting_params.mtdid[0],0,sizeof(setting_params.mtdid));
		memset((unsigned char *)&setting_params.imei[0],0,sizeof(setting_params.imei));
		set_setting_params();
	}
}

void set_device_info(device_info_t * info){
	memset((unsigned char *)&setting_params.mtdid[0],0,sizeof(setting_params.mtdid));
	memset((unsigned char *)&setting_params.imei[0],0,sizeof(setting_params.imei));
	
	rt_snprintf((char *)&setting_params.mtdid[0],sizeof(setting_params.mtdid),"%d",info->mtd_id);
	rt_strncpy((char*)setting_params.imei,info->imei_id,MAX_IMEI_CHARS_SIZE);
	set_setting_params();
}

void get_hw_version(char * buff)
{
	//memset(buff,0x00,MAX_HW_VERSION_SIZE);
	memcpy(buff,HW_VERSION_APP,strlen(HW_VERSION_APP)+1);
	//DbgWarn("CRC_VER:%s",CORE_VERSION_570);
}

void set_setting_params(void)
{
	setting_params.crc = crc16_compute((unsigned char *)&setting_params,sizeof(params_setting_t)-2,NULL);
    if(param_fd==NULL){
        param_fd = &param_file;
    }
	build_spi_fs_fd(param_fd,PARAM_FILE_ADDRESS,MIN(sizeof(params_setting_t),PARAM_FILE_MAX_SIZE));
	setting_params_to_file(param_fd,&setting_params);
	build_spi_fs_fd(param_fd,PARAM_FILE_BAK_ADDRESS,MIN(sizeof(params_setting_t),PARAM_FILE_MAX_SIZE));
	setting_params_to_file(param_fd,&setting_params);
	
	debug_setting_params();
}

void reset_setting_param(){
	setting_params.location_report_policy=0;
	setting_params.location_report_scheme=0;
	
	setting_params.time_report_accon_interval = 60;
	setting_params.time_report_accoff_interval = 300;
	
	memcpy((unsigned char*)&setting_params.main_server_ipaddr[0],DEFAULT_GB905_HOST,strlen(DEFAULT_GB905_HOST));
	setting_params.main_server_tcp_port = DEFAULT_GB905_PORT;
}

void init_setting_params(void)
{
	int ret;
	int flag;
	params_setting_t setting;
	memset((unsigned char*)&setting,0,sizeof(params_setting_t));
    if(param_fd==NULL){
        param_fd = &param_file;
    }

	DbgFuncEntry();
    build_spi_fs_fd(param_fd,PARAM_FILE_ADDRESS,MIN(sizeof(params_setting_t),PARAM_FILE_MAX_SIZE));
	ret = setting_params_from_file(param_fd,&setting);
	DbgWarn("SHINKI::%s:ret=[%x]",__FUNCTION__, ret);
	if(ret ==0 ){
		build_spi_fs_fd(param_fd,PARAM_FILE_BAK_ADDRESS,MIN(sizeof(params_setting_t),PARAM_FILE_MAX_SIZE));
		ret = setting_params_from_file(param_fd,&setting);
		DbgWarn("SHINKI::%s:ret1=[%x]",__FUNCTION__, ret);
		if(ret>0)
		{
			DbgWarn("PARAM USING BACKUP FILE NOW!!!");
		}
	}

	flag = ret;

	DbgWarn("SHINKI::%s:flag=[%x]",__FUNCTION__, flag);
	if(flag==0){
			reset_setting_param();
			set_setting_params();
	}
	else{
			memcpy((void *)&setting_params,(void *)&setting,sizeof(params_setting_t));
	}

	if(setting_params.location_report_policy>2)
	{
		setting_params.location_report_policy=0;
	}
	if(setting_params.location_report_scheme>3)//
	{
		setting_params.location_report_scheme=0;
	}
	if(setting_params.time_report_accon_interval<5)
	{
		setting_params.time_report_accon_interval = 60;
	}
	if(setting_params.time_report_accoff_interval<5)
	{
		setting_params.time_report_accoff_interval = 300;
	}
	#if 1
	if(strlen((char*)setting_params.main_server_ipaddr)==0||setting_params.main_server_tcp_port==0)
	{
		memset((unsigned char*)&setting_params.main_server_ipaddr[0],0,sizeof(setting_params.main_server_ipaddr));
		memcpy((unsigned char*)&setting_params.main_server_ipaddr[0],DEFAULT_GB905_HOST,strlen(DEFAULT_GB905_HOST));
		setting_params.main_server_tcp_port = DEFAULT_GB905_PORT;
		
	}
	#endif
    setting_params.location_report_policy =0;
    setting_params.location_report_scheme=0;

	debug_setting_params();
	DbgFuncExit();
}

static int setting_params_from_file(spi_file_fd fd,params_setting_t * info)
{
	return params_from_file(fd,(unsigned char*)info,sizeof(params_setting_t));
}

static int setting_params_to_file(spi_file_fd fd,params_setting_t * info)
{
	return params_to_file(fd,(unsigned char*)info,sizeof(params_setting_t));
}


static int params_from_xcrc_file(spi_file_fd fd,unsigned char* info,int read_szie,int crc_flag)
{
    int fd_ret=0;
	size_t	size;
	unsigned short crc;
	unsigned short data_crc;
	int ret = 1;

	DbgFuncEntry();


	fd_ret = spi_fs_open(fd,SPI_FS_MODE_RO);
	if(fd_ret<=0)
	{
		DbgError("fopen failed!");
		return 0;
	}
	size = spi_fs_read(fd,(unsigned char*)info,read_szie);
	if(size<=0)
	{
        DbgError(" setting struct old crc error!!!!!");
        memset(info,0x00,read_szie);
        //DbgWarn("info->crc=%x,crc=[%x], error!",info->crc,crc);
        ret = 0;
	}
	else
	{
		if(crc_flag)
		{
			crc = crc16_compute((unsigned char *)info,read_szie-2,NULL);
			data_crc = *((unsigned short*)(info+read_szie-2));
			if(data_crc != crc)
			{
				DbgError(" setting struct old new error!!!!!");
				memset(info,0x00,read_szie);
				//DbgWarn("info->crc=%x,crc=[%x], error!",info->crc,crc);
				ret = 0;
			}
		}else{
			if(info[0]==0x00||(info[0]==0xFF&&info[1]==0xFF&&info[2]==0xFF&&info[3]==0xFF)){
				DbgError("data  info 0xFFFFFFF or string empty detect, maybe error!");
				ret=0;
			}
		}

	}
    //sync file
	spi_fs_close(fd);
	DbgFuncExit();
	
	
	return ret;
}

static int params_from_file(spi_file_fd fd,unsigned char* info,int read_szie)
{
	return params_from_xcrc_file(fd,info,read_szie,1);
}

static int params_to_file(spi_file_fd fd,unsigned char * info,int write_szie)
{
	int fd_ret=0;
	size_t	size;
	int ret = 1;
	DbgFuncEntry();

	fd_ret = spi_fs_open(fd,SPI_FS_MODE_RW);
	if(fd_ret<=0)
	{
		DbgError("fopen failed!");
		return 0;
	}
	
	size = spi_fs_write(fd,(unsigned char*)info,write_szie);

	DbgPrintf("size = 0x%x",size);

	if(size<0)
	{
		DbgError("write info error!");
		ret = 0;
	}
	spi_fs_close(fd);
	DbgFuncExit();
	
	
	return ret;
}

S2jHook rt_s2jHook = {
        .malloc_fn = rt_malloc,
        .free_fn = rt_free,
};

void save_satte_file(){
	char* json_string= build_state_json();
	LOG_W(json_string);
	if(param_fd==NULL){
        param_fd = &param_file;
    }
	build_spi_fs_fd(param_fd,FUNC_STATE_FILE_ADDRESS,MIN(strlen(json_string)+1,FUNC_STATE_FILE_MAX_SIZE));
	params_to_file(param_fd,(unsigned char*)json_string,strlen(json_string)+1);
	rt_free(json_string);
}

void load_state_file(){
	int ret;
	int buffer_size = 512;
	char* json_string = rt_malloc(buffer_size);
	memset(json_string,0,buffer_size);

	build_spi_fs_fd(param_fd,FUNC_STATE_FILE_ADDRESS,MIN(buffer_size,FUNC_STATE_FILE_MAX_SIZE));
	ret = params_from_xcrc_file(param_fd,(unsigned char*)json_string,buffer_size,0);
	if(ret){
		parse_state_json(json_string);
	}

	rt_free(json_string);
}


void setting_init()
{
	s2j_init(&rt_s2jHook);
	init_setting_params();
	init_func_params();
	init_io_setting();
	init_sensor_data();
	load_state_file();
	
}
