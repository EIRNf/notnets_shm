THISDIR := $(realpath .)

EXECUTABLE = run_all_tests

UNAME    := $(shell uname)

CPPFLAGS =

INCLUDES = \
 	-I $(THISDIR)/../libnotnets/include \
	    -I /opt/homebrew/Cellar/googletest/1.14.0/include \
	    -I /usr/src/gtest/include \
        -I /usr/local/include/gtest/ \

LDFLAGS = \
	-L $(THISDIR)/../libnotnets/bin \
	-L /opt/homebrew/Cellar/googletest/1.14.0/lib/ \
	-L /usr/local/lib \
	-lnotnets \
        -lpthread -lgtest -lgtest_main \


ifeq ($(UNAME), Linux)
LDFLAGS += \
	-Wl,-rpath,$(THISDIR)/../libnotnets/bin \
	-lnotnets \
    -lpthread -lm \
	-lgtest  -lgtest_main\
	

endif

include ../Makefile.exe
