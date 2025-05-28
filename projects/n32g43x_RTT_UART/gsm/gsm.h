#ifndef __PKG_GSM_H_
#define __PKG_GSM_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "drv_gpio.h"
//A7670C
//POWERON : PKEY->HIGH(50ms+)->LOW(50ms+/100ms-)->HIGH(--)
//WAITME:15S
//LOOPRESET:30S
//
#define     OUT_GSM_PKEY_PIN        GET_PIN(A,  1)
#define     OUT_GSM_DTR_PIN        GET_PIN(C,  8)
#define     OUT_GSM_PEN_PIN        GET_PIN(A,  15)
#define     OUT_GSM_WAKEUP_PIN        GET_PIN(C,  3)
#define     OUT_GSM_AT_READY_PIN        GET_PIN(C,  5)

#define     IN_GSM_RING_PIN        GET_PIN(C,  4)

#define     GSM_INIT_TIMEOUT     (10*1000)

#define GSM_SERIAL_NAME "usart2"

int gateway_handle(int index,void* data,int data_size);

void report_trige(unsigned char index);
void init_gsm_thread();

#ifdef __cplusplus
}
#endif

#endif /* __PKG_GSM_H_ */
