-include $(TOPDIR)/Make.defs
-include $(SDKDIR)/Make.defs

CONFIG_APS_MULTICORE_PRIORITY ?= SCHED_PRIORITY_DEFAULT
CONFIG_APS_MULTICORE_STACKSIZE ?= 2048

APPNAME = aps_multicore
PRIORITY = $(CONFIG_APS_MULTICORE_PRIORITY)
STACKSIZE = $(CONFIG_APS_MULTICORE_STACKSIZE)

MAINSRC = aps_multicore_main.c
ASRCS =
CSRCS = $(filter-out $(MAINSRC),$(wildcard *.c) $(wildcard */*.c))
CXXSRCS = $(wildcard *.cpp) $(wildcard */*.cpp) $(wildcard *.cxx) $(wildcard */*.cxx)

CFLAGS += $(foreach inc,$(wildcard ../*/include),-I$(inc))
CXXFLAGS += $(foreach inc,$(wildcard ../*/include),-I$(inc))

CONFIG_APS_MULTICORE_PROGNAME ?= aps_multicore$(EXEEXT)
PROGNAME = $(CONFIG_APS_MULTICORE_PROGNAME)

include $(APPDIR)/Application.mk
