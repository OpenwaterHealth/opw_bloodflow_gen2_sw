ifeq ($(TARGET_CPU), $(filter $(TARGET_CPU), x86_64 A72))

include $(PRELUDE)

TARGET      := vx_app_openwater
TARGETTYPE  := exe

CSOURCES    := main.c
CSOURCES    += network.c
CSOURCES    += lsw_laser_pdu.c
CSOURCES    += system_state.c
CSOURCES    += app_histo_module.c
CSOURCES    += app_cust_cap_module.c
CSOURCES    += app_streaming_module.c
CSOURCES    += inputStream.c
CSOURCES    += inputStreamVisionFile.c
CSOURCES    += encrypt_data.c
CSOURCES    += app_commands.c
CSOURCES    += pymodule_lib.c
CSOURCES    += owconnector.c

ifeq ($(TARGET_CPU),x86_64)
include $(VISION_APPS_PATH)/apps/concerto_x86_64_inc.mak
CSOURCES    += main_x86.c
# Not building for PC
SKIPBUILD=1
endif

ifeq ($(TARGET_CPU),A72)
ifeq ($(TARGET_OS), $(filter $(TARGET_OS), LINUX QNX))
include $(VISION_APPS_PATH)/apps/concerto_a72_inc.mak
CSOURCES    += main_linux_arm.c
endif
endif

IDIRS += $(IMAGING_IDIRS)
IDIRS += $(VISION_APPS_KERNELS_IDIRS)
IDIRS += $(VISION_APPS_MODULES_IDIRS)
IDIRS += $(TARGETFS)/usr/include
IDIRS += $(TARGETFS)/usr/include/python3.8

STATIC_LIBS += $(IMAGING_LIBS)
STATIC_LIBS += $(VISION_APPS_KERNELS_LIBS)
STATIC_LIBS += $(VISION_APPS_MODULES_LIBS)

SYS_SHARED_LIBS += ssl crypto tar python3.8

include $(FINALE)

endif
