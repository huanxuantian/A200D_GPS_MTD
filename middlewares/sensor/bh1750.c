/*
 * Copyright (c) 2006-2019, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-4-30     Sanjay_Wu  the first version
 */

#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>

#include <string.h>

#define DBG_ENABLE
#define DBG_SECTION_NAME "bh1750"
#define DBG_LEVEL DBG_LOG
#define DBG_COLOR
#include <rtdbg.h>

#include "bh1750.h"


#ifdef PKG_USING_SENSOR_BH1750


static rt_err_t bh1750_read_regs(struct rt_i2c_bus_device *bus, rt_uint8_t len, rt_uint8_t *buf)
{
    struct rt_i2c_msg msgs;

    msgs.addr = BH1750_ADDR;
    msgs.flags = RT_I2C_RD;
    msgs.buf = buf;
    msgs.len = len;

    if (rt_i2c_transfer(bus, &msgs, 1) == 1)
    {
        return RT_EOK;
    }
    else
    {
        return -RT_ERROR;
    }
}

static rt_err_t bh1750_write_cmd(struct rt_i2c_bus_device *bus, rt_uint8_t cmd)
{
    struct rt_i2c_msg msgs;

    msgs.addr = BH1750_ADDR;
    msgs.flags = RT_I2C_WR;
    msgs.buf = &cmd;
    msgs.len = 1;

    if (rt_i2c_transfer(bus, &msgs, 1) == 1)
        return RT_EOK;
    else
        return -RT_ERROR;
}

static rt_err_t bh1750_set_measure_mode(bh1750_device_t hdev, rt_uint8_t mode, rt_uint8_t m_time)
{
    RT_ASSERT(hdev);
	
	bh1750_write_cmd(hdev->bus, BH1750_RESET);
	bh1750_write_cmd(hdev->bus, mode);
	rt_thread_mdelay(m_time);
	
    return RT_EOK;
}

rt_err_t bh1750_power_on(bh1750_device_t hdev)
{
    RT_ASSERT(hdev);

    bh1750_write_cmd(hdev->bus, BH1750_POWER_ON);
	
    return RT_EOK;
}

rt_err_t bh1750_power_down(bh1750_device_t hdev)
{
    RT_ASSERT(hdev);

    bh1750_write_cmd(hdev->bus, BH1750_POWER_DOWN);
	
    return RT_EOK;
}

rt_err_t bh1750_init(bh1750_device_t hdev, const char *i2c_bus_name)
{
    hdev->bus = rt_i2c_bus_device_find(i2c_bus_name);
    if (hdev->bus == RT_NULL)
    {
        LOG_E("Can't find bh1750 device on '%s' ", i2c_bus_name);
        rt_free(hdev->bus);
        return RT_NULL;
    }
	
    return RT_EOK;
}

rt_err_t bh1750_read_light(bh1750_device_t hdev,float* rvalue)
{
		rt_err_t ret;
    rt_uint8_t temp[2];
		memset(temp,0,sizeof(temp));
    float current_light = 0;

    RT_ASSERT(hdev);

	bh1750_set_measure_mode(hdev, BH1750_CON_H_RES_MODE2, BH1750_H_RES_MODE2_DELAY_MS);
	ret = bh1750_read_regs(hdev->bus, 2, temp);
	if(ret==RT_EOK){
	current_light = ((float)((temp[0] << 8) + temp[1]) / 1.2);
	*rvalue = current_light;
	}
    return ret;
}


static bh1750_device_t bh1750_create(const char* i2c_bus_name)
{
    bh1750_device_t hdev = rt_malloc(sizeof(bh1750_device_t));
	
    if (hdev == RT_NULL)
    {
        return RT_NULL;
    }
	
    bh1750_init(hdev, i2c_bus_name);

    return hdev;
}
static struct rt_device n32g43x_bh1750_dev;

rt_err_t bh1750_set_power(bh1750_device_t hdev, rt_uint8_t power)
{
    if (power == RT_BH1750_POWER_NORMAL)
    {
        bh1750_power_on(hdev);
    }
    else if (power == RT_BH1750_POWER_DOWN)
    {
        bh1750_power_down(hdev);
    }
    else
    {
        return -RT_ERROR;
    }
	
    return RT_EOK;
}

rt_err_t bh1750_fetch_data(bh1750_device_t hdev, void *buf){
		rt_err_t ret;
		float light_value;
		bh1750_data_t data = (bh1750_data_t)buf;
		ret = bh1750_read_light(hdev,&light_value);
	if(ret==RT_EOK){
		data->value = (rt_int32_t)(light_value * 1000.0);	
		data->fetch_time = rt_tick_get();
	}
	
    return ret;	
}

static rt_err_t n3243x_bh1750_control(rt_device_t dev, int cmd, void *args){
	rt_err_t result = RT_EOK;
    bh1750_device_t hdev = dev->user_data;
	    switch (cmd)
    {
		case RT_BH1750_CTRL_SET_POWER:
			result = bh1750_set_power(hdev, (rt_uint32_t)args & 0xff);
			break;
		case RT_BH1750_CTRL_FETCH_REGS:
			result = bh1750_fetch_data(hdev,args);
        break;
		case RT_BH1750_CTRL_SET_REGS:
			result =  -RT_EINVAL;
        break;		
		default:
			result = -RT_ERROR;
		break;
    }
    return result;
	
}

#ifdef RT_USING_DEVICE_OPS
const static struct rt_device_ops soft_bh1750_ops = 
{
    RT_NULL,
    RT_NULL,
    RT_NULL,
    RT_NULL,
    RT_NULL,
    n3243x_bh1750_control
};
#endif
int bh1750_port(void)
{
		bh1750_device_t bh1750_dev;
	
		n32g43x_bh1750_dev.type = RT_Device_Class_Unknown;
	#ifdef RT_USING_DEVICE_OPS
    n32g43x_bh1750_dev.ops     = &soft_bh1750_ops;
#else
    n32g43x_bh1750_dev.init    = RT_NULL;
    n32g43x_bh1750_dev.open    = RT_NULL;
    n32g43x_bh1750_dev.close   = RT_NULL;
    n32g43x_bh1750_dev.read    = RT_NULL;
    n32g43x_bh1750_dev.write   = RT_NULL;
    n32g43x_bh1750_dev.control = n3243x_bh1750_control;
#endif
	
		bh1750_dev = bh1750_create("i2c1");	
		
		n32g43x_bh1750_dev.user_data = bh1750_dev;
	
		rt_device_register(&n32g43x_bh1750_dev, "bh1750", RT_DEVICE_FLAG_RDWR);
	
    return 0;
}
INIT_DEVICE_EXPORT(bh1750_port);

#endif /* PKG_USING_SENSOR_BH1750 */

