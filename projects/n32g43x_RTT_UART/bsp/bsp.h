#ifndef __PKG_BSP_H_
#define __PKG_BSP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "drv_gpio.h"

//ADC:PC0,PC1
//adc_device:adc,CH0,CH1
#define PWR_ADC_CHN	ADC_CH_11_PC0
#define EXT_ADC_CHN	ADC_CH_12_PC1

#define ADC_mV1_K  (50490)//(1530*33)//1%fix


//ACC input
#define     IN_ACC_DET_PIN        GET_PIN(A,  5)
//single input
#define     IN_ALARM_PIN        GET_PIN(C,  13)
#define     IN_DOOR_SIG_PIN        GET_PIN(C,  2)
//control output
#define     OUT_LOCK_PIN        GET_PIN(A,  6)
#define     OUT_GEN1_PIN        GET_PIN(A,  0)
//in/out config floating port
#define     INOUT_GEN1_PIN        GET_PIN(A,  4)
#define     INOUT_GEN2_PIN        GET_PIN(B,  0)
//EXT power control
#define     EXT_POWER1_PIN        GET_PIN(A,  8)
#define     EXT_POWER2_PIN        GET_PIN(C,  9)
//speak mute
#define     SPK_MUTE_PIN        GET_PIN(A,  7)

//swj led reuse (SCLK LED link to exten_watchdog,must flip in 2s)
#define     LED1_SCLK_PIN        GET_PIN(A,  14)
#define     LED2_SWIO_PIN        GET_PIN(A,  13)

#define LED_CLOSE 0
#define LED_SLOW 1
#define LED_FAST 2
#define LED_LONGSLOW 0xFF

//SPI use PB15(MOSI),PB14(MISO),PB13(SPI_CLK),PB12(CS0)

#define     OUT_CONTROL_CH1 1
#define     OUT_CONTROL_CH2 2

#define     OUT_LEVEL_HIGH 1
#define     OUT_LEVEL_LOW 0

#define CIRCUIT_OUT OUT_CONTROL_CH1
#define DOOR_OUT OUT_CONTROL_CH2

#define ACC_NOCHANGE    0
#define ACC_ON_DETECT   2
#define ACC_OFF_DETECT  1

void update_imei(char* imei);
void get_imei(char* buff,int buffer_size);
void update_iccid(char* iccid);
void get_iccid(char* buff,int buffer_size);

unsigned int  build_uuid(unsigned char* buff,int buff_size);
int init_adc_config();
unsigned int get_pwr_mv();
unsigned int get_ext_mv();

void wd_feed();
void led_link_handle();
void led_link_set(unsigned char mode);

int bsp_init();

int acc_state_poll(char mode);
void init_update_output(int chn,int level);
void control_output(int chn,int level);

#ifdef __cplusplus
}
#endif

#endif /* __PKG_BSP_H_ */
