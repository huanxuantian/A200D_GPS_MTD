#define DBG_LVL    DBG_WARNING
#define DBG_TAG    "GSM"
#include <rtdbg.h>
#include <rtthread.h>

#include "pin.h"

#include <utils.h>
#include "gnss_port.h"
#include "system_bsp.h"
#include "gsm.h"
#include "cip_sockmgr.h"

#include <protocol.h>

#include <business.h>
ALIGN(RT_ALIGN_SIZE)
static rt_uint8_t gsm_stack[ 2048 ];
static struct rt_thread gsm_thread;
ALIGN(1)
typedef enum
{
    MD_RESET,        				//复位模块
    MD_AT_REQ,       				//AT握手
    MD_WORK_STA_CHK, 				//工作状态检测
    MD_CONNETINIT,   				//连接配置信息初始化
    MD_CONNETED,     				//数据通信
	MD_FLIGHTMODE,   				//飞行模式
    MD_OK = 0xFE,    				//正常
    MD_ERR = 0xFF,   				//异常
} MD_RUN_STATE;

typedef enum{
  MD_RUN_STEP0=0,
  MD_RUN_STEP1,
  MD_RUN_STEP2,
  MD_RUN_STEP3,
  MD_RUN_STEP4,
  MD_RUN_STEP5,
  MD_RUN_STEP6,
  MD_RUN_STEP7,
  MD_RUN_STEP8,
  MD_RUN_STEP9,
  MD_RUN_STEPA,
  MD_RUN_STEPB,
  MD_RUN_STEPC,
  MD_RUN_STEPD,
  MD_RUN_STEPE,
  MD_RUN_STEPF,
  MD_RUN_OK = 0xFE,    				//正常
  MD_RUN_ERR = 0xFF,   				//异常
} MD_RUN_STEP;

#define LTE_POWER_ON_WAIT_TIME 2000 										//LTE开机等待时间
#define SIGNALMIN 15                                    //信号质量最低阀值
#define SIGNALMAX 31                                    //信号质量最低阀值


MD_RUN_STEP ucStateNum = MD_RUN_STEP0;                                 //命令执行顺序标识值
uint8_t retrytimes = 0;                                 //命令重试次数
uint8_t ucErrorTimes = 0;                               //错误次数累计值
uint8_t ucFlightModeTimes = 0;															//进入飞行模式次数

uint32_t  last_print_tick=0;

uint32_t  last_time_tick=0;

void init_gsm_io()
{
    rt_pin_mode(OUT_GSM_PKEY_PIN, PIN_MODE_OUTPUT);
	rt_pin_mode(OUT_GSM_DTR_PIN, PIN_MODE_OUTPUT);
    rt_pin_mode(OUT_GSM_PEN_PIN, PIN_MODE_OUTPUT);
    rt_pin_mode(OUT_GSM_WAKEUP_PIN, PIN_MODE_OUTPUT);
    rt_pin_mode(OUT_GSM_AT_READY_PIN, PIN_MODE_OUTPUT);
    //input
    rt_pin_mode(IN_GSM_RING_PIN, PIN_MODE_INPUT_PULLUP);
	
	//pwr on wakeup try
	rt_pin_write(OUT_GSM_PEN_PIN, PIN_HIGH);
	rt_pin_write(OUT_GSM_DTR_PIN, PIN_LOW);
	rt_pin_write(OUT_GSM_PKEY_PIN, PIN_HIGH);
	
	
}

void gsm_prst(int mdelay){
    if(mdelay<300) mdelay =500;
    rt_pin_write(OUT_GSM_PKEY_PIN, PIN_HIGH);
    rt_thread_mdelay(50);
    rt_pin_write(OUT_GSM_PKEY_PIN, PIN_LOW);
    rt_thread_mdelay(mdelay);
    rt_pin_write(OUT_GSM_PKEY_PIN, PIN_HIGH);
}
void gsm_hard_rst(){
    gsm_prst(1500);
}
void gsm_soft_rst(){
    gsm_prst(500);
}

static int gsm_paser_callback(gnss_ctx_t *ctx,int read_len,int ack_flag){
    return cip_paser_callback(ctx,read_len,ack_flag);
}

void wait_gsm_ready(gnss_device_t gsm_dev){
	int count =0;
	while(send_AT_CMD(gsm_dev,"ATE0","OK",1500)==0){
		if(at_cmdres_keyword_matching("OK")) {
			LOG_W("at check ready OK");
			break;
		}
		rt_thread_mdelay(1000);
		count++;
		if(count>20){
			LOG_E("at check ready failed");
			break;
		}
	}
}


/**
 * @description:复位LTE模块
 * @param None
 * @return None
 */
static int module_reset(gnss_device_t gsm_dev)
{
    switch (ucStateNum)
    {
    //拉低PEN引脚,拉高PEN引脚
    case MD_RUN_STEP0:
		gsm_hard_rst();
        ucStateNum = MD_RUN_STEP1;
        last_time_tick = OSTimeGet();
        break;
    //查询是否READY
    case MD_RUN_STEP1:
        if (OSTimeGet()-last_time_tick>1)
        {
			wait_gsm_ready(gsm_dev);
            ucStateNum = MD_RUN_STEP2;
            last_time_tick = OSTimeGet();
        }
        break;
    //再次验证AT
    case MD_RUN_STEP2:
        if (OSTimeGet()-last_time_tick>2)
        {
            if (send_AT_CMD(gsm_dev,"AT","OK", 1000)==1)
            {
                if(at_cmdres_keyword_matching("OK")) {
                    ucErrorTimes = 0;
                    ucStateNum = MD_RUN_STEP0;
                    return 1;
                }

            }else{
                ucErrorTimes++;
            }
            last_time_tick = OSTimeGet();
            if (ucErrorTimes++ > 10){
                ucStateNum=MD_RUN_STEP0;
            }

        }
        break;
    default:
        break;
    }
    return 0;
}

static int match_imei(char* str){
    int i=0;
    char tmp[20] = {0};
    char *p = NULL, *q = NULL;

    p=str;
    //jump to number start
    while ((*p < 0x30 || *p > 0x39) && p < str+strlen(str))
    {
        p++;
    }
    //check end or not 
    if(p < str+strlen(str))
	{
        q = strstr(p, "OK");
        //check copy max 16 number only
        while(p<q && i<=16 && *p!='\r'){
            if(*p >=0x30 && *p <=0x39){
                tmp[i]= *p;
                i++;
            }
            p++;
        }
        if(strlen(tmp)>0){
            LOG_W("parse imei OK,imei=%s",tmp);
            update_imei(tmp);
            return 1;
        }

    }
    return 0;
    
}

static int match_iccid(char* str){
    int i=0;
    char tmp[32] = {0};
    char *p = NULL, *q = NULL;

    p = strstr(str, "+ICCID: ");
		if(p){
			p =p + 8;

			q = strstr(p,"OK");

			while(p<q  && *p!='\r'){
					if(*p >=0x30 && *p <=0x39){
							tmp[i]= *p;
							i++;
					}
					p++;
			}
			if(strlen(tmp)>0){
                    update_iccid(tmp);
					LOG_W("parse iccid OK,iccid=%s",tmp);
					return 1;
			}
		}
    return 0;

}

/**
 * @description: 检测获取模块信息
 * @param None
 * @return 0：检测未完成；MD_OK：模块已就绪；MD_ERR：错误，不满足工作状态
 */
static int module_info(gnss_device_t gsm_dev)
{
    switch (ucStateNum)
    {
    //AT+CGSN获取IMEI
    case MD_RUN_STEP0:
        if (send_AT_CMD(gsm_dev,"AT+CGSN","OK", 2000)==1){
            if(at_cmdres_keyword_matching("OK")) {
                if(match_imei(get_at_recv_ptr())){
                    //parse CGSN here
                    ucErrorTimes = 0;
                    ucStateNum = MD_RUN_STEP1;
                }else{
                    //发送10次得不到正确应答
                    if (ucErrorTimes++ > 10)
                    {
                        ucStateNum = MD_RUN_ERR;
                    }
					rt_thread_mdelay(500);
                }

            }else{
                //发送10次得不到正确应答
                if (ucErrorTimes++ > 10)
                {
                    ucStateNum = MD_RUN_ERR;
                }
								rt_thread_mdelay(1000);
            }
        }
        break;
    //ATI获取厂家信息（目前未解析和使用）
    case MD_RUN_STEP1:
        if (send_AT_CMD(gsm_dev,"ATI","OK", 8000)==1){
            if(at_cmdres_keyword_matching("OK")) {
                //parse ATI here
                ucErrorTimes = 0;
                ucStateNum = MD_RUN_STEP2;
            }else{
                //发送10次得不到正确应答
                if (ucErrorTimes++ > 10)
                {
                    ucStateNum = MD_RUN_ERR;
                }
								rt_thread_mdelay(1000);
            }
        }
        break;
    //CPIN 查看SIM卡状态
    case MD_RUN_STEP2:
        if (send_AT_CMD(gsm_dev,"AT+CPIN?","+CPIN: READY", 8000)==1){
            if (at_cmdres_keyword_matching("+CPIN: READY")){
                ucErrorTimes = 0;
                ucStateNum = MD_RUN_STEP3;
            }
            else
            {    
				//发送10次得不到正确应答
                if (ucErrorTimes++ > 10)
                {
                    ucStateNum = MD_RUN_ERR;
                }
								rt_thread_mdelay(1000);
            }
        }
        break;
    //CICCID获取SIM卡的唯一ID
    case MD_RUN_STEP3:
        if (send_AT_CMD(gsm_dev,"AT+CICCID","+ICCID: ", 3000)==1){
            if (at_cmdres_keyword_matching("+ICCID: ")){
                if(match_iccid(get_at_recv_ptr())){
                    ucErrorTimes = 0;
                    ucStateNum = MD_RUN_OK;//last step finish
                }else{
                    //发送10次得不到正确应答
                    if (ucErrorTimes++ > 10)
                    {
                        ucStateNum = MD_RUN_ERR;
                    }
                    rt_thread_mdelay(500);                 
                }

            }
            else
            {    
				//发送10次得不到正确应答
                if (ucErrorTimes++ > 10)
                {
                    ucStateNum = MD_RUN_ERR;
                }
				rt_thread_mdelay(1000);
            }
        }
        break;
    		//错误跳至RESET 未检测到SIM卡或者AT信息获取错误
	case MD_RUN_ERR:
        ucStateNum = MD_RUN_STEP0;
        return MD_ERR;
    //完成
    case MD_RUN_OK:
		ucStateNum = MD_RUN_STEP0;
        return MD_OK;
    default:
        break;
    }
    return 0;
}

/**
 * @description:
 * @param str：要检索的字符串
 * @param minval：要匹配信号质量区间最小值
 * @param minval：要匹配信号质量区间最大值
 * @return 0:信号质量不满足正常工作状态, 1:信号质量满足正常工作状态
 */
static int match_csq(char *str, int minval, int maxval)
{
    int lpCsqVal = 0;
    char tmp[5] = {0};
    char *p = NULL, *q = NULL;
		
    p = strstr(str, "+CSQ:");
    if (p == NULL)
    {
        return 0;
    }
		
    p = p + 5;
		
    while (*p < 0x30 || *p > 0x39)
    {
        p++;
    }
    q = p;

    while (*p != ',')
    {
        p++;
    }
    memcpy(tmp, q, p - q);
    lpCsqVal = atoi(tmp);
    /* 判断信号质量是否在设置的区间内 */
    if (lpCsqVal >= minval && lpCsqVal <= maxval)
    {
        return 1;
    }
    return 0;
}

/**
 * @description: 检测模块工作状态是否就绪
 * @param None
 * @return 0：检测未完成；MD_OK：模块已就绪；MD_ERR：错误，不满足工作状态
 */
static int module_is_ready(gnss_device_t gsm_dev)
{
    switch (ucStateNum)
    {
    //关闭AT命令回显
    case MD_RUN_STEP0:
        if (send_AT_CMD(gsm_dev,"ATE0","OK", 2000))
        {
						//收到OK
            if (at_cmdres_keyword_matching("OK"))
            {
                ucErrorTimes = 0;
                ucStateNum=MD_RUN_STEP1;
            }
            else
            {
								//发送10次得不到正确应答
                if (ucErrorTimes++ > 50)
                {
                    ucStateNum = MD_RUN_ERR;
                }
								rt_thread_mdelay(500);
            }
        }
        break;
    //读信号注册状态
    case MD_RUN_STEP1:
        if (send_AT_CMD(gsm_dev,"AT+CGREG?","+CGREG: 0,1", 5000)==1){
            if (at_cmdres_keyword_matching("+CGREG: 0,1")){
                ucErrorTimes = 0;
                ucStateNum=MD_RUN_STEP2;
            }
            else
            {    
				//发送10次得不到正确应答
                if (ucErrorTimes++ > 30)
                {
                    ucStateNum = MD_RUN_ERR;
                }
								rt_thread_mdelay(1000);
            }
        }
        break;
    //查询信号质量
    case MD_RUN_STEP2:
        if (send_AT_CMD(gsm_dev,"AT+CSQ","+CSQ:", 3000))
        {
						//收到OK
            if (at_cmdres_keyword_matching("OK"))
            {
                //收到的是99（射频信号未初始化）
                if (at_cmdres_keyword_matching("+CSQ: 99,99"))
                {
                        //发送30次得不到正确应答
                        if (ucErrorTimes++ > 30)
                        {
                                ucStateNum = MD_RUN_ERR;
                        }
                }
                else
                {    
					//信号值在SIGNALMIN~SIGNALMAX这个区间
                    if (match_csq(get_at_recv_ptr(), SIGNALMIN, SIGNALMAX))
                    {
                        ucErrorTimes = 0;
						ucStateNum = MD_RUN_STEP3;
                    }else{
						ucStateNum = MD_RUN_ERR;
					}
                }
            }
			//没收到应答
            else
            {
								//发送30次不应答
                if (ucErrorTimes++ > 30)
                {
                    ucStateNum = MD_RUN_ERR;
                }
								rt_thread_mdelay(500);
            }
        }
        break;
			//查看当前GPRS附着状态
    case MD_RUN_STEP3:
        if (send_AT_CMD(gsm_dev,"AT+CGATT?","+CGATT: 1", 8000))
        {
						//收到+CGATT: 1
            if (at_cmdres_keyword_matching("+CGATT: 1"))
            {
                ucErrorTimes = 0;
								ucStateNum = MD_RUN_OK;
								//ntp_retry(gsm_dev);
                                try_check_ntp(gsm_dev,1);
								rt_thread_mdelay(50);
            }
            else
            {       
				//发送30次得不到正确应答
                if (ucErrorTimes++ > 30)
                {
                    ucStateNum = MD_RUN_ERR;
                }
								rt_thread_mdelay(1000);
            }
        }		
				
        break;
		//错误跳至飞行模式
	case MD_RUN_ERR:
				ucStateNum = MD_RUN_STEP0;
				return MD_ERR;
    //完成
    case MD_RUN_OK:
        LOG_W("sim card cgatt init finish!");
		ucStateNum = MD_RUN_STEP0;
        return MD_OK;
    default:
        break;
    }
    return 0;
}

/**
 * @description: Socket连接相关配置初始化
 * @param None
 * @return 0：检测未完成；MD_OK：模块已就绪；MD_ERR：错误，不满足工作状态
 */
int module_connet_parm_init(gnss_device_t gsm_dev)
{
		params_setting_t * param;
    switch (ucStateNum)
    {
    //断开所有连接
    case MD_RUN_STEP0:
        if (send_AT_CMD(gsm_dev,"AT+NETCLOSE","+NETCLOSE", 1000))
        {
						//收到+NETCLOSE
            if (at_cmdres_keyword_matching("+NETCLOSE"))
            {
                ucErrorTimes = 0;
                ucStateNum=MD_RUN_STEP1;
            }
            else
            {
								//发送5次得不到正确应答
                if (ucErrorTimes++ > 10)
                {
                    ucStateNum = MD_RUN_ERR;
                }
								rt_thread_mdelay(1000);
            }
        }
        break;		
		//配置TCPIP应用为模式为非透明传输模式
    case MD_RUN_STEP1:
        if (send_AT_CMD(gsm_dev,"AT+CIPMODE=0","OK", 1000))
        {
						//收到OK
            if (at_cmdres_keyword_matching("OK"))
            {
                ucErrorTimes = 0;
                ucStateNum = MD_RUN_STEP2;
            }
            else
            {
								//发送30次得不到正确应答
                if (ucErrorTimes++ > 30)
                {
                    ucStateNum = MD_RUN_ERR;
                }
								rt_thread_mdelay(500);
            }
        }
        break;
    //开启网络服务
    case MD_RUN_STEP2:
        if (send_AT_CMD(gsm_dev,"AT+NETOPEN","+NETOPEN: 0", 3000))
        {
						//收到+NETOPEN: 0
            if (at_cmdres_keyword_matching("+NETOPEN: 0"))
            {
                ucErrorTimes = 0;
                ucStateNum = MD_RUN_STEP3;
            }else if(at_cmdres_keyword_matching("Network is already opened")){
                ucErrorTimes = 0;
                ucStateNum = MD_RUN_STEP3;							
						}else
            {
								//发送30次得不到正确应答
                if (ucErrorTimes++ > 30)
                {
                    ucStateNum = MD_RUN_ERR;
                }
								rt_thread_mdelay(1000);
            }
        }
        break;
		//查询运营商分配的 IP 地址
    case MD_RUN_STEP3:
        if (send_AT_CMD(gsm_dev,"AT+IPADDR","+IPADDR", 1000))
        {
						//收到+IPADDR
            if (at_cmdres_keyword_matching("+IPADDR"))
            {
                ucErrorTimes = 0;
                ucStateNum = MD_RUN_STEP4;
            }
            else
            {
								//发送10次得不到正确应答
                if (ucErrorTimes++ > 30)
                {
                    ucStateNum = MD_RUN_ERR;
                }
								rt_thread_mdelay(500);
            }
        }
        break;
    //设置为手动获得网络数据
    case MD_RUN_STEP4:
        if (send_AT_CMD(gsm_dev,"AT+CIPRXGET=1","OK", 5000))
        {
						//收到OK
            if (at_cmdres_keyword_matching("OK"))
            {
                ucErrorTimes = 0;
				ucStateNum = MD_RUN_STEP5;
            }
            else
            {
				//发送5次得不到正确应答
                if (ucErrorTimes++ > 30)
                {
                    ucStateNum = MD_RUN_ERR;
                }
								rt_thread_mdelay(1000);
            }
        }
        break;
	//尝试认证
    case MD_RUN_STEP5:
			led_link_set(LED_SLOW);
				send_AT_CMD(gsm_dev,"AT+CCLK?","OK", 1000);
        //TODO::先执行认证逻辑，认证完成后进入下一步
       // auth_loop();
		    ucErrorTimes = 0;
				ucStateNum = MD_RUN_STEP6;
				break;
	//建立连接
    case MD_RUN_STEP6:
				
			led_link_set(LED_FAST);
        //TODO::最终多路初始化在此处处理
		param = get_setting_params();
        #if 1
        if(get_socket_flag(0) ==1){
            ucErrorTimes = 0;
            ucStateNum = MD_RUN_OK;
        }else if(cip_sock_open(gsm_dev,0,(char*)&param->main_server_ipaddr[0],param->main_server_tcp_port)>=0)
        {
        	    ucErrorTimes = 0;
				//set_socket_flag(0,1);
               ucStateNum = MD_RUN_OK;
               LOG_W("cip_open init OK");
        }else{
        		//发送5次得不到正确应答
                if (ucErrorTimes++ > 5)
                {
                    //ucStateNum = MD_RUN_ERR;
                    ucErrorTimes = 0;
                    ucStateNum = MD_RUN_OK;
                }
				rt_thread_mdelay(200);
        }
        if(get_socket_flag(1) <=0 && ucStateNum == MD_RUN_OK)
        {
            cip_sock_open(gsm_dev,1,SEED_HOST,SEED_PORT);
        }
        #endif

        break;		
    //完成
    case MD_RUN_OK:
        LOG_W("network init finish!");
		ucStateNum = MD_RUN_STEP0;
        return MD_OK;
    //错误跳至飞行模式
    case MD_RUN_ERR:
				ucStateNum = MD_RUN_STEP0;
        return MD_ERR;
    default:
        break;
    }
    return 0;
}
//return hadled_size 
int gateway_handle(int index,void* data,int data_size){
	int size;

    switch(index){
        case 0:
            size = gb905_protocol_anaylze(index,data,data_size);
            break;
        case 1:
            size = gb905_protocol_anaylze(index,data,data_size);
            break;
        case 2:
            size = auth_protocol_anaylze(index,data,data_size);
			break;
        default:
            LOG_W("gateway_handle[%d]:",index);
            debug_print_block_data(0,data,data_size);
            size = data_size;
            break;
    }
    
	return size;
}
typedef struct {
   	uint32_t last_recv_tick;
    uint32_t last_send_tick;
	uint32_t last_beat_tick;
    uint32_t last_err_tick; 
}socket_tick;

typedef struct {
    char* host;
    int port;
}sock_connect_param;

static int socket_task(gnss_device_t gsm_dev,int index,socket_tick* p_socket_check_tick,sock_connect_param param)
{
    uint8_t need_reconnect=0;
    unsigned int heat_beat_period;
    unsigned int report_period;
    static uint8_t param_flag = 0;
    if(index <0||index>=MAX_SOCK_USE) return 0;
    if(index==0){
        heat_beat_period = get_heat_beat_period();
        report_period = get_report_period();
    }else{
        heat_beat_period = get_heat_beat_period();
        report_period = get_report1_period();
    }
    //try send heat and report for sock 0
		if(get_socket_flag(index)==1){
            if(index==1 && param_flag==0)
            {
                param_flag=1;
                report_keyparam(index);
            }
			led_link_set(LED_SLOW);
            if(p_socket_check_tick->last_beat_tick==0 || OSTimeGetMS() - p_socket_check_tick->last_beat_tick >= heat_beat_period*1000){
                p_socket_check_tick->last_beat_tick = OSTimeGetMS();
                //send beat
                send_heart_beat(index);
                rt_thread_mdelay(10);
            }
            if(p_socket_check_tick->last_send_tick==0 || OSTimeGetMS() - p_socket_check_tick->last_send_tick >= report_period*1000){
                p_socket_check_tick->last_send_tick = OSTimeGetMS();
                //send report
                sys_gb905_report(index);
                rt_thread_mdelay(10);
            }
		}else{
            if(p_socket_check_tick->last_err_tick==0 || OSTimeGetMS() - p_socket_check_tick->last_err_tick > 1000)
            {
                p_socket_check_tick->last_err_tick = OSTimeGetMS();
                if(get_socket_err(index) >10 && get_socket_err(index)%5==0)
                {
                    LOG_W("sockerr[%d]:%d",index,get_socket_err(index));
                }
                if(inc_socket_err(index) >(60)){
                    ucErrorTimes++;
                    set_socket_err(index,0);
                    LOG_W("need disconnect socket[%d]:%d",index,get_socket_err(index));
                    need_reconnect=1;
                }
            }

		}
        if(OSTimeGetMS()-p_socket_check_tick->last_recv_tick  >180*1000){
            p_socket_check_tick->last_recv_tick = OSTimeGetMS();
            set_socket_err(index,0);
            need_reconnect =1;
            led_link_set(LED_FAST);
        }
        if(need_reconnect){
            if(index==1 && param_flag!=0){param_flag=0;}
            cip_sock_close(gsm_dev,index);
            rt_thread_mdelay(50);
            cip_sock_open(gsm_dev,index,param.host,param.port);
            rt_thread_mdelay(50);
            ucErrorTimes =0;
            p_socket_check_tick->last_beat_tick=0;
            p_socket_check_tick->last_send_tick=0;
            return 1;
        }
        return 0;
}

static socket_tick socket_check_tick[2]={
    {0,0,0,0},
    {0,0,0,0}
};

void report_trige(unsigned char index)
{
    if(index<2)
    {
        socket_check_tick[index].last_send_tick=0;
    }
}

sock_connect_param sock_param;
/**
 * @description: 数据收发部分
 * @return 0：检测未完成；MD_ERR：错误
 */
static int module_data(gnss_device_t gsm_dev)
{
	params_setting_t * param;
	int ret=0;
	static uint32_t last_cclk_tick=0;
    if(socket_check_tick[0].last_recv_tick==0){socket_check_tick[0].last_recv_tick = OSTimeGetMS();}
    if(socket_check_tick[1].last_recv_tick==0){socket_check_tick[1].last_recv_tick = OSTimeGetMS();}
    //ret = recv_data_check(gsm_dev,0);
    ret = recv_data_select(gsm_dev);
	if(ret>0){
		ucErrorTimes=0;
        if(is_read_flag_set(ret,0)) {
            LOG_W("sock 0 recv OK!");
            socket_check_tick[0].last_recv_tick = OSTimeGetMS();
        }
        if(is_read_flag_set(ret,1)) {
            LOG_W("sock 1 recv OK!");
            socket_check_tick[1].last_recv_tick = OSTimeGetMS();
        }
	}else if(ret < 0) {
				led_link_set(LED_FAST);
        if (ucErrorTimes++ > 60)
        {
			LOG_E("recv err!need reset!!!");
            ucStateNum = MD_RUN_ERR;
        }
        if(ucStateNum==MD_RUN_ERR){
        	return MD_ERR;
        }
        return 0;
	}else if(ret ==0)
    {
        ucErrorTimes=0;
    }
    if((last_cclk_tick==0||OSTimeGetMS() -  last_cclk_tick> 2*60*1000))// 
    {
        last_cclk_tick = OSTimeGetMS();
        try_check_ntp(gsm_dev,0);

    }
    {

        http_sys_update_module_poll();
        param = get_setting_params();
        sock_param.host = (char*)&param->main_server_ipaddr[0];
        sock_param.port = param->main_server_tcp_port;
        socket_task(gsm_dev,0,&socket_check_tick[0],sock_param);
        sock_param.host = SEED_HOST;
        sock_param.port = SEED_PORT;
        socket_task(gsm_dev,1,&socket_check_tick[1],sock_param);
        gb905_operate_check(1,1);
    }
    
	return 0;
}
/**
 * @description: 飞行模式处理函数
 * @param None
 * @return 0：检测未完成；MD_WORK_STA_CHK：重新开启网络跳至模块状态检测；MD_ERR：错误
 */
static int module_flightmode(gnss_device_t gsm_dev)
{
		switch (ucStateNum)
    {
			case MD_RUN_STEP0:
				ucFlightModeTimes++;
				ucStateNum = MD_RUN_STEP1;
				LOG_W("进入飞行模式次数：%d",ucFlightModeTimes);
				led_link_set(LED_CLOSE);
				break;
			case MD_RUN_STEP1:
				if (ucFlightModeTimes == 2)
				{
						LOG_W("第二次进入飞行模式，复位模块");
						ucStateNum = MD_RUN_ERR;
				}
				else{
                    ucStateNum = MD_RUN_STEP2;
                }
				break;
			case MD_RUN_STEP2:
                if (send_AT_CMD(gsm_dev,"AT+CFUN=0","OK", 3000))
                {
                                //收到OK
                    if (at_cmdres_keyword_matching("OK"))
                    {
                        ucErrorTimes = 0;
                        ucStateNum = MD_RUN_STEP3;
                        last_time_tick = OSTimeGetMS();
                    }else{
                        //发送5次得不到正确应答，跳至MD_ERR
                        if (ucErrorTimes++ > 5)
                        {
                            ucStateNum = MD_RUN_ERR;
                        }
                    }
                    rt_thread_mdelay(250);
                }
                break;
			case MD_RUN_STEP3:
                if(OSTimeGetMS()-last_time_tick >= (5*1000)){
                    last_time_tick = OSTimeGetMS();
                    ucStateNum = MD_RUN_STEP4;
                }else{
                    rt_thread_mdelay(50);
                }
                break;
			case MD_RUN_STEP4:
                if (send_AT_CMD(gsm_dev,"AT+CFUN=1","OK", 3000))
                {
                                //收到OK,状态更为MD_WORK_STA_CHK，跳至模块状态检测
                    if (at_cmdres_keyword_matching("OK"))
                    {
                        LOG_W("再开启功能");
                        ucStateNum = MD_RUN_STEP0;
                        return MD_WORK_STA_CHK;
                    }else{
                        //发送5次得不到正确应答，跳至MD_ERR
                        if (ucErrorTimes++ > 5)
                        {
                            ucStateNum = MD_RUN_ERR;
                        }
                    }
                    rt_thread_mdelay(250);

                }
                break;
			case MD_RUN_ERR:
				ucStateNum = MD_RUN_STEP0;
				return MD_ERR;
			default:
        break;
    }
    return 0;	
}


void cip_mgr_run(gnss_device_t gsm_dev)
{
    static MD_RUN_STATE state = MD_RESET;
    static MD_RUN_STATE last_state = MD_RESET;
    static MD_RUN_STEP last_step = 0;
    int ret = 0;
    if(last_print_tick==0)
    {
        last_print_tick = OSTimeGet();
		state = MD_RESET;
		ucStateNum = MD_RUN_STEP2;
		LOG_I("mode = %d,step = %d",state,ucStateNum);
    }else if(last_state!=state || last_step!=ucStateNum){
        last_state = state;
        last_step = ucStateNum;
        last_print_tick = OSTimeGet();
        LOG_I("mode = %d,step = %d",state,ucStateNum);
    }else if(OSTimeGet()-last_print_tick >2){
        last_print_tick = OSTimeGet();
        LOG_I("mode = %d,step = %d",state,ucStateNum);
    }
    
    switch (state)
    {
    //复位模块
    case MD_RESET:
        if (module_reset(gsm_dev))
        {
			state = MD_AT_REQ;
        }
        break;
    //AT握手
    case MD_AT_REQ:
        ret = module_info(gsm_dev);
        if(ret == MD_OK){
            state = MD_WORK_STA_CHK;
        }
        else if(ret == MD_ERR)
        {
            state = MD_ERR;
        }
        break;
    //模块状态检测
    case MD_WORK_STA_CHK:
        ret = module_is_ready(gsm_dev);
        if (ret == MD_OK)
        {
            state = MD_CONNETINIT;
        }else if (ret == MD_ERR)
        {
            state = MD_FLIGHTMODE;
        }
        break;
    //连接参数初始化
    case MD_CONNETINIT:
        ret = module_connet_parm_init(gsm_dev);
        if (ret == MD_OK)
        {
            state = MD_CONNETED;
			ucFlightModeTimes = 0;
        }
        else if (ret == MD_ERR)
        {
            state = MD_FLIGHTMODE;
        }
        break;
    //数据通信处理
    case MD_CONNETED:
        if(module_data(gsm_dev) == MD_ERR)
        {
                state = MD_FLIGHTMODE;
        }
        break;
    //飞行模式处理
    case MD_FLIGHTMODE:
        ret = module_flightmode(gsm_dev);
        if(ret == MD_WORK_STA_CHK)
        {
            state = MD_WORK_STA_CHK;
        }else if(ret == MD_ERR)
        {
            ucFlightModeTimes = 0;
            state = MD_ERR;
        }
        break;
    //错误
    case MD_ERR:
        ucErrorTimes = 0;
		state = MD_RESET;
        break;
    default:
        break;
    }

}

static void gsm_thread_entry(void* parameter){
	int ret;
    gnss_device_t gsm_dev;
    //init io for gsm 
    init_gsm_io();
    //init gsm uart
	gsm_dev = create_gnss();
	if(gsm_dev==NULL){
		LOG_E("gsm_dev init master error");
		return;
	}
	ret = gnss_set_serial(gsm_dev,GSM_SERIAL_NAME,115200,8,'N',STOP_BITS_1,0);
	if(GNSS_RT_EOK != ret ){
		LOG_E("gsm_dev init uart error");
		return;
	}
    //set callback
	gnss_set_parse_callback(gsm_dev,gsm_paser_callback);

    ret = gnss_open_sync(gsm_dev);
	if(GNSS_RT_EOK != ret ){
		LOG_E("gnss_dev open uart error");
		return;
	}
	gsm_hard_rst();
	wait_gsm_ready(gsm_dev);
    init_cip_dev(gsm_dev);
	init_cb_buffer();
    while(1){
				cip_mgr_run(gsm_dev);
				rt_thread_mdelay(20);
				
    }

}

void init_gsm_thread(){
    rt_err_t result;
    /* init gsm device thread */
	result = rt_thread_init(&gsm_thread, "gsm", gsm_thread_entry, RT_NULL, (rt_uint8_t*)&gsm_stack[0], sizeof(gsm_stack), 6, 5);
    if (result == RT_EOK)
    {
        rt_thread_startup(&gsm_thread);
    }
}
