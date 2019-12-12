-include $(TOPDIR)/Make.defs
-include $(SDKDIR)/Make.defs

CONFIG_APS_CXX_AUDIO_DETECT_PRIORITY ?= SCHED_PRIORITY_DEFAULT
CONFIG_APS_CXX_AUDIO_DETECT_STACKSIZE ?= 2048

APPNAME = aps_cxx_audio_detect
PRIORITY = $(CONFIG_APS_CXX_AUDIO_DETECT_PRIORITY)
STACKSIZE = $(CONFIG_APS_CXX_AUDIO_DETECT_STACKSIZE)

# change .c -> .cxx
MAINSRC = aps_cxx_audio_detect_main.cxx
ASRCS =
CSRCS = 
# add $(filter-out $(MAINSRC),$(wildcard *.c) $(wildcard */*.c)) in CXXSRCS
CXXSRCS = $(filter-out $(MAINSRC),$(wildcard *.c) $(wildcard */*.c)) $(wildcard *.cpp) $(wildcard */*.cpp) $(wildcard *.cxx) $(wildcard */*.cxx)

CFLAGS += $(foreach inc,$(wildcard ../*/include),-I$(inc))
CXXFLAGS += $(foreach inc,$(wildcard ../*/include),-I$(inc))

CONFIG_APS_CXX_AUDIO_DETECT_PROGNAME ?= aps_cxx_audio_detect$(EXEEXT)
PROGNAME = $(CONFIG_APS_CXX_AUDIO_DETECT_PROGNAME)

include $(APPDIR)/Application.mk
