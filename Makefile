SHELL := /bin/bash

CXX = g++
CXXFLAGS = -O3 -std=c++11

CC=gcc
CFLAGS=-O3 -std=c99

file = good.x bad.x opencl.x opencl2.x
libs = Good.o Bad.o

all: $(file)

debug: CXXFLAGS = -O0 -DDEBUG -g -std=c++11 -Wall -DUNIX
debug: CFLAGS = -O0 -DDEBUG -g -std=c99 -Wall -DUNIX
debug: $(file)

#---OpenCL, this will go inside an if
PROC_TYPE = $(strip $(shell uname -m | grep 64))
OS = $(shell uname -s 2>/dev/null | tr [:lower:] [:upper:])
# Linux OS
LIBS_OCL=-lOpenCL
ifeq ($(PROC_TYPE),)
CFLAGS+=-m32
else
CFLAGS+=-m64

# Check for Linux-AMD
ifdef AMDAPPSDKROOT
INC_DIRS=. $(AMDAPPSDKROOT)/include
ifeq ($(PROC_TYPE),)
LIB_DIRS=$(AMDAPPSDKROOT)/lib/x86
else
LIB_DIRS=$(AMDAPPSDKROOT)/lib/x86_64
endif
else

# Check for Linux-Nvidia
ifdef CUDA
INC_DIRS=. $(CUDA)/OpenCL/common/inc
endif

endif
endif

Filter_OpenCL.o: Filter_OpenCL.c
	$(CC) $(CFLAGS) -o $@ -c $^ $(INC_DIRS:%=-I%) $(LIB_DIRS:%=-L%) $(LIBS_OCL) -DUOCL

Filter_OpenCL2.o: Filter_OpenCL.c
	$(CC) $(CFLAGS) -o $@ -c $^ $(INC_DIRS:%=-I%) $(LIB_DIRS:%=-L%) $(LIBS_OCL) -DUOCL2

#------END OpenCL------------------

good.x: main.cc $(libs)
	$(CXX) $(CXXFLAGS) $^ -o $@ -DGOOD

bad.x: main.cc $(libs)
	$(CXX) $(CXXFLAGS) $^ -o $@ -DBAD

opencl.x: main.cc Good.cpp Filter_OpenCL.o
	$(CXX) $(CXXFLAGS) $^ -o $@ -DGOOD -DUOCL $(INC_DIRS:%=-I%) $(LIB_DIRS:%=-L%) $(LIBS_OCL)

opencl2.x: main.cc Good.cpp Filter_OpenCL2.o
	$(CXX) $(CXXFLAGS) $^ -o $@ -DGOOD -DUOCL2 $(INC_DIRS:%=-I%) $(LIB_DIRS:%=-L%) $(LIBS_OCL)

%.o: %.cpp Filter.o
	$(CXX) $(CXXFLAGS) -c $^ -o $@

.PHONY: clean
clean:
	rm -rf *.x *.o *.txt *.a

.PHONY: test
test: $(file)
	for a in $(file); do ./$$a test.dat; done

.PHONY: check
check: $(file)
	rm -rf *.txt
	for a in $(file); do ./$$a test.dat; done
	diff Opencl_states2.txt Bad_states.txt
	diff Opencl_states.txt Bad_states.txt
	diff Good_states.txt Bad_states.txt

.PHONY: hard_check
hard_check: $(file)
	rm -rf *.txt
	for a in $(file); do ./$$a in.dat; done
	diff Opencl_states2.txt Bad_states.txt
	diff Opencl_states.txt Bad_states.txt
	diff Good_states.txt Bad_states.txt
