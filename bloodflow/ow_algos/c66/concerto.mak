
ifeq ($(TARGET_CPU), $(filter $(TARGET_CPU), X86 x86_64 C66 ))

include $(PRELUDE)
TARGET      := vx_target_kernels_ow_algos_c66
TARGETTYPE  := library
CSOURCES    := $(call all-c-files)
IDIRS       += $(VISION_APPS_PATH)/kernels/ow_algos/include
IDIRS       += $(VISION_APPS_PATH)/kernels/ow_algos/host
IDIRS       += $(VISION_APPS_PATH)/utils/perf_stats/include
IDIRS       += $(CGT6X_ROOT)/include
IDIRS       += $(PDK_PATH)/packages/
IDIRS       += $(PSDK_PATH)/imglib_c66x_3_2_0_1/packages
IDIRS       += $(TIOVX_PATH)/kernels/include
IDIRS       += $(VXLIB_PATH)/packages
# < DEVELOPER_TODO: Add any custom include paths using 'IDIRS' >
# < DEVELOPER_TODO: Add any custom preprocessor defines or build options needed using
#                   'CFLAGS'. >
# < DEVELOPER_TODO: Adjust which cores this library gets built on using 'SKIPBUILD'. >

ifeq ($(TARGET_CPU),C66)
DEFS += CORE_DSP
DEFS += SOC_J721E
CFLAGS += -o3 -mt -ms0 -k -mw -mh32 -mv6600 --symdebug:none
endif

ifeq ($(BUILD_BAM),yes)
DEFS += BUILD_BAM
endif

ifeq ($(TARGET_CPU), $(filter $(TARGET_CPU), x86_64))
DEFS += _HOST_BUILD _TMS320C6600 TMS320C66X HOST_EMULATION
endif

include $(FINALE)
endif