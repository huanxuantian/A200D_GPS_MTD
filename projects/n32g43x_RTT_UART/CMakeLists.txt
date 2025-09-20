cmake_minimum_required(VERSION 3.16)

set(TOP_BASE "../../")
#set(MCU_TOOLCHAIN_PATH /config/arm/gcc/)
#set(MCU_TARGET_TRIPLET arm-none-eabi)
#set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/${TOP_BASE}cmake/mcu_gcc.cmake)

set(MCU_TYPE "N32G43x")
project(rtt-modbus  C ASM)

option(USE_DOXYGEN_DOC "build option doc with doxygen extend(default no)" OFF)

# #rt-thread core
# INCLUDE_DIRECTORIES("${TOP_BASE}middlewares/rt-thread/include")
# INCLUDE_DIRECTORIES("${TOP_BASE}middlewares/rt-thread/include/libc")
# INCLUDE_DIRECTORIES("${TOP_BASE}middlewares/rt-thread/components/libc/compilers/newlib")
# INCLUDE_DIRECTORIES("${TOP_BASE}middlewares/rt-thread/components/libc/compilers/minilibc")
# #rtt components
# INCLUDE_DIRECTORIES("${TOP_BASE}middlewares/rt-thread/components/drivers/include")
# INCLUDE_DIRECTORIES("${TOP_BASE}middlewares/rt-thread/components/drivers/include/drivers")
# INCLUDE_DIRECTORIES("${TOP_BASE}middlewares/rt-thread/components/drivers/include/ipc")
# INCLUDE_DIRECTORIES("${TOP_BASE}middlewares/rt-thread/components/finsh")
# #spi
# INCLUDE_DIRECTORIES("${TOP_BASE}middlewares/rt-thread/components/drivers/spi")
# INCLUDE_DIRECTORIES("${TOP_BASE}middlewares/rt-thread/components/drivers/spi/sfud/inc")

# INCLUDE_DIRECTORIES("APP/inc")

set(SYS_INCDIR 
    # app main
    "APP/inc"
    #rt-thread core
    "${TOP_BASE}middlewares/rt-thread/include"
    "${TOP_BASE}middlewares/rt-thread/include/libc"
    "${TOP_BASE}middlewares/rt-thread/components/libc/compilers/newlib"
    "${TOP_BASE}middlewares/rt-thread/components/libc/compilers/minilibc"
    #rtt components
    "${TOP_BASE}middlewares/rt-thread/components/drivers/include"
    "${TOP_BASE}middlewares/rt-thread/components/drivers/include/drivers"
    "${TOP_BASE}middlewares/rt-thread/components/drivers/include/ipc"
    "${TOP_BASE}middlewares/rt-thread/components/finsh"
    #spi
    "${TOP_BASE}middlewares/rt-thread/components/drivers/spi"
    "${TOP_BASE}middlewares/rt-thread/components/drivers/spi/sfud/inc"
)

INCLUDE_DIRECTORIES(${SYS_INCDIR})

# #drv
# INCLUDE_DIRECTORIES("DeviceDrivers/gpio/inc")
# INCLUDE_DIRECTORIES("DeviceDrivers/adc/inc")
# INCLUDE_DIRECTORIES("DeviceDrivers/spi/inc")
# INCLUDE_DIRECTORIES("DeviceDrivers/i2c/inc")
# INCLUDE_DIRECTORIES("DeviceDrivers/uart/inc")
# INCLUDE_DIRECTORIES("DeviceDrivers/rtc/inc")

# #core
# INCLUDE_DIRECTORIES("${TOP_BASE}firmware/CMSIS/core")
# INCLUDE_DIRECTORIES("${TOP_BASE}firmware/CMSIS/device")
# INCLUDE_DIRECTORIES("${TOP_BASE}firmware/n32g43x_std_periph_driver/inc")
# INCLUDE_DIRECTORIES("bsp")
# INCLUDE_DIRECTORIES("gnss_sk")
# INCLUDE_DIRECTORIES("business")
# INCLUDE_DIRECTORIES("gsm")
# INCLUDE_DIRECTORIES("protocol")

set(BSP_INCDIR
    #drv
    "DeviceDrivers/gpio/inc"
    "DeviceDrivers/adc/inc"
    "DeviceDrivers/spi/inc"
    "DeviceDrivers/i2c/inc"
    "DeviceDrivers/uart/inc"
    "DeviceDrivers/rtc/inc"
    #core
    "${TOP_BASE}firmware/CMSIS/core"
    "${TOP_BASE}firmware/CMSIS/device"
    "${TOP_BASE}firmware/n32g43x_std_periph_driver/inc"
    #app bsp code
    "bsp"
    "gnss_sk"
    "business"
    "gsm"
    "protocol"
)

INCLUDE_DIRECTORIES(${BSP_INCDIR})

#modbus_rt inc
# INCLUDE_DIRECTORIES("${TOP_BASE}middlewares/modbus_rt/rtt_uart")
# INCLUDE_DIRECTORIES("${TOP_BASE}middlewares/modbus_rt/modbus_rt/modbus_rt")
# INCLUDE_DIRECTORIES("${TOP_BASE}middlewares/modbus_rt/modbus_rt/agile_modbus")
# INCLUDE_DIRECTORIES("${TOP_BASE}middlewares/modbus_rt/modbus_rt/platform/rt_thread")
# INCLUDE_DIRECTORIES("${TOP_BASE}middlewares/modbus_rt/modbus_rt/slave_util")
set(MODBUS_LIB_INC
    "${TOP_BASE}middlewares/modbus_rt/rtt_uart"
    "${TOP_BASE}middlewares/modbus_rt/modbus_rt/modbus_rt"
    "${TOP_BASE}middlewares/modbus_rt/modbus_rt/agile_modbus"
    "${TOP_BASE}middlewares/modbus_rt/modbus_rt/platform/rt_thread"
    "${TOP_BASE}middlewares/modbus_rt/modbus_rt/slave_util"
)

# INCLUDE_DIRECTORIES("${TOP_BASE}middlewares/gnss_port")
# INCLUDE_DIRECTORIES("${TOP_BASE}middlewares/gps")
# INCLUDE_DIRECTORIES("${TOP_BASE}middlewares/nmea")


# INCLUDE_DIRECTORIES("${TOP_BASE}middlewares/sensor")

# INCLUDE_DIRECTORIES("${TOP_BASE}middlewares/utils")
# INCLUDE_DIRECTORIES("${TOP_BASE}middlewares/struct2json")
# INCLUDE_DIRECTORIES("${TOP_BASE}middlewares/cip")

# INCLUDE_DIRECTORIES("${TOP_BASE}middlewares/http")
set(MISC_LIB_INC
    "${TOP_BASE}middlewares/utils"
    "${TOP_BASE}middlewares/struct2json"
    "${TOP_BASE}middlewares/gnss_port"
    "${TOP_BASE}middlewares/gps"
    "${TOP_BASE}middlewares/nmea"
    "${TOP_BASE}middlewares/sensor"
    "${TOP_BASE}middlewares/cip"
    "${TOP_BASE}middlewares/http"
)

set(APP_EXTEND_INCLUDES
    ${MODBUS_LIB_INC}
    ${MISC_LIB_INC}
)
INCLUDE_DIRECTORIES(${APP_EXTEND_INCLUDES})

#sources
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

AUX_SOURCE_DIRECTORY("${TOP_BASE}middlewares/sensor" DIR_SRCS)
AUX_SOURCE_DIRECTORY("bsp" DIR_SRCS)
AUX_SOURCE_DIRECTORY("business" DIR_SRCS)
AUX_SOURCE_DIRECTORY("${TOP_BASE}middlewares/utils" DIR_SRCS)
AUX_SOURCE_DIRECTORY("${TOP_BASE}middlewares/struct2json" DIR_SRCS)
AUX_SOURCE_DIRECTORY("${TOP_BASE}middlewares/cip" DIR_SRCS)
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
add_definitions("-DI2CSENSOR")
add_definitions("-DDEBUG_APP")
add_definitions("-DDEBUG_UART1")
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
    #i2c
    "${TOP_BASE}middlewares/rt-thread/components/drivers/i2c/i2c_core.c"
    "${TOP_BASE}middlewares/rt-thread/components/drivers/i2c/i2c_dev.c"
    "${TOP_BASE}middlewares/rt-thread/components/drivers/i2c/i2c-bit-ops.c"
)

message("#########################################")
message( "DEVICE: ${MCU_TYPE} for Project ${PROJECT_NAME}" )
message("#########################################")


add_executable(${PROJECT_NAME} ${PROJECT_SOURCES} )
target_link_libraries(${PROJECT_NAME} MCU::NoSys ${MCU_TYPE})

target_compile_options(${PROJECT_NAME} PUBLIC "-Os")
mcu_add_linker_script(${PROJECT_NAME} PUBLIC ${LINKER_SCRIPT})


target_link_options(${PROJECT_NAME} PUBLIC -Xlinker -Map=${PROJECT_NAME}.map)

mcu_generate_binary_file(${PROJECT_NAME})
mcu_generate_hex_file(${PROJECT_NAME})
mcu_print_size_of_target(${PROJECT_NAME})


#for doxygen doc build for this project
IF(USE_DOXYGEN_DOC)
#srcs for build project
set(DOC_SRCS ${PROJECT_SOURCES} ${APP_EXTEND_SOURCES})
#format cmake list to dosygen list
foreach(line IN LISTS DOC_SRCS)
    string(APPEND DOXYGEN_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${line} \\ \n")
endforeach() 
string(APPEND DOXYGEN_SOURCES ". \n")
#incs for build project
set( DOC_INCS ${SYS_INCDIR} ${BSP_INCDIR} ${APP_EXTEND_INCLUDES})
#format cmake list to dosygen list 
foreach(line IN LISTS DOC_INCS)
    string(APPEND DOXYGEN_INCS "${CMAKE_CURRENT_SOURCE_DIR}/${line} \\ \n")
endforeach() 
string(APPEND DOXYGEN_INCS ". \n")

find_package(Doxygen)
if(DOXYGEN_FOUND)
   configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
   add_custom_target(doc ALL
       ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
       WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
       COMMENT "Generating API documentation with Doxygen" VERBATIM
   )
   MESSAGE("DOXYGEN FOUND!!!")
else(DOXYGEN_FOUND)
MESSAGE("DOXYGEN not FOUND!!!")
endif(DOXYGEN_FOUND)

ENDIF(USE_DOXYGEN_DOC)
