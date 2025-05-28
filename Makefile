# ------------------------------------------------
#
# @file Makefile (based on gcc)
# @author huanxuantian@msn.cn
# @version v1.0.0
#
# ChangeLog :
#   2024-08-01
# ------------------------------------------------
######################################
# path resolve
######################################
MFILE_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
CUR_MFILE_PATH := $(dir $(MFILE_PATH))

DIR_ROOT := $(CUR_MFILE_PATH)

ROOT = $(PWD)

SUBDIR = $(DIR_ROOT)/projects/Bootloader/GCC
SUBDIR += $(DIR_ROOT)/projects/n32g43x_RTT_UART/GCC

define make_subdir
@for i in $(SUBDIR); do \
	(cd $$i && make $1) \
done;
endef

ALL:
	$(call make_subdir)

clean:
	$(call make_subdir, clean)

dowmload:
	$(call make_subdir, download)