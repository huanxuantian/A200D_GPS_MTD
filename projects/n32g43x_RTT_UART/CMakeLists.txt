cmake_minimum_required(VERSION 3.16)

set(TOP_BASE "../../")
#set(MCU_TOOLCHAIN_PATH /config/arm/gcc/)
#set(MCU_TARGET_TRIPLET arm-none-eabi)
#set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/${TOP_BASE}cmake/mcu_gcc.cmake)

set(MCU_TYPE "N32G43x")
project(rtt-modbus  C ASM)

INCLUDE_DIRECTORIES("APP/inc")
#drv
INCLUDE_DIRECTORIES("DeviceDrivers/gpio/inc")
INCLUDE_DIRECTORIES("DeviceDrivers/adc/inc")
INCLUDE_DIRECTORIES("DeviceDrivers/spi/inc")
INCLUDE_DIRECTORIES("DeviceDrivers/i2c/inc")
INCLUDE_DIRECTORIES("DeviceDrivers/uart/inc")
INCLUDE_DIRECTORIES("DeviceDrivers/rtc/inc")

INCLUDE_DIRECTORIES("gnss_sk")
INCLUDE_DIRECTORIES("${TOP_BASE}middlewares/gnss_port")
INCLUDE_DIRECTORIES("${TOP_BASE}middlewares/gps")
INCLUDE_DIRECTORIES("${TOP_BASE}middlewares/nmea")

INCLUDE_DIRECTORIES("bsp")
INCLUDE_DIRECTORIES("business")
INCLUDE_DIRECTORIES("${TOP_BASE}middlewares/utils")
INCLUDE_DIRECTORIES("${TOP_BASE}middlewares/struct2json")
INCLUDE_DIRECTORIES("gsm")

INCLUDE_DIRECTORIES("${TOP_BASE}middlewares/http")
INCLUDE_DIRECTORIES("protocol")

#modbus_rt inc
INCLUDE_DIRECTORIES("${TOP_BASE}middlewares/modbus_rt/rtt_uart")
INCLUDE_DIRECTORIES("${TOP_BASE}middlewares/modbus_rt/modbus_rt/modbus_rt")
INCLUDE_DIRECTORIES("${TOP_BASE}middlewares/modbus_rt/modbus_rt/agile_modbus")
INCLUDE_DIRECTORIES("${TOP_BASE}middlewares/modbus_rt/modbus_rt/platform/rt_thread")
INCLUDE_DIRECTORIES("${TOP_BASE}middlewares/modbus_rt/modbus_rt/slave_util")

#core
INCLUDE_DIRECTORIES("${TOP_BASE}firmware/CMSIS/core")
INCLUDE_DIRECTORIES("${TOP_BASE}firmware/CMSIS/device")
INCLUDE_DIRECTORIES("${TOP_BASE}firmware/n32g43x_std_periph_driver/inc")

#rt-thread core
INCLUDE_DIRECTORIES("${TOP_BASE}middlewares/rt-thread/include")
INCLUDE_DIRECTORIES("${TOP_BASE}middlewares/rt-thread/include/libc")
INCLUDE_DIRECTORIES("${TOP_BASE}middlewares/rt-thread/components/libc/compilers/newlib")
INCLUDE_DIRECTORIES("${TOP_BASE}middlewares/rt-thread/components/libc/compilers/minilibc")
#rtt components
INCLUDE_DIRECTORIES("${TOP_BASE}middlewares/rt-thread/components/drivers/include")
INCLUDE_DIRECTORIES("${TOP_BASE}middlewares/rt-thread/components/drivers/include/drivers")
INCLUDE_DIRECTORIES("${TOP_BASE}middlewares/rt-thread/components/drivers/include/ipc")
INCLUDE_DIRECTORIES("${TOP_BASE}middlewares/rt-thread/components/finsh")
#spi
INCLUDE_DIRECTORIES("${TOP_BASE}middlewares/rt-thread/components/drivers/spi")
INCLUDE_DIRECTORIES("${TOP_BASE}middlewares/rt-thread/components/drivers/spi/sfud/inc")


AUX_SOURCE_DIRECTORY("APP/src" DIR_SRCS)
#drv
AUX_SOURCE_DIRECTORY("DeviceDrivers/gpio/src" DIR_SRCS)
AUX_SOURCE_DIRECTORY("DeviceDrivers/adc/src" DIR_SRCS)
AUX_SOURCE_DIRECTORY("DeviceDrivers/spi/src" DIR_SRCS)
AUX_SOURCE_DIRECTORY("DeviceDrivers/i2c/src" DIR_SRCS)
AUX_SOURCE_DIRECTORY("DeviceDrivers/uart/src" DIR_SRCS)
AUX_SOURCE_DIRECTORY("DeviceDrivers/rtc/src" DIR_SRCS)

AUX_SOURCE_DIRECTORY("gnss_sk" DIR_SRCS)
AUX_SOURCE_DIRECTORY("${TOP_BASE}middlewares/gnss_port" DIR_SRCS)
AUX_SOURCE_DIRECTORY("${TOP_BASE}middlewares/gps" DIR_SRCS)
AUX_SOURCE_DIRECTORY("${TOP_BASE}middlewares/nmea" DIR_SRCS)

AUX_SOURCE_DIRECTORY("bsp" DIR_SRCS)
AUX_SOURCE_DIRECTORY("business" DIR_SRCS)
AUX_SOURCE_DIRECTORY("${TOP_BASE}middlewares/utils" DIR_SRCS)
AUX_SOURCE_DIRECTORY("${TOP_BASE}middlewares/struct2json" DIR_SRCS)
AUX_SOURCE_DIRECTORY("gsm" DIR_SRCS)

AUX_SOURCE_DIRECTORY("protocol" DIR_SRCS)
AUX_SOURCE_DIRECTORY("protocol/gb905" DIR_SRCS)
AUX_SOURCE_DIRECTORY("${TOP_BASE}middlewares/http" DIR_SRCS)
AUX_SOURCE_DIRECTORY("protocol/http" DIR_SRCS)

AUX_SOURCE_DIRECTORY("${TOP_BASE}middlewares/modbus_rt/rtt_uart" DIR_SRCS)
AUX_SOURCE_DIRECTORY("${TOP_BASE}middlewares/modbus_rt/modbus_rt/agile_modbus" DIR_SRCS)
AUX_SOURCE_DIRECTORY("${TOP_BASE}middlewares/modbus_rt/modbus_rt/modbus_rt" DIR_SRCS)
AUX_SOURCE_DIRECTORY("${TOP_BASE}middlewares/modbus_rt/modbus_rt/platform/rt_thread" DIR_SRCS)
AUX_SOURCE_DIRECTORY("${TOP_BASE}middlewares/modbus_rt/modbus_rt/slave_util" DIR_SRCS)


AUX_SOURCE_DIRECTORY("${TOP_BASE}firmware/CMSIS/device" DIR_SRCS)

#rt-thread src
AUX_SOURCE_DIRECTORY("${TOP_BASE}middlewares/rt-thread/src" DIR_SRCS)
AUX_SOURCE_DIRECTORY("${TOP_BASE}middlewares/rt-thread/libcpu/common" DIR_SRCS)
AUX_SOURCE_DIRECTORY("${TOP_BASE}middlewares/rt-thread/libcpu/cortex-m4" DIR_SRCS)

#AUX_SOURCE_DIRECTORY("${TOP_BASE}middlewares/rt-thread/components/libc/compilers/newlib" DIR_SRCS)
AUX_SOURCE_DIRECTORY("${TOP_BASE}middlewares/rt-thread/components/libc/compilers/minilibc" DIR_SRCS)

AUX_SOURCE_DIRECTORY("${TOP_BASE}middlewares/rt-thread/components/finsh" DIR_SRCS)
AUX_SOURCE_DIRECTORY("${TOP_BASE}middlewares/rt-thread/components/drivers/src" DIR_SRCS)
AUX_SOURCE_DIRECTORY("${TOP_BASE}middlewares/rt-thread/components/drivers/serial" DIR_SRCS)
AUX_SOURCE_DIRECTORY("${TOP_BASE}middlewares/rt-thread/components/drivers/spi/sfud/src" DIR_SRCS)


set(STARTUP_ASM ${TOP_BASE}firmware/CMSIS/device/startup/startup_n32g43x_gcc_rtt.s)

set(RTTHREAD_ASM ${TOP_BASE}middlewares/rt-thread/libcpu/cortex-m4/context_gcc_w.s)

set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/APP/link.lds)

add_definitions("-DUSE_STDPERIPH_DRIVER")
add_definitions("-DN32G43X")

add_definitions("-DRT_USING_MINILIBC")
add_definitions("-DDEBUG_APP")
#add_definitions("-DDEBUG_UART1")
add_definitions("-DDEBUG_SWD")

set(PROJECT_SOURCES
    ${DIR_SRCS}
    ${STARTUP_ASM}
    "${TOP_BASE}firmware/n32g43x_std_periph_driver/src/misc.c"
    "${TOP_BASE}firmware/n32g43x_std_periph_driver/src/n32g43x_exti.c"
    "${TOP_BASE}firmware/n32g43x_std_periph_driver/src/n32g43x_rcc.c"
    "${TOP_BASE}firmware/n32g43x_std_periph_driver/src/n32g43x_gpio.c"
    "${TOP_BASE}firmware/n32g43x_std_periph_driver/src/n32g43x_flash.c"
    "${TOP_BASE}firmware/n32g43x_std_periph_driver/src/n32g43x_pwr.c"
    "${TOP_BASE}firmware/n32g43x_std_periph_driver/src/n32g43x_usart.c"
    "${TOP_BASE}firmware/n32g43x_std_periph_driver/src/n32g43x_i2c.c"
    "${TOP_BASE}firmware/n32g43x_std_periph_driver/src/n32g43x_spi.c"
    "${TOP_BASE}firmware/n32g43x_std_periph_driver/src/n32g43x_rtc.c"
    "${TOP_BASE}firmware/n32g43x_std_periph_driver/src/n32g43x_adc.c"
    #rt-thread
    ${RTTHREAD_ASM}
    "${TOP_BASE}middlewares/rt-thread/components/drivers/misc/pin.c"
    "${TOP_BASE}middlewares/rt-thread/components/drivers/misc/adc.c"
    "${TOP_BASE}middlewares/rt-thread/components/drivers/rtc/rtc.c"
    #spi
    "${TOP_BASE}middlewares/rt-thread/components/drivers/spi/spi_core.c"
    "${TOP_BASE}middlewares/rt-thread/components/drivers/spi/spi_dev.c"
    "${TOP_BASE}middlewares/rt-thread/components/drivers/spi/spi_flash_sfud.c"
    "${TOP_BASE}middlewares/rt-thread/components/drivers/spi/qspi_core.c"
)

message("#########################################")
message( "DEVICE: ${MCU_TYPE}" )
message("#########################################")


add_executable(${PROJECT_NAME} ${PROJECT_SOURCES} )
target_link_libraries(${PROJECT_NAME} MCU::NoSys ${MCU_TYPE})

target_compile_options(${PROJECT_NAME} PUBLIC "-Os")
mcu_add_linker_script(${PROJECT_NAME} PUBLIC ${LINKER_SCRIPT})


target_link_options(${PROJECT_NAME} PUBLIC -Xlinker -Map=${PROJECT_NAME}.map)

mcu_generate_binary_file(${PROJECT_NAME})
mcu_print_size_of_target(${PROJECT_NAME})

