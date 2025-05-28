#ifndef __PKG_PROTOCOL_H_
#define __PKG_PROTOCOL_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <rtthread.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <string.h>
#include <rtdbg.h>

//utils 
#ifndef __packed
#define __packed    __attribute__((packed))
#endif


#include "gb905/gb905_protocol.h"
#include "gb905/gb905_param.h"
#include "gb905/gb905_report.h"
#include "gb905/gb905_remote.h"
#include "gb905/gb905_control.h"
#include "gb905/gb905_operate.h"

#include "http/http_client.h"
#include "http/module_http.h"


#ifdef __cplusplus
}
#endif

#endif /* __PKG_PROTOCOL_H_ */
