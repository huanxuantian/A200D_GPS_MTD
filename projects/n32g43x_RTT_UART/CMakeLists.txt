cmake_minimum_required(VERSION 3.16)

#set(MCU_TOOLCHAIN_PATH /config/arm/gcc/)
#set(MCU_TARGET_TRIPLET arm-none-eabi)
#set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/mcu_gcc.cmake)

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
INCLUDE_DIRECTORIES("gnss_sk/gps")
INCLUDE_DIRECTORIES("gnss_sk/nmea")

INCLUDE_DIRECTORIES("bsp")
INCLUDE_DIRECTORIES("business")
INCLUDE_DIRECTORIES("utils")
INCLUDE_DIRECTORIES("utils/struct2json")
INCLUDE_DIRECTORIES("gsm")

INCLUDE_DIRECTORIES("protocol")

#modbus_rt inc
INCLUDE_DIRECTORIES("modbus_rt/rtt_uart")
INCLUDE_DIRECTORIES("modbus_rt/modbus_rt/modbus_rt")
INCLUDE_DIRECTORIES("modbus_rt/modbus_rt/agile_modbus")
INCLUDE_DIRECTORIES("modbus_rt/modbus_rt/platform/rt_thread")
INCLUDE_DIRECTORIES("modbus_rt/modbus_rt/slave_util")

#core
INCLUDE_DIRECTORIES("../../firmware/CMSIS/core")
INCLUDE_DIRECTORIES("../../firmware/CMSIS/device")
INCLUDE_DIRECTORIES("../../firmware/n32g43x_std_periph_driver/inc")

#rt-thread core
INCLUDE_DIRECTORIES("../../middlewares/rt-thread/include")
INCLUDE_DIRECTORIES("../../middlewares/rt-thread/include/libc")
INCLUDE_DIRECTORIES("../../middlewares/rt-thread/components/libc/compilers/newlib")
INCLUDE_DIRECTORIES("../../middlewares/rt-thread/components/libc/compilers/minilibc")
#rtt components
INCLUDE_DIRECTORIES("../../middlewares/rt-thread/components/drivers/include")
INCLUDE_DIRECTORIES("../../middlewares/rt-thread/components/drivers/include/drivers")
INCLUDE_DIRECTORIES("../../middlewares/rt-thread/components/drivers/include/ipc")
INCLUDE_DIRECTORIES("../../middlewares/rt-thread/components/finsh")
#spi
INCLUDE_DIRECTORIES("../../middlewares/rt-thread/components/drivers/spi")
INCLUDE_DIRECTORIES("../../middlewares/rt-thread/components/drivers/spi/sfud/inc")


AUX_SOURCE_DIRECTORY("APP/src" DIR_SRCS)
#drv
AUX_SOURCE_DIRECTORY("DeviceDrivers/gpio/src" DIR_SRCS)
AUX_SOURCE_DIRECTORY("DeviceDrivers/adc/src" DIR_SRCS)
AUX_SOURCE_DIRECTORY("DeviceDrivers/spi/src" DIR_SRCS)
AUX_SOURCE_DIRECTORY("DeviceDrivers/i2c/src" DIR_SRCS)
AUX_SOURCE_DIRECTORY("DeviceDrivers/uart/src" DIR_SRCS)
AUX_SOURCE_DIRECTORY("DeviceDrivers/rtc/src" DIR_SRCS)

AUX_SOURCE_DIRECTORY("gnss_sk" DIR_SRCS)
AUX_SOURCE_DIRECTORY("gnss_sk/gps" DIR_SRCS)
AUX_SOURCE_DIRECTORY("gnss_sk/nmea" DIR_SRCS)

AUX_SOURCE_DIRECTORY("bsp" DIR_SRCS)
AUX_SOURCE_DIRECTORY("business" DIR_SRCS)
AUX_SOURCE_DIRECTORY("utils" DIR_SRCS)
AUX_SOURCE_DIRECTORY("utils/struct2json" DIR_SRCS)
AUX_SOURCE_DIRECTORY("gsm" DIR_SRCS)

AUX_SOURCE_DIRECTORY("protocol" DIR_SRCS)
AUX_SOURCE_DIRECTORY("protocol/gb905" DIR_SRCS)
AUX_SOURCE_DIRECTORY("protocol/http" DIR_SRCS)

AUX_SOURCE_DIRECTORY("modbus_rt/rtt_uart" DIR_SRCS)
AUX_SOURCE_DIRECTORY("modbus_rt/modbus_rt/agile_modbus" DIR_SRCS)
AUX_SOURCE_DIRECTORY("modbus_rt/modbus_rt/modbus_rt" DIR_SRCS)
AUX_SOURCE_DIRECTORY("modbus_rt/modbus_rt/platform/rt_thread" DIR_SRCS)
AUX_SOURCE_DIRECTORY("modbus_rt/modbus_rt/slave_util" DIR_SRCS)


AUX_SOURCE_DIRECTORY("../../firmware/CMSIS/device" DIR_SRCS)

#rt-thread src
AUX_SOURCE_DIRECTORY("../../middlewares/rt-thread/src" DIR_SRCS)
AUX_SOURCE_DIRECTORY("../../middlewares/rt-thread/libcpu/common" DIR_SRCS)
AUX_SOURCE_DIRECTORY("../../middlewares/rt-thread/libcpu/cortex-m4" DIR_SRCS)

#AUX_SOURCE_DIRECTORY("../../middlewares/rt-thread/components/libc/compilers/newlib" DIR_SRCS)
AUX_SOURCE_DIRECTORY("../../middlewares/rt-thread/components/libc/compilers/minilibc" DIR_SRCS)

AUX_SOURCE_DIRECTORY("../../middlewares/rt-thread/components/finsh" DIR_SRCS)
AUX_SOURCE_DIRECTORY("../../middlewares/rt-thread/components/drivers/src" DIR_SRCS)
AUX_SOURCE_DIRECTORY("../../middlewares/rt-thread/components/drivers/serial" DIR_SRCS)
AUX_SOURCE_DIRECTORY("../../middlewares/rt-thread/components/drivers/spi/sfud/src" DIR_SRCS)


set(STARTUP_ASM ../../firmware/CMSIS/device/startup/startup_n32g43x_gcc.s)

set(RTTHREAD_ASM ../../middlewares/rt-thread/libcpu/cortex-m4/context_gcc_w.s)

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
    "../../firmware/n32g43x_std_periph_driver/src/misc.c"
    "../../firmware/n32g43x_std_periph_driver/src/n32g43x_exti.c"
    "../../firmware/n32g43x_std_periph_driver/src/n32g43x_rcc.c"
    "../../firmware/n32g43x_std_periph_driver/src/n32g43x_gpio.c"
    "../../firmware/n32g43x_std_periph_driver/src/n32g43x_flash.c"
    "../../firmware/n32g43x_std_periph_driver/src/n32g43x_pwr.c"
    "../../firmware/n32g43x_std_periph_driver/src/n32g43x_usart.c"
    "../../firmware/n32g43x_std_periph_driver/src/n32g43x_i2c.c"
    "../../firmware/n32g43x_std_periph_driver/src/n32g43x_spi.c"
    "../../firmware/n32g43x_std_periph_driver/src/n32g43x_rtc.c"
    "../../firmware/n32g43x_std_periph_driver/src/n32g43x_adc.c"
    #rt-thread
    ${RTTHREAD_ASM}
    "../../middlewares/rt-thread/components/drivers/misc/pin.c"
    "../../middlewares/rt-thread/components/drivers/misc/adc.c"
    "../../middlewares/rt-thread/components/drivers/rtc/rtc.c"
    #spi
    "../../middlewares/rt-thread/components/drivers/spi/spi_core.c"
    "../../middlewares/rt-thread/components/drivers/spi/spi_dev.c"
    "../../middlewares/rt-thread/components/drivers/spi/spi_flash_sfud.c"
    "../../middlewares/rt-thread/components/drivers/spi/qspi_core.c"
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

