-include $(TOPDIR)/Make.defs
-include $(SDKDIR)/Make.defs

CONFIG_APS_TEMPLATE_ASMP_PRIORITY ?= SCHED_PRIORITY_DEFAULT
CONFIG_APS_TEMPLATE_ASMP_STACKSIZE ?= 2048

APPNAME = aps_template_asmp
PRIORITY = $(CONFIG_APS_TEMPLATE_ASMP_PRIORITY)
STACKSIZE = $(CONFIG_APS_TEMPLATE_ASMP_STACKSIZE)

MAINSRC = aps_template_asmp_main.c
ASRCS =
CSRCS = $(filter-out $(MAINSRC),$(wildcard *.c) $(wildcard */*.c))
CXXSRCS = $(wildcard *.cpp) $(wildcard */*.cpp) $(wildcard *.cxx) $(wildcard */*.cxx)

CFLAGS += $(foreach inc,$(wildcard ../*/include),-I$(inc))
CXXFLAGS += $(foreach inc,$(wildcard ../*/include),-I$(inc))

CONFIG_APS_TEMPLATE_ASMP_PROGNAME ?= aps_template_asmp$(EXEEXT)
PROGNAME = $(CONFIG_APS_TEMPLATE_ASMP_PROGNAME)

include $(APPDIR)/Application.mk
