THISDIR := $(realpath .)

EXECUTABLE = simulation

UNAME    := $(shell uname)

CPPFLAGS =

INCLUDES = \
 	-I $(THISDIR)/../libnotnets/include \
        -I /usr/src/googletest/googletest/include/ \

LDFLAGS = \
	-L $(THISDIR)/../libnotnets/bin \
	-L /usr/src/googletest/googletest/lib/ \
	-lnotnets \
        -lgtest \


ifeq ($(UNAME), Linux)
LDFLAGS += \
	-Wl,-rpath,$(THISDIR)/../libnotnets/bin \

endif

include ../Makefile.exe
