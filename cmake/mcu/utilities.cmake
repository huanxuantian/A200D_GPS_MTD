function(mcu_util_create_family_targets MCU_TYPE)

    if(NOT (TARGET ${MCU_TYPE}))
        add_library(${MCU_TYPE} INTERFACE IMPORTED)
        # Set compiler flags for target
        # -Wall: all warnings activated
        # -ffunction-sections -fdata-sections: remove unused code
        target_compile_options(${MCU_TYPE} INTERFACE 
            --sysroot="${TOOLCHAIN_SYSROOT}"
            -mthumb -Wall -ffunction-sections -fdata-sections
        )
        # Set linker flags
        # -mthumb: Generate thumb code
        # -Wl,--gc-sections: Remove unused code
        target_link_options(${MCU_TYPE} INTERFACE 
            --sysroot="${TOOLCHAIN_SYSROOT}"
            -mthumb -Wl,--gc-sections
            -Wl,--print-memory-usage
        )
        target_compile_definitions(${MCU_TYPE} INTERFACE 
            MCU_${MCU_TYPE}
        )
    endif()
endfunction()
#riscv without -mthumb option
function(riscv_util_create_family_targets MCU_TYPE)

    if(NOT (TARGET ${MCU_TYPE}))
        add_library(${MCU_TYPE} INTERFACE IMPORTED)
        # Set compiler flags for target
        # -Wall: all warnings activated
        # -ffunction-sections -fdata-sections: remove unused code
        target_compile_options(${MCU_TYPE} INTERFACE 
            --sysroot="${TOOLCHAIN_SYSROOT}"
            -Wall -ffunction-sections -fdata-sections
            -fmessage-length=0 -fsigned-char -fno-common -fsingle-precision-constant
        )
        # Set linker flags
        # -Wl,--gc-sections: Remove unused code
        target_link_options(${MCU_TYPE} INTERFACE 
            --sysroot="${TOOLCHAIN_SYSROOT}"
            -Wl,--gc-sections 
            -Wl,--print-memory-usage
        )
        target_compile_definitions(${MCU_TYPE} INTERFACE 
            MCU_${MCU_TYPE}
        )
    endif()
endfunction()