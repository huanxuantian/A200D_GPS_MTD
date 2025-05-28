#ifndef __MODBUS_M_H_
#define __MODBUS_M_H_

#ifdef __cplusplus
extern "C" {
#endif

#define MASTER1_SERIAL_NAME "usart1"//光照
#define MASTER2_SERIAL_NAME "uart5" //温湿度
#define MASTER3_SERIAL_NAME "usart3"//485土壤8项

void modbus_init();
void handle_modbus_task();
void start_modbus_thread();
void stop_modbus_thread();
void restart_modbus_thread();
void init_block_data();
#ifdef __cplusplus
}
#endif

#endif /* __MODBUS_M_H_ */
