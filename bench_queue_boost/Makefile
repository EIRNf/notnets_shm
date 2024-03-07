THISDIR := $(realpath .)

EXECUTABLE = bench_boost

UNAME    := $(shell uname)

CPPFLAGS =

INCLUDES = \
 	-I /opt/homebrew/Cellar/boost/1.84.0_1/include \


LDFLAGS = \


ifeq ($(UNAME), Linux)
LDFLAGS += \

endif

include ../Makefile.exe
