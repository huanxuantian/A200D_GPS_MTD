

mcu_util_create_family_targets(STM32F1)

target_compile_options(STM32F1 INTERFACE 
    -mcpu=cortex-m3
)
target_link_options(STM32F1 INTERFACE 
    -mcpu=cortex-m3
)

mcu_util_create_family_targets(N32G43x)

target_compile_options(N32G43x INTERFACE 
    -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard
)
target_link_options(N32G43x INTERFACE 
    -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard
)

riscv_util_create_family_targets(CH32V30x)

target_compile_options(CH32V30x INTERFACE 
    -march=rv32imafc -mabi=ilp32f 
    -mcmodel=medany -msmall-data-limit=8 -mno-save-restore
)
target_link_options(CH32V30x INTERFACE 
    -march=rv32imafc -mabi=ilp32f
	-nostartfiles 
)
