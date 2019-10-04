-include $(TOPDIR)/Make.defs
-include $(SDKDIR)/Make.defs

CONFIG_APS00_SANDBOX_APS_MAINCORE_PRIORITY ?= SCHED_PRIORITY_DEFAULT
CONFIG_APS00_SANDBOX_APS_MAINCORE_STACKSIZE ?= 2048

APPNAME = aps_maincore
PRIORITY = $(CONFIG_APS00_SANDBOX_APS_MAINCORE_PRIORITY)
STACKSIZE = $(CONFIG_APS00_SANDBOX_APS_MAINCORE_STACKSIZE)

ASRCS =
CSRCS = $(wildcard *.c) $(wildcard */*.c)
CXXSRCS = $(wildcard *.cpp) $(wildcard */*.cpp) $(wildcard *.cxx) $(wildcard */*.cxx)
MAINSRC = aps_maincore_main.c

CFLAGS += $(foreach inc,$(wildcard ../*/include),-I$(inc))
CXXFLAGS += $(foreach inc,$(wildcard ../*/include),-I$(inc))

CONFIG_APS00_SANDBOX_APS_MAINCORE_PROGNAME ?= aps_maincore$(EXEEXT)
PROGNAME = $(CONFIG_APS00_SANDBOX_APS_MAINCORE_PROGNAME)

include $(APPDIR)/Application.mk
