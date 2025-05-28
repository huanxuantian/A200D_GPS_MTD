# ------------------------------------------------
#
# @file Makefile (based on gcc)
# @author Dewin
# @version v1.0.0
#
# ChangeLog :
#   2021-07-20
# ------------------------------------------------

######################################
# path resolve
######################################
MFILE_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
CUR_MFILE_PATH := $(dir $(MFILE_PATH))

DIR_ROOT := $(CUR_MFILE_PATH)

######################################
# target
######################################
TARGET = boot-iap

######################################
# building variables
######################################
# debug build
ifeq ($(release), y)
DEBUG = 0
else
DEBUG = 1
endif
# optimization
OPT = -Os

#######################################
# Build path
#######################################
BUILD_DIR = boot_build

######################################
# chip platform info
######################################
TARGET_PLATFORM := n32g43x
DEFS += -DN32G43X
DEFS += -DUSE_STDPERIPH_DRIVER

######################################
# Algo libs
######################################
USELIB = 0

######################################
#TOOLS CHAIN
######################################
CROSS_COMPILE = arm-none-eabi-

######################################
# C sources
######################################
C_DIRS += $(DIR_ROOT)/../APP/src
#C_DIRS += $(DIR_ROOT)/../../../bsp/src
C_DIRS += $(DIR_ROOT)/../../../firmware/CMSIS/device
#C_DIRS += $(DIR_ROOT)/../../../firmware/$(TARGET_PLATFORM)_std_periph_driver/src
#C_DIRS += $(DIR_ROOT)/../../../../../../firmware/$(TARGET_PLATFORM)_usbfs_driver/src
C_DIRS += $(DIR_ROOT)/../update
#nanosys
#C_DIRS += $(DIR_ROOT)/../gcc-nano

SRC_OBJS_DIRS += $(foreach DIR, $(C_DIRS), $(wildcard $(DIR)/*.c))
C_SOURCES = $(SRC_OBJS_DIRS)
#单独的引用源文件
C_SOURCES += $(DIR_ROOT)/../../../firmware/$(TARGET_PLATFORM)_std_periph_driver/src/n32g43x_gpio.c
C_SOURCES += $(DIR_ROOT)/../../../firmware/$(TARGET_PLATFORM)_std_periph_driver/src/n32g43x_rcc.c
C_SOURCES += $(DIR_ROOT)/../../../firmware/$(TARGET_PLATFORM)_std_periph_driver/src/n32g43x_spi.c
C_SOURCES += $(DIR_ROOT)/../../../firmware/$(TARGET_PLATFORM)_std_periph_driver/src/n32g43x_usart.c
C_SOURCES += $(DIR_ROOT)/../../../firmware/$(TARGET_PLATFORM)_std_periph_driver/src/misc.c
C_SOURCES +=

######################################
# ASM sources
######################################
ASM_SOURCES = $(DIR_ROOT)/../../../firmware/CMSIS/device/startup/startup_$(TARGET_PLATFORM)_gcc.s
#rt-thread s file
ASM_SOURCES += 

RTT_ASM_SOURCES = 

######################################
# C includes
######################################
C_INCS += $(DIR_ROOT)/../APP/inc
C_INCS += $(DIR_ROOT)/../update
#C_INCS += $(DIR_ROOT)/../../../../bsp/inc
C_INCS += $(DIR_ROOT)/../../../firmware/CMSIS/core
C_INCS += $(DIR_ROOT)/../../../firmware/CMSIS/device
C_INCS += $(DIR_ROOT)/../../../firmware/$(TARGET_PLATFORM)_std_periph_driver/inc
#C_INCS += $(DIR_ROOT)/../../../../../../firmware/$(TARGET_PLATFORM)_usbfs_driver/inc



INCS_OBJS_DIR = $(foreach DIR2, $(C_INCS), $(wildcard $(DIR2)/*.h))
INCS_OBJS_PATH = $(sort $(dir $(INCS_OBJS_DIR)))
C_INCLUDES = $(addprefix -I,$(INCS_OBJS_PATH))

######################################
# Lib files
######################################
ifeq ($(USELIB), 1)
SRC_LIB_DIR += $(DIR_ROOT)/../../../firmware/$(TARGET_PLATFORM)_algo_lib/src
INC_LIB_DIR += $(DIR_ROOT)/../../../firmware/$(TARGET_PLATFORM)_algo_lib/inc
LIB_SOURCES = $(foreach DIR3, $(SRC_LIB_DIR), $(wildcard $(DIR3)/*.lib))
LIB_SOURCES_L = $(addprefix -L,$(LIB_SOURCES))
C_LIBS = $(LIB_SOURCES_L)
LIB_INCS = $(foreach DIR4, $(INC_LIB_DIR), $(wildcard $(DIR4)/*.h))
LIB_INCS_PATH = $(sort $(dir $(LIB_INCS)))
LIB_INCS_PATH_I = $(addprefix -I,$(LIB_INCS_PATH))
C_INCLUDES += $(LIB_INCS_PATH_I)
endif

######################################
# Compile & Link flags
######################################
# cpu
CPU = -mcpu=cortex-m4
# fpu
FPU = -mfpu=fpv4-sp-d16
# float-abi
FLOAT-ABI = -mfloat-abi=hard
# mcu
MCU = $(CPU) -mthumb $(FPU) $(FLOAT-ABI)
#CFLAGS
CFLAGS += $(MCU) -Wall
CFLAGS += $(OPT)
CFLAGS += -ffunction-sections -fdata-sections
#DEBUG
ifeq ($(DEBUG), 1)
CFLAGS += -g -gdwarf-2
endif
# Generate dependency information
CFLAGS += -MMD -MP -MF"$(@:%.o=%.d)"
#LFLAGS
LFLAGS += $(MCU)
LFLAGS += -Wl,--gc-sections 
LFLAGS += --specs=nosys.specs
#LFLAGS += --specs=nano.specs
LFLAGS += -Xlinker -Map=$(BUILD_DIR)/$(TARGET).map
# link script
#LDSCRIPT = $(DIR_ROOT)/../../../firmware/CMSIS/device/$(TARGET_PLATFORM)_flash.ld
LDSCRIPT = $(DIR_ROOT)/../APP/link.lds

######################################
# Objects
######################################
all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin

# list of objects
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))
# list of ASM program objects .s
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))
# list of ASM program objects .S
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(RTT_ASM_SOURCES:.S=.o)))
vpath %.S $(sort $(dir $(RTT_ASM_SOURCES)))


$(BUILD_DIR)/%.o: %.c $(DIR_ROOT)/Makefile | $(BUILD_DIR) 
	$(CROSS_COMPILE)gcc $(CFLAGS) $(DEFS) $(C_INCLUDES) $(C_LIBS) -c -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.c=.lst)) $< -o $@
    
$(BUILD_DIR)/%.o: %.s $(DIR_ROOT)/Makefile | $(BUILD_DIR)
	$(CROSS_COMPILE)gcc $(CFLAGS) -c $< -o $@
	
$(BUILD_DIR)/%.o: %.S $(DIR_ROOT)/Makefile | $(BUILD_DIR)
	$(CROSS_COMPILE)gcc $(CFLAGS) -c $< -o $@
    
$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) $(DIR_ROOT)/Makefile
	$(CROSS_COMPILE)gcc $(OBJECTS) $(LFLAGS) -T$(LDSCRIPT) -o $@
	$(CROSS_COMPILE)size $@
	
$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	$(CROSS_COMPILE)objcopy -O binary -S $< $@

$(BUILD_DIR)/$(TARGET).hex: $(BUILD_DIR)/$(TARGET).elf
	$(CROSS_COMPILE)objcopy -O ihex -S $< $@
    
$(BUILD_DIR):
	@-mkdir $@

#######################################
# clean up
#######################################
clean:
	@-rm -rf $(BUILD_DIR)

#######################################
# dependencies
#######################################
-include $(wildcard $(BUILD_DIR)/*.d)

#######################################
# download .hex/.bin by jlink
#######################################
#Your JLink installation directory
PATH_WINPC = 'C:\\Program Files\\SEGGER\\JLink\\'
PATH_LINUX = /opt/SEGGER/JLink_V640b/
#JK_DPATH = $(PATH_LINUX)
JK_DPATH = $(PATH_WINPC)
#Jlink script store directory
JKS_DIR = $(DIR_ROOT)/../../../jlink
#Chip type
CHIP_TYPE = N32G435RB
download:
#@$(JK_DPATH)JLinkExe -device $(CHIP_TYPE) -if SWD -speed 4000 -autoconnect 1 -CommanderScript $(JKS_DIR)/boot-flash.jlink
	@$(JK_DPATH)JLink.exe -device $(CHIP_TYPE) -if SWD -speed 4000 -autoconnect 1 -CommanderScript $(JKS_DIR)/boot-flash.jlink
	@echo "Download Completed!"

debug:
	@$(JK_DPATH)JLinkGDBServer -select USB -device $(CHIP_TYPE) -if SWD -speed auto -noir -LocalhostOnly

# *** EOF ***
