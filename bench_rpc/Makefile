THISDIR := $(realpath .)

EXECUTABLE = bench_rpc

UNAME    := $(shell uname)

CPPFLAGS =

INCLUDES = \
 	-I $(THISDIR)/../libnotnets/include \

LDFLAGS = \
	-L $(THISDIR)/../libnotnets/bin \
	-lnotnets \


ifeq ($(UNAME), Linux)
LDFLAGS += \
	-Wl,-rpath,$(THISDIR)/../libnotnets/bin \

endif

include ../Makefile.exe
