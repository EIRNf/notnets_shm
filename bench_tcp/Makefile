THISDIR := $(realpath .)

EXECUTABLE = bench_tcp

UNAME    := $(shell uname)

CPPFLAGS =

INCLUDES = \
 	-I $(THISDIR)/../libnotnets/include \
	-I /opt/homebrew/Cellar/boost/1.84.0_1/include \

LDFLAGS = \
	-L $(THISDIR)/../libnotnets/bin \
	-lnotnets \


ifeq ($(UNAME), Linux)
LDFLAGS += \
	-Wl,-rpath,$(THISDIR)/../libnotnets/bin \

endif

include ../Makefile.exe
