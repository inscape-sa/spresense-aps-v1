-include $(TOPDIR)/Make.defs
-include $(SDKDIR)/Make.defs

CONFIG_APS00_SANDBOX_APS_CAMERA_ASMP_PRIORITY ?= SCHED_PRIORITY_DEFAULT
CONFIG_APS00_SANDBOX_APS_CAMERA_ASMP_STACKSIZE ?= 2048

APPNAME = aps_camera_asmp
PRIORITY = $(CONFIG_APS00_SANDBOX_APS_CAMERA_ASMP_PRIORITY)
STACKSIZE = $(CONFIG_APS00_SANDBOX_APS_CAMERA_ASMP_STACKSIZE)

ASRCS =
CSRCS = $(wildcard *.c) $(wildcard */*.c)
CXXSRCS = $(wildcard *.cpp) $(wildcard */*.cpp) $(wildcard *.cxx) $(wildcard */*.cxx)
MAINSRC = aps_camera_asmp_main.c

CFLAGS += $(foreach inc,$(wildcard ../*/include),-I$(inc))
CXXFLAGS += $(foreach inc,$(wildcard ../*/include),-I$(inc))

CONFIG_APS00_SANDBOX_APS_CAMERA_ASMP_PROGNAME ?= aps_camera_asmp$(EXEEXT)
PROGNAME = $(CONFIG_APS00_SANDBOX_APS_CAMERA_ASMP_PROGNAME)

include $(APPDIR)/Application.mk
