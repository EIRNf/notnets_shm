THISDIR := $(realpath .)

EXECUTABLE = experiments

UNAME    := $(shell uname)

CPPFLAGS = 

INCLUDES = \
	-I  /usr/include/boost \
	-I $(THISDIR)/../libnotnets/include \
	-I $(THISDIR)/../bench_rpc/include 

LDFLAGS = \
	-L $(THISDIR)/../libnotnets/bin \
	-L /usr/lib/x86_64-linux-gnu \
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
	-Wl,-rpath,/usr/lib/x86_64-linux-gnu
endif


include ../Makefile.exe
