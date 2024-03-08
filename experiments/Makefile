THISDIR := $(realpath .)

EXECUTABLE = experiments

UNAME    := $(shell uname)

CPPFLAGS = 

INCLUDES = \
	-I /opt/homebrew/Cellar/boost/1.84.0_1/include \
	-I $(THISDIR)/../libnotnets/include \

LDFLAGS = \
	-L $(THISDIR)/../libnotnets/bin \
	-L /opt/homebrew/Cellar/boost/1.84.0_1/lib \
	-lboost_filesystem-mt \
	-lboost_iostreams-mt \
	-lboost_coroutine-mt \
	-lboost_context-mt \
	-lboost_date_time-mt \
	-lboost_chrono-mt \
	-lboost_thread-mt \
	-lboost_system-mt \
	-lnotnets \

ifeq ($(UNAME), Linux)
LDFLAGS += \
	-Wl,-rpath,$(THISDIR)/../libnotnets/bin \
	-Wl,-rpath,/opt/homebrew/Cellar/boost/1.84.0_1/lib
endif


include ../Makefile.exe
