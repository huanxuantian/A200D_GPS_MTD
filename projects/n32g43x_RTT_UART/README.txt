用于在 keil使用rt-thread下的 modbus_rt实现来构筑modbus应用的依赖
1，modbus_rtt_uart 
modbus用于rtt下对接硬件usart的收发控制的基本接口
需要用户移植是根据自己的rtt和硬件方案配置如485方向控制以及对应的串口配置映射
2,modbus_rtt_phal
modbus用于rtt下实现modbus rt需要的适配器(如网络，线程，内存，串行接口管理)
根据rtt配置和版本可能需要调整对应接口实现
3，agile_modbus_misc
modbus_rt中的扩展agile_modbus的代码（目前只有一个slave的数据管理）
定义SLAVE_DATA_DEVICE_BINDING后实现对应的函数可绑定到存储设备（目前未定义实现）
4，agile_modbus
agile_modbus的源代码，基本不要修改
5，modbus_rt
modbus_rt实现的代码，包含对agile_modbus的封装和对接platform的port调用以及自己实现的扩展功能
使用modbus_config.h 来配置特征功能
（为了裁减必须要修改该配置文件，删除不必要的实现的MODBUS_XX_ENABLE宏定义）
我已调整部分可选功能，讲原有的ifndef MODBUS_XX_ENABLE 改成了ifdef USE_XX的用法
并且加载rtconfig来配置这些USE_XX的开关，便于项目全局可配置都在rtconfig中
目前该DEMO代码已设置3个主机采集外部串口设备数据，一路从机实现。三个主机外设采集数据映缓存到自身的从机内存寄存器中
for keil rt-thread modbus struct for modbus_rt group depends
1,modbus_rtt_uart 
base function fix to rtt and hardware device config
2,modbus_rtt_phal
platform interface adapter for rtt （network,thread,memery serial）
3,agile_modbus_misc
agile modbus base strack extend code(slave only now)
4,agile_modbus
agile modbus base strack code only
5,modbus_rt
modbus function and interface in system
use modbus_config.h for feature config
(for rtt cut down some code by change MODBUS_XX_ENABLE)
i have change to use USE_XX insead ifndef MODBUS_XX_ENABLE 
set USE_XX in rtconfig for function feature control for whole project control only