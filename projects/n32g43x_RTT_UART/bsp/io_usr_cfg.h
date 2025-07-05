#ifndef __PKG_IO_USR_CFG_H_
#define __PKG_IO_USR_CFG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <rtconfig.h>


//for rtt_uart uart_dev name
#define UART1_DEV_NAME  "usart1"
#define UART2_DEV_NAME  "usart2"
#define UART3_DEV_NAME  "usart3"
#define UART4_DEV_NAME  "uart4"
#define UART5_DEV_NAME  "uart5"
#define UART6_DEV_NAME  "lpuart"

//for rtt_uart 485
//IO 序号参考drv_gpio.c的定义
#if defined(RTU_USING_UART1) && defined(RTU_UART1_USING_RS485)
#define UART1_RS485_RE_PIN          GET_PIN(A, 8)                //PA8
#define uart1_rs485_tx_en()         rt_pin_write(UART1_RS485_RE_PIN, PIN_HIGH)
#define uart1_rs485_rx_en()         rt_pin_write(UART1_RS485_RE_PIN, PIN_LOW)
#endif

#if defined(RTU_USING_UART2) && defined(RTU_UART2_USING_RS485)
#define UART2_RS485_RE_PIN          GET_PIN(A, 4)                 //PE5
#define uart2_rs485_tx_en()         rt_pin_write(UART2_RS485_RE_PIN, PIN_HIGH)
#define uart2_rs485_rx_en()         rt_pin_write(UART2_RS485_RE_PIN, PIN_LOW)
#endif

#if defined(RTU_USING_UART3) && defined(RTU_UART3_USING_RS485)
#define UART3_RS485_RE_PIN          GET_PIN(B, 1)                //PD3
#define uart3_rs485_tx_en()         rt_pin_write(UART3_RS485_RE_PIN, PIN_HIGH)
#define uart3_rs485_rx_en()         rt_pin_write(UART3_RS485_RE_PIN, PIN_LOW)
#endif

#if defined(RTU_USING_UART4) && defined(RTU_UART4_USING_RS485)
#define UART4_RS485_RE_PIN          GET_PIN(B, 9)                //PH9
#define uart4_rs485_tx_en()         rt_pin_write(UART4_RS485_RE_PIN, PIN_LOW)
#define uart4_rs485_rx_en()         rt_pin_write(UART4_RS485_RE_PIN, PIN_HIGH)
#endif

#if defined(RTU_USING_UART5) && defined(RTU_UART5_USING_RS485)
#define UART5_RS485_RE_PIN          GET_PIN(D, 4)                //PD4
#define uart5_rs485_tx_en()         rt_pin_write(UART5_RS485_RE_PIN, PIN_HIGH)
#define uart5_rs485_rx_en()         rt_pin_write(UART5_RS485_RE_PIN, PIN_LOW)
#endif

#if defined(RTU_USING_UART6) && defined(RTU_UART6_USING_RS485)
#define UART6_RS485_RE_PIN          GET_PIN(C, 3)                //PC3
#define uart6_rs485_tx_en()         rt_pin_write(UART6_RS485_RE_PIN, PIN_HIGH)
#define uart6_rs485_rx_en()         rt_pin_write(UART6_RS485_RE_PIN, PIN_LOW)
#endif

#ifdef __cplusplus
}
#endif

#endif /* __PKG_UART_USR_CFG_H_ */