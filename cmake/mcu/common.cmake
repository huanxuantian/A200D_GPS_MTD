
if(NOT MCU_TOOLCHAIN_PATH)
    if(DEFINED ENV{MCU_TOOLCHAIN_PATH})
        message(STATUS "Detected toolchain path MCU_TOOLCHAIN_PATH in environmental variables: ")
        message(STATUS "$ENV{MCU_TOOLCHAIN_PATH}")
        set(MCU_TOOLCHAIN_PATH $ENV{MCU_TOOLCHAIN_PATH})
    endif()
    if(NOT CMAKE_C_COMPILER)
        set(MCU_TOOLCHAIN_PATH "/usr")
        message(STATUS "No MCU_TOOLCHAIN_PATH specified, using default: " ${MCU_TOOLCHAIN_PATH})
    else()
        # keep only directory of compiler
        get_filename_component(MCU_TOOLCHAIN_PATH ${CMAKE_C_COMPILER} DIRECTORY)
        # remove the last /bin directory
        get_filename_component(MCU_TOOLCHAIN_PATH ${MCU_TOOLCHAIN_PATH} DIRECTORY)
    endif()
    file(TO_CMAKE_PATH "${MCU_TOOLCHAIN_PATH}" MCU_TOOLCHAIN_PATH)
endif()

if(NOT MCU_TARGET_TRIPLET)
    set(MCU_TARGET_TRIPLET "arm-none-eabi")
    message(STATUS "No MCU_TARGET_TRIPLET specified, using default: " ${MCU_TARGET_TRIPLET})
endif()

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
#set(CMAKE_SYSTEM_PROCESSOR riscv)

set(TOOLCHAIN_SYSROOT  "${MCU_TOOLCHAIN_PATH}/${MCU_TARGET_TRIPLET}")
set(TOOLCHAIN_BIN_PATH "${MCU_TOOLCHAIN_PATH}/bin")
set(TOOLCHAIN_INC_PATH "${MCU_TOOLCHAIN_PATH}/${MCU_TARGET_TRIPLET}/include")
set(TOOLCHAIN_LIB_PATH "${MCU_TOOLCHAIN_PATH}/${MCU_TARGET_TRIPLET}/lib")

find_program(CMAKE_OBJCOPY NAMES ${MCU_TARGET_TRIPLET}-objcopy PATHS ${TOOLCHAIN_BIN_PATH})
find_program(CMAKE_OBJDUMP NAMES ${MCU_TARGET_TRIPLET}-objdump PATHS ${TOOLCHAIN_BIN_PATH})
find_program(CMAKE_SIZE NAMES ${MCU_TARGET_TRIPLET}-size PATHS ${TOOLCHAIN_BIN_PATH})
find_program(CMAKE_DEBUGGER NAMES ${MCU_TARGET_TRIPLET}-gdb PATHS ${TOOLCHAIN_BIN_PATH})
find_program(CMAKE_CPPFILT NAMES ${MCU_TARGET_TRIPLET}-c++filt PATHS ${TOOLCHAIN_BIN_PATH})

function(mcu_print_size_of_target TARGET)
    add_custom_target(${TARGET}_always_display_size
        ALL COMMAND ${CMAKE_SIZE} ${TARGET}${CMAKE_EXECUTABLE_SUFFIX_C}
        COMMENT "Target Sizes: "
        DEPENDS ${TARGET}
    )
endfunction()

function(mcu_generate_binary_file TARGET)
    add_custom_command(
        TARGET ${TARGET}
        POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O binary ${TARGET}${CMAKE_EXECUTABLE_SUFFIX_C} ${TARGET}.bin
        BYPRODUCTS ${TARGET}.bin
        COMMENT "Generating binary file ${CMAKE_PROJECT_NAME}.bin"
    )
endfunction()

function(mcu_generate_hex_file TARGET)
    add_custom_command(
        TARGET ${TARGET}
        POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O ihex ${TARGET}${CMAKE_EXECUTABLE_SUFFIX_C} ${TARGET}.hex
        BYPRODUCTS ${TARGET}.hex
        COMMENT "Generating hex file ${CMAKE_PROJECT_NAME}.hex"
    )
endfunction()

function(mcu_generate_dump_file TARGET)
    add_custom_command(
        TARGET ${TARGET}
        POST_BUILD
        COMMAND ${CMAKE_OBJDUMP} --all-headers --demangle --disassemble ${TARGET}${CMAKE_EXECUTABLE_SUFFIX_C} > ${TARGET}.lst
        BYPRODUCTS ${TARGET}.lst
        COMMENT "Generating list file ${CMAKE_PROJECT_NAME}.lst"
    )
endfunction()


function(mcu_add_linker_script TARGET VISIBILITY SCRIPT)
    get_filename_component(SCRIPT "${SCRIPT}" ABSOLUTE)
    target_link_options(${TARGET} ${VISIBILITY} -T "${SCRIPT}")

    get_target_property(TARGET_TYPE ${TARGET} TYPE)
    if(TARGET_TYPE STREQUAL "INTERFACE_LIBRARY")
        set(INTERFACE_PREFIX "INTERFACE_")
    endif()

    get_target_property(LINK_DEPENDS ${TARGET} ${INTERFACE_PREFIX}LINK_DEPENDS)
    if(LINK_DEPENDS)
        list(APPEND LINK_DEPENDS "${SCRIPT}")        
    else()
        set(LINK_DEPENDS "${SCRIPT}")
    endif()


    set_target_properties(${TARGET} PROPERTIES ${INTERFACE_PREFIX}LINK_DEPENDS "${LINK_DEPENDS}")
endfunction()

if(NOT (TARGET MCU::NoSys))
    add_library(MCU::NoSys INTERFACE IMPORTED)
    target_compile_options(MCU::NoSys INTERFACE $<$<C_COMPILER_ID:GNU>:--specs=nosys.specs>)
    target_link_options(MCU::NoSys INTERFACE $<$<C_COMPILER_ID:GNU>:--specs=nosys.specs>)
endif()

if(NOT (TARGET MCU::Nano))
    add_library(MCU::Nano INTERFACE IMPORTED)
    target_compile_options(MCU::Nano INTERFACE $<$<C_COMPILER_ID:GNU>:--specs=nano.specs>)
    target_link_options(MCU::Nano INTERFACE $<$<C_COMPILER_ID:GNU>:--specs=nano.specs>)
endif()

if(NOT (TARGET MCU::Nano::FloatPrint))
    add_library(MCU::Nano::FloatPrint INTERFACE IMPORTED)
    target_link_options(MCU::Nano::FloatPrint INTERFACE
        $<$<C_COMPILER_ID:GNU>:-Wl,--undefined,_printf_float>
    )
endif()

if(NOT (TARGET MCU::Nano::FloatScan))
    add_library(MCU::Nano::FloatScan INTERFACE IMPORTED)
    target_link_options(MCU::Nano::FloatPrint INTERFACE
        $<$<C_COMPILER_ID:GNU>:-Wl,--undefined,_scanf_float>
    )
endif()

include(mcu/utilities)