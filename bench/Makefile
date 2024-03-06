THISDIR := $(realpath .)

UNAME  := $(shell uname)

CPPFLAGS = 

INCLUDES = \
 	-I $(THISDIR)/../libnotnets/include \
        -I /opt/homebrew/Cellar/googletest/1.14.0/include \

LDFLAGS = \
	-L $(THISDIR)/../libnotnets/bin \
	-L /opt/homebrew/Cellar/googletest/1.14.0/lib/ \
	-lnotnets \
        -lgtest \

ifeq ($(UNAME), Linux)
LDFLAGS += \
	-Wl,-rpath,$(THISDIR)/../libnotnets/bin \

endif

include ../Makefile.common

.PHONY: bench 

bench:
	(./runner.sh "$(COMPILE) $(LDFLAGS)")

