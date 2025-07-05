
#ifndef __PKG_GNSS_PORT_H_
#define __PKG_GNSS_PORT_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <rtthread.h>

#define GNSS_RT_EOK                          0                /**< 返回正常 */
#define GNSS_RT_ERROR                        1                /**< 一个标准错误或者未定义错误 */
#define GNSS_RT_ETIMEOUT                     2                /**< 超时 */
#define GNSS_RT_EFULL                        3                /**< 数据已满 */
#define GNSS_RT_EEMPTY                       4                /**< 数据为空 */
#define GNSS_RT_ENOMEM                       5                /**< 内存空间不足 */
#define GNSS_RT_ENOSYS                       6                /**< 系统错误 */
#define GNSS_RT_EBUSY                        7                /**< 设备忙 */
#define GNSS_RT_EIO                          8                /**< IO 错误 */
#define GNSS_RT_EINTR                        9                /**< 系统中断错误 */
#define GNSS_RT_EINVAL                       10               /**< 函数参数错误 */
#define GNSS_RT_ISOPEN                       11               /**< 设备已经开启 */

typedef struct gnss_serial {
    rt_device_t serial_port;
    int serial;
    struct gnss_serial * pre;
    struct gnss_serial *next;
}gnss_serial_t;

int gnss_serial_open(const char *devname, int baudrate, int bytesize, char parity, int stopbits, int xonxoff);
void gnss_serial_close(int serial);
void gnss_serial_send(int serial, void *buf, int len);
int gnss_serial_receive(int serial, void *buf, int bufsz, const int timeout, const int bytes_timeout);

#ifdef __cplusplus
}
#endif

#endif /* __PKG_GNSS_PORT_H_ */
