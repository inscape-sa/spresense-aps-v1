-include $(TOPDIR)/Make.defs
-include $(SDKDIR)/Make.defs

CONFIG_APS00_SANDBOX_APS_MULTICORE_PRIORITY ?= SCHED_PRIORITY_DEFAULT
CONFIG_APS00_SANDBOX_APS_MULTICORE_STACKSIZE ?= 2048

APPNAME = aps_multicore
PRIORITY = $(CONFIG_APS00_SANDBOX_APS_MULTICORE_PRIORITY)
STACKSIZE = $(CONFIG_APS00_SANDBOX_APS_MULTICORE_STACKSIZE)

ASRCS =
CSRCS = $(wildcard *.c) $(wildcard */*.c)
CXXSRCS = $(wildcard *.cpp) $(wildcard */*.cpp) $(wildcard *.cxx) $(wildcard */*.cxx)
MAINSRC = aps_multicore_main.c

CFLAGS += $(foreach inc,$(wildcard ../*/include),-I$(inc))
CXXFLAGS += $(foreach inc,$(wildcard ../*/include),-I$(inc))

CONFIG_APS00_SANDBOX_APS_MULTICORE_PROGNAME ?= aps_multicore$(EXEEXT)
PROGNAME = $(CONFIG_APS00_SANDBOX_APS_MULTICORE_PROGNAME)

include $(APPDIR)/Application.mk
