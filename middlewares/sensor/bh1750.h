/*
 * Copyright (c) 2006-2019, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-04-30     Sanjay_Wu  the first version
 */

#ifndef __BH1750_H__
#define __BH1750_H__

#include <rthw.h>
#include <rtthread.h>

#include <rthw.h>
#include <rtdevice.h>

/*bh1750 device address */
#define BH1750_ADDR 0x23

/*bh1750 registers define */
#define BH1750_POWER_DOWN   	0x00	// power down
#define BH1750_POWER_ON			0x01	// power on
#define BH1750_RESET			0x07	// reset	
#define BH1750_CON_H_RES_MODE	0x10	// Continuously H-Resolution Mode
#define BH1750_CON_H_RES_MODE2	0x11	// Continuously H-Resolution Mode2 
#define BH1750_CON_L_RES_MODE	0x13	// Continuously L-Resolution Mode
#define BH1750_ONE_H_RES_MODE	0x20	// One Time H-Resolution Mode
#define BH1750_ONE_H_RES_MODE2	0x21	// One Time H-Resolution Mode2
#define BH1750_ONE_L_RES_MODE	0x23	// One Time L-Resolution Mode

#define BH1750_H_RES_MODE2_DELAY_MS 150


#define RT_BH1750_CTRL_SET_POWER	0x10
#define RT_BH1750_CTRL_FETCH_REGS	0x20
#define RT_BH1750_CTRL_SET_REGS	0x30

#define RT_BH1750_POWER_NORMAL 0x31
#define RT_BH1750_POWER_DOWN 0x32

struct bh1750_device
{	
    struct rt_i2c_bus_device *bus;
};

typedef struct bh1750_device *bh1750_device_t;

struct rt_bh1750_data
{
	rt_int32_t type;
	rt_int32_t reg_address;
	rt_int32_t value;
	rt_uint32_t fetch_time;
};
typedef struct rt_bh1750_data * bh1750_data_t;

rt_err_t bh1750_power_on(bh1750_device_t hdev);
rt_err_t bh1750_power_down(bh1750_device_t hdev);
rt_err_t bh1750_init(bh1750_device_t hdev, const char *i2c_bus_name);
rt_err_t bh1750_read_light(bh1750_device_t hdev,float* rvalue);


#endif /* __BH1750_H__ */
