
#include <rtconfig.h>

#define DBG_LEVEL           DBG_WARNING
#define DBG_SECTION_NAME    "BSP"
#include <rtdbg.h>

#include <rtthread.h>
#include "pin.h"
#include <business.h>

#include "bsp.h"
#define MAX_IMEI_SIZE 16
#define MAX_ICCID_SIZE 32
unsigned char imei_data[MAX_IMEI_SIZE];
unsigned char iccid_data[MAX_ICCID_SIZE];
typedef  union{
	uint8_t uid[UID_LENGTH];
	struct{
	uint16_t x_bcd;
	uint16_t y_bcd;
	uint8_t w_hex;
	uint8_t serial[7];
	}uuid;
}uuid_n32_t;
unsigned int  build_uuid(unsigned char* buff,int buff_size)
{
	uint32_t uuid=0XFFFFFFFF;
	uint16_t x,y;
	uint8_t w;
	uint32_t serial1;
	uint32_t serial2;
	uuid_n32_t n32_uid;
	memset(&n32_uid,0,UID_LENGTH);
	
	memcpy(&n32_uid.uid[0],(void*)UID_BASE,UID_LENGTH);
	
	x = bcd2decimal(&n32_uid.uid[0],2)&0xFFFF;
	y = bcd2decimal(&n32_uid.uid[2],2)&0xFFFF;
	w = n32_uid.uid[4];
	serial1 = (n32_uid.uid[5]<<16)+(n32_uid.uid[6]<<8)+n32_uid.uid[7];
	serial2 = (n32_uid.uid[8]<<24)+(n32_uid.uid[9]<<16)+(n32_uid.uid[10]<<8)+n32_uid.uid[11];
	uuid = (((((x*y)+w) +((serial1<<8)&(serial2))))&0x7FFFFFFF);
	
	rt_snprintf((char*)buff,buff_size,"%ld",uuid%10000000000);
	return strlen((char*)buff);
}

void update_imei(char* imei){
	if(strlen(imei)<=0) return;
	memset(imei_data,0,sizeof(imei_data));
	memcpy(imei_data,imei,MIN(strlen(imei),MAX_IMEI_SIZE));
}
void get_imei(char* buff,int buffer_size){
	memcpy(buff,imei_data,MIN(MAX_IMEI_SIZE,buffer_size));
}

void update_iccid(char* iccid){
	if(strlen(iccid)<=0) return;
	memset(iccid_data,0,sizeof(iccid_data));
	memcpy(iccid_data,iccid,MIN(strlen(iccid),MAX_ICCID_SIZE));
}
void get_iccid(char* buff,int buffer_size){
	memcpy(buff,iccid_data,MIN(MAX_ICCID_SIZE,buffer_size));
}


void input_init(){
	rt_pin_mode(IN_ACC_DET_PIN, PIN_MODE_INPUT_PULLUP);
	rt_pin_mode(IN_ALARM_PIN, PIN_MODE_INPUT_PULLUP);
	rt_pin_mode(IN_DOOR_SIG_PIN, PIN_MODE_INPUT_PULLUP);
}
#define ACC_CHECK_TIME_ALL		(400)
#define ACC_CHECK_MS_DELAY	(20)
#define ACC_CHECK_THO_TIME	(320)

#define ACC_CHECK_LOOP_COUNT (ACC_CHECK_TIME_ALL/ACC_CHECK_MS_DELAY)
#define ACC_CHECK_THO_COUNT (ACC_CHECK_THO_TIME/ACC_CHECK_MS_DELAY)

static char acc_on_check_count=0;
static char acc_off_check_count=0;
static rt_uint32_t last_check_ms=0;

static int check_acc_state()
{
	int temp_acc =-1;
	static int last_acc=-1;
	int accon_count =0;
	int accoff_count =0;
	int i;
	//500 ms check acc state
	for(i=0;i<ACC_CHECK_LOOP_COUNT;i++)
	{
		temp_acc = rt_pin_read(IN_ACC_DET_PIN);
		if(temp_acc){accoff_count++;}
		else{accon_count++;}
		rt_thread_mdelay(ACC_CHECK_MS_DELAY);
	}
	if(accon_count>=ACC_CHECK_THO_COUNT){ last_acc = 1;}
	else if(accoff_count >=ACC_CHECK_THO_COUNT){last_acc = 0;}
	
	if(last_acc==-1){
		last_acc = rt_pin_read(IN_ACC_DET_PIN)?0:1;
	}
	if(last_check_ms==0){
		last_check_ms = OSTimeGetMS();
	}
	
	return last_acc;
}

static int acc_check_new()
{
	int temp_acc =-1;
	if(OSTimeGetMS()-last_check_ms >=ACC_CHECK_MS_DELAY){
		temp_acc = rt_pin_read(IN_ACC_DET_PIN);
		if(temp_acc){acc_on_check_count++;}
		else{acc_off_check_count++;}
	}
	if(acc_on_check_count>=ACC_CHECK_THO_COUNT){
		acc_on_check_count=0;
		acc_off_check_count=0;
		return ACC_ON_DETECT;
	}else if(acc_off_check_count>=ACC_CHECK_THO_COUNT){
		acc_on_check_count=0;
		acc_off_check_count=0;
		return ACC_OFF_DETECT;
	}
	return ACC_NOCHANGE;
}

//return: <0:erroe ,0:nochange,1:acc_off,2:acc_on
int acc_state_poll(char mode)
{
	int acc_value=-1;
	if(mode==0){//old mode
		if(check_acc_state())
		{
			return 2;
		}else{
			return 1;
		}
	}else{
		return acc_check_new();
	}
	return acc_value;
}

static int is_debug=1;
static char link_state=0;
static int link_ms_tick=1000;

void disable_swd_setup_led()
{
	//disable swd
	rt_pin_mode(LED1_SCLK_PIN, PIN_MODE_OUTPUT_OD);
	rt_pin_mode(LED2_SWIO_PIN, PIN_MODE_OUTPUT);
	is_debug=0;
}
static void wd_real_feed()
{
	static uint8_t flag=0;
	#if DISABLE_SWD_DEBUG
	if(!is_debug){
		rt_pin_write(LED1_SCLK_PIN,flag%2?PIN_HIGH:PIN_LOW);
		flag++;
	}
	#endif
}


void wd_feed(){
	static int last_ms_tick=0;
	if(last_ms_tick==0||OSTimeGetMS()-last_ms_tick>500){
		last_ms_tick=OSTimeGetMS();
		wd_real_feed();
	}
}
	

static void led_link_change()
{
	static uint8_t flag=0;
	
	#if DISABLE_SWD_DEBUG
	if(!is_debug){
		LOG_D("led link change!!!");
		rt_pin_write(LED2_SWIO_PIN,flag%2?PIN_HIGH:PIN_LOW);
		flag++;
	}
	#endif
}

static void led_link_close()
{
	
	#if DISABLE_SWD_DEBUG
	if(!is_debug){
		LOG_D("led link close!!!");
		rt_pin_write(LED2_SWIO_PIN,PIN_LOW);
	}
	#endif
}

void led_link_handle(){
	static int last_ms_tick=0;
	
		if(last_ms_tick==0||OSTimeGetMS()-last_ms_tick>link_ms_tick){
			last_ms_tick=OSTimeGetMS();
			if(link_state>0){
				led_link_change();
			}else{
				led_link_close();
			}
	}
}

void led_link_set(unsigned char mode)
{
	if(mode==LED_CLOSE){//close
		link_state =0;
		link_ms_tick =500;
	}else if(mode==LED_SLOW){//slow
		link_state =1;
		link_ms_tick =500;
	}else if(mode==LED_FAST){//fast
		link_state =1;
		link_ms_tick =200;
	}else{//long slow LED_LONGSLOW
		link_state =0;
		link_ms_tick =2000;
	}

}

rt_uint32_t adc_pwr_value = 0;
rt_uint32_t adc_ext_value = 0;

void update_adc()
{
	rt_uint32_t adc_value = 0;
	adc_value = get_pwr_mv();
	get_ext_mv();
	if(adc_value >10*1000){
		#if DISABLE_SWD_DEBUG
		disable_swd_setup_led();
		#endif
	}
}
static unsigned int get_adc_mv(rt_uint32_t* p_value,rt_uint32_t channel)
{
	rt_uint32_t adc_converted_value = 0;
	rt_adc_device_t adc_dev;
	adc_dev = (rt_adc_device_t)rt_device_find("adc");
	if(adc_dev!=RT_NULL){
	adc_converted_value = rt_adc_read(adc_dev, channel);
		LOG_I("pwr_adc[%d]=%d",channel,adc_converted_value);
	*p_value = (uint32_t)((ADC_mV1_K * adc_converted_value/4096.0));
	}
	return *p_value;
}

unsigned int  get_pwr_mv(){
	return get_adc_mv(&adc_pwr_value,PWR_ADC_CHN);
}

unsigned int  get_ext_mv(){
	return get_adc_mv(&adc_ext_value,EXT_ADC_CHN);
}


void init_adc_config(){
	
	rt_adc_device_t adc_dev;
	adc_dev = (rt_adc_device_t)rt_device_find("adc");
	if(adc_dev!=RT_NULL){
	rt_adc_enable(adc_dev, PWR_ADC_CHN);
	rt_adc_enable(adc_dev, EXT_ADC_CHN);
	update_adc();
	}
}

void output_init(){
	rt_pin_mode(OUT_LOCK_PIN, PIN_MODE_OUTPUT);
	rt_pin_mode(OUT_GEN1_PIN, PIN_MODE_OUTPUT);
	//default 
	rt_pin_write(OUT_LOCK_PIN, PIN_LOW);
	rt_pin_write(OUT_GEN1_PIN, PIN_LOW);
}

void init_update_output(int chn,int level){
	if(chn>2||chn<=0) return;
	if(chn==1)
	{
		rt_pin_write(OUT_LOCK_PIN, level?PIN_HIGH:PIN_LOW);
	}
	else if(chn ==2){
		rt_pin_write(OUT_GEN1_PIN, level?PIN_HIGH:PIN_LOW);
	}
}

void control_output(int chn,int level)
{
	if(chn>2||chn<=0) return;
	if(chn==1)
	{
			rt_pin_write(OUT_LOCK_PIN, level?PIN_HIGH:PIN_LOW);
			set_output_state(chn,level);
	}
	else if(chn ==2){
		rt_pin_write(OUT_GEN1_PIN, level?PIN_HIGH:PIN_LOW);
		set_output_state(chn,level);
	}
}

void inout_init(){
	rt_pin_mode(INOUT_GEN1_PIN, PIN_MODE_OUTPUT_OD);
	rt_pin_mode(INOUT_GEN2_PIN, PIN_MODE_OUTPUT_OD);
}

void ext_power_init(){
	rt_pin_mode(EXT_POWER1_PIN, PIN_MODE_OUTPUT);
	rt_pin_mode(EXT_POWER2_PIN, PIN_MODE_OUTPUT);
	
	//init
	rt_pin_write(EXT_POWER1_PIN, PIN_HIGH);
	rt_pin_write(EXT_POWER2_PIN, PIN_HIGH);
}



void spk_init(){
		rt_pin_mode(SPK_MUTE_PIN, PIN_MODE_OUTPUT);
	//init
		rt_pin_write(SPK_MUTE_PIN, PIN_HIGH);
}

//WARNING: this init for hard pin only before all other device init ,
//DONT! USE! device function here ,no device config init before this
void bsp_init(){
	input_init();
	output_init();
	inout_init();
	ext_power_init();
	spk_init();
}