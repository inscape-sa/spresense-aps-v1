-include $(TOPDIR)/Make.defs
-include $(SDKDIR)/Make.defs

CONFIG_APS_GPIO_ONOFF_PRIORITY ?= SCHED_PRIORITY_DEFAULT
CONFIG_APS_GPIO_ONOFF_STACKSIZE ?= 2048

APPNAME = aps_gpio_onoff
PRIORITY = $(CONFIG_APS_GPIO_ONOFF_PRIORITY)
STACKSIZE = $(CONFIG_APS_GPIO_ONOFF_STACKSIZE)

MAINSRC = aps_gpio_onoff_main.c
ASRCS =
CSRCS = $(filter-out $(MAINSRC),$(wildcard *.c) $(wildcard */*.c))
CXXSRCS = $(wildcard *.cpp) $(wildcard */*.cpp) $(wildcard *.cxx) $(wildcard */*.cxx)

CFLAGS += $(foreach inc,$(wildcard ../*/include),-I$(inc))
CXXFLAGS += $(foreach inc,$(wildcard ../*/include),-I$(inc))

CONFIG_APS_GPIO_ONOFF_PROGNAME ?= aps_gpio_onoff$(EXEEXT)
PROGNAME = $(CONFIG_APS_GPIO_ONOFF_PROGNAME)

include $(APPDIR)/Application.mk
