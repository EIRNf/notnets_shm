THISDIR := $(realpath .)

EXECUTABLE = experiments

UNAME    := $(shell uname)

CPPFLAGS = 

INCLUDES = \
	-I /opt/homebrew/Cellar/boost/1.84.0_1/include \
	-I /usr/local/include \
	-I $(THISDIR)/../libnotnets/include \

LDFLAGS = \
	-L $(THISDIR)/../libnotnets/bin \
	-L /opt/homebrew/Cellar/boost/1.84.0_1/lib \
	-L /usr/local/lib \
	-lboost_filesystem \
	-lboost_iostreams \
	-lboost_coroutine \
	-lboost_context \
	-lboost_date_time \
	-lboost_chrono \
	-lboost_thread \
	-lboost_system \
	-lnotnets \

ifeq ($(UNAME), Linux)
LDFLAGS += \
	-Wl,-rpath,$(THISDIR)/../libnotnets/bin \
	-Wl,-rpath,/opt/homebrew/Cellar/boost/1.84.0_1/lib
endif


include ../Makefile.exe
