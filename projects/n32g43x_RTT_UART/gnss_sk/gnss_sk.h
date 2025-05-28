#ifndef __PKG_GNSS_SK_H_
#define __PKG_GNSS_SK_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <rtthread.h>
#include "drv_gpio.h"
#include "MicroNMEA.h"

#define GNSS_SERIAL_NAME "uart4"

#define     GNSS_PWR_PIN        GET_PIN(B,  5)
#define     ANT_IN_NON_PIN        GET_PIN(B,  4)
#define     ANT_IN_NSHORT_PIN        GET_PIN(B,  3)

void init_gnss_thread(struct rt_thread *thread,
                        const char       *name,
						void             *stack_start,
                        rt_uint32_t       stack_size);
void gnss_io_init();
void change_pps();

#ifdef __cplusplus
}
#endif

#endif /* __PKG_GNSS_SK_H_ */
