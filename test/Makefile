THISDIR := $(realpath .)

EXECUTABLE = run_all_tests

UNAME    := $(shell uname)

CPPFLAGS =

INCLUDES = \
 	-I $(THISDIR)/../libnotnets/include \
        -I /opt/homebrew/Cellar/googletest/1.14.0/include \
		 -I /opt/homebrew/Cellar/boost/1.84.0_1/include \

LDFLAGS = \
	-L $(THISDIR)/../libnotnets/bin \
	-L /opt/homebrew/Cellar/googletest/1.14.0/lib/ \
	-lnotnets \
        -lgtest \


ifeq ($(UNAME), Linux)
LDFLAGS += \
	-Wl,-rpath,$(THISDIR)/../libnotnets/bin \

endif

include ../Makefile.exe
