THISDIR := $(realpath .)

EXECUTABLE = run_all_tests

UNAME    := $(shell uname)

CPPFLAGS =

INCLUDES = \
 	-I $(THISDIR)/../libnotnets/include \
        -I /usr/local/include/gtest/ \

LDFLAGS = \
	-L $(THISDIR)/../libnotnets/bin \
	-L /usr/local/lib \
	-lnotnets \
        -lgtest \


ifeq ($(UNAME), Linux)
LDFLAGS += \
	-Wl,-rpath,$(THISDIR)/../libnotnets/bin \

endif

include ../Makefile.exe
