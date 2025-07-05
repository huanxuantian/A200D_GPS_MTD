#include <string.h>
#include "rtt_uart.h"
#include "gnss_port.h"

static gnss_serial_t *p_serial_info = NULL;

static int gnss_serial_add_dev(rt_device_t serial_port)
{
    int serial = 0;
    gnss_serial_t *serial_port_temp = rt_malloc(sizeof(struct gnss_serial));
    if(NULL == serial_port_temp) {
        return - GNSS_RT_ENOMEM;
    }
    memset(serial_port_temp, 0,  sizeof(struct gnss_serial));
    if(NULL == p_serial_info) {
        p_serial_info = serial_port_temp;
        serial++;
    } else {
        gnss_serial_t *temp = p_serial_info;
        serial = temp->serial;
        while(NULL != temp->next) {
            temp = temp->next;
						serial = temp->serial;
        }
        temp->next = serial_port_temp;
        serial_port_temp->pre = temp;
        serial++;
    }
    serial_port_temp->serial_port = serial_port;
    serial_port_temp->serial = serial;
    return serial;
}

static rt_device_t gnss_serial_get_dev(int serial)
{
    gnss_serial_t *serial_port_temp = p_serial_info;
    while( NULL != serial_port_temp) {
        if(serial == serial_port_temp->serial){
            return serial_port_temp->serial_port;
        } 
        serial_port_temp = serial_port_temp->next;
    }
    return NULL;
}

static void gnss_serial_close_dev(int serial)
{
    gnss_serial_t *serial_port_temp = p_serial_info;
    while( NULL != serial_port_temp) {
        if(serial == serial_port_temp->serial){
            break;
        } 
        serial_port_temp = serial_port_temp->next;
    }
    if(NULL == serial_port_temp) {
        return ;
    }
    rt_device_t serial_port = serial_port_temp->serial_port;
    rtt_uart_close(serial_port);
    if(NULL == serial_port_temp->pre) {
        if(NULL == serial_port_temp->next) {
            p_serial_info = NULL;
            rt_free(serial_port_temp);
        } else {
            p_serial_info = serial_port_temp->next;
            rt_free(serial_port_temp);
        }
    } else if(NULL == serial_port_temp->next) {
        serial_port_temp->pre->next = NULL;
        rt_free(serial_port_temp);
    } else {
        serial_port_temp->pre->next = serial_port_temp->next;
        serial_port_temp->next->pre = serial_port_temp->pre;
        rt_free(serial_port_temp);
    }
}

int gnss_serial_open(const char *devname, int baudrate, int bytesize, char parity, int stopbits, int xonxoff) {
    rt_device_t serial_port = rtt_uart_open(devname, baudrate, bytesize, parity, stopbits,xonxoff);
    if(NULL == serial_port) {
        return -GNSS_RT_ERROR;
    }
    return gnss_serial_add_dev(serial_port);
}

void gnss_serial_close(int serial) {
    gnss_serial_close_dev(serial);
}

void gnss_serial_send(int serial, void *buf, int len) {
    rt_device_t serial_port = gnss_serial_get_dev(serial);
    rtt_uart_send(serial_port, buf, len);
}

int gnss_serial_receive(int serial, void *buf, int bufsz, const int timeout, const int bytes_timeout) {
    rt_device_t serial_port = gnss_serial_get_dev(serial);
    return rtt_uart_receive(serial_port, buf, bufsz, timeout, bytes_timeout);
}
