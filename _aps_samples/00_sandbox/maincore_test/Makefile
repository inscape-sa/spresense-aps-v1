-include $(TOPDIR)/Make.defs
-include $(SDKDIR)/Make.defs

CONFIG_00_SANDBOX_MAINCORE_TEST_PRIORITY ?= SCHED_PRIORITY_DEFAULT
CONFIG_00_SANDBOX_MAINCORE_TEST_STACKSIZE ?= 2048

APPNAME = maincore_test
PRIORITY = $(CONFIG_00_SANDBOX_MAINCORE_TEST_PRIORITY)
STACKSIZE = $(CONFIG_00_SANDBOX_MAINCORE_TEST_STACKSIZE)

ASRCS =
CSRCS = $(wildcard *.c) $(wildcard */*.c)
CXXSRCS = $(wildcard *.cpp) $(wildcard */*.cpp) $(wildcard *.cxx) $(wildcard */*.cxx)
MAINSRC = maincore_test_main.c

CFLAGS += $(foreach inc,$(wildcard ../*/include),-I$(inc))
CXXFLAGS += $(foreach inc,$(wildcard ../*/include),-I$(inc))

CONFIG_00_SANDBOX_MAINCORE_TEST_PROGNAME ?= maincore_test$(EXEEXT)
PROGNAME = $(CONFIG_00_SANDBOX_MAINCORE_TEST_PROGNAME)

include $(APPDIR)/Application.mk
