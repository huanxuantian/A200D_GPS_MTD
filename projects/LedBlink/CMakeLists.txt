cmake_minimum_required(VERSION 3.16)

set(TOP_BASE "../../")
#set(MCU_TOOLCHAIN_PATH /config/arm/gcc/)
#set(MCU_TARGET_TRIPLET arm-none-eabi)
#set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/${TOP_BASE}cmake/mcu_gcc.cmake)

set(MCU_TYPE "N32G43x")
project(LedBlink C ASM)

INCLUDE_DIRECTORIES("APP/inc")
INCLUDE_DIRECTORIES("${TOP_BASE}firmware/CMSIS/core")
INCLUDE_DIRECTORIES("${TOP_BASE}firmware/CMSIS/device")
INCLUDE_DIRECTORIES("${TOP_BASE}firmware/n32g43x_std_periph_driver/inc")

AUX_SOURCE_DIRECTORY("APP/src" DIR_SRCS)
AUX_SOURCE_DIRECTORY("${TOP_BASE}firmware/CMSIS/device" DIR_SRCS)

set(STARTUP_ASM ${TOP_BASE}firmware/CMSIS/device/startup/startup_n32g43x_gcc.s)

set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/APP/link.lds)

add_definitions("-DUSE_STDPERIPH_DRIVER")
add_definitions("-DN32G43X")

set(PROJECT_SOURCES
    ${DIR_SRCS}
    ${STARTUP_ASM}
    "${TOP_BASE}firmware/n32g43x_std_periph_driver/src/n32g43x_gpio.c"
    "${TOP_BASE}firmware/n32g43x_std_periph_driver/src/n32g43x_rcc.c"
    "${TOP_BASE}firmware/n32g43x_std_periph_driver/src/misc.c"
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

