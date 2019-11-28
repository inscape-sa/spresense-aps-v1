-include $(TOPDIR)/Make.defs
-include $(SDKDIR)/Make.defs

CONFIG_TEST_ASMP_PRIORITY ?= SCHED_PRIORITY_DEFAULT
CONFIG_TEST_ASMP_STACKSIZE ?= 2048

APPNAME = test_asmp
PRIORITY = $(CONFIG_TEST_ASMP_PRIORITY)
STACKSIZE = $(CONFIG_TEST_ASMP_STACKSIZE)

MAINSRC = test_asmp_main.c
ASRCS =
CSRCS = $(filter-out $(MAINSRC),$(wildcard *.c) $(wildcard */*.c))
CXXSRCS = $(wildcard *.cpp) $(wildcard */*.cpp) $(wildcard *.cxx) $(wildcard */*.cxx)

CFLAGS += $(foreach inc,$(wildcard ../*/include),-I$(inc))
CXXFLAGS += $(foreach inc,$(wildcard ../*/include),-I$(inc))

CONFIG_TEST_ASMP_PROGNAME ?= test_asmp$(EXEEXT)
PROGNAME = $(CONFIG_TEST_ASMP_PROGNAME)

include $(APPDIR)/Application.mk
