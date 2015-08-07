SHELL := /bin/bash

# First Compilers
# C++ compiler
CXX = g++
CXXFLAGS = -O3 -std=c++11

# C compiler
CC=gcc
CFLAGS=-O3 -std=c99

file = good.x bad.x bad_good.x opencl.x opencl2.x
libs = Good.o Bad.o
SRCS = main.cc Filter_Cuda.cu Filter_OpenCL.c 

# check of cuda compiler
NVCC_RESULT := $(shell which nvcc 2> /dev/null)
NVCC_TEST := $(notdir $(NVCC_RESULT))
ifeq ($(NVCC_TEST),nvcc)
CUDACC=nvcc
CUFLAGS=-O3 -std=c++11
file += cuda.x cuda2.x
endif

all: $(file)

debug: CXXFLAGS = -O0 -DDEBUG -g -std=c++11 -Wall -DUNIX
debug: CFLAGS = -O0 -DDEBUG -g -std=c99 -Wall -DUNIX
debug: CUFLAGS = -O0 -DDEBUG -g -std=c++11 -DUNIX
debug: $(file)

#---Cuda, this will go inside an if
Filter_Cuda.o: Filter_Cuda.cu
	$(CUDACC) $(CUFLAGS) -o $@ -c $^ -DUCUDA=1

Filter_Cuda2.o: Filter_Cuda.cu
	$(CUDACC) $(CUFLAGS) -o $@ -c $^ -DUCUDA=2

#---OpenCL, this will go inside an if
PROC_TYPE = $(strip $(shell uname -m | grep 64))
OS = $(shell uname -s 2>/dev/null | tr [:lower:] [:upper:])
# Linux OS
LIBS_OCL=-lOpenCL
ifeq ($(PROC_TYPE),)
  CFLAGS+=-m32
else
  CFLAGS+=-m64
  ifdef AMDAPPSDKROOT # Check for Linux-AMD
    INC_DIRS=. $(AMDAPPSDKROOT)/include
    ifeq ($(PROC_TYPE),)
      LIB_DIRS=$(AMDAPPSDKROOT)/lib/x86
    else
      LIB_DIRS=$(AMDAPPSDKROOT)/lib/x86_64
    endif
  else
    ifdef CUDA # Check for Linux-Nvidia
      INC_DIRS=. $(CUDA)/OpenCL/common/inc
    endif
  endif
endif

Filter_OpenCL.o: Filter_OpenCL.c
	$(CC) $(CFLAGS) -o $@ -c $< $(INC_DIRS:%=-I%) $(LIB_DIRS:%=-L%) $(LIBS_OCL) -DUOCL=1

Filter_OpenCL2.o: Filter_OpenCL.c
	$(CC) $(CFLAGS) -o $@ -c $< $(INC_DIRS:%=-I%) $(LIB_DIRS:%=-L%) $(LIBS_OCL) -DUOCL=2

#------END OpenCL------------------
#-------Headers--------------------
depend: .depend

.depend: $(SRCS)
	echo "Remaking deppend files"
	rm -f ./.depend
	$(CXX) $(CXXFLAGS) -MM main.cc Good.cpp Bad.cpp  >  ./.depend;
	$(CC) $(CFLAGS) -MM Filter_OpenCL.c >>  ./.depend;
	$(CXX) $(CUFLAGS) -MM Filter_Cuda.cu >> ./.depend;

include .depend

#------ End Headers---------------
bad_good.x: main.cc Good.cpp Bad.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@ -DBAD_GOOD

good.x: main.cc Good.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@ -DGOOD

bad.x: main.cc Bad.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@ -DBAD

opencl.x: main.cc Good.cpp Filter_OpenCL.o
	$(CXX) $(CXXFLAGS) $^ -o $@ -DGOOD -DUOCL=1 $(INC_DIRS:%=-I%) $(LIB_DIRS:%=-L%) $(LIBS_OCL)

opencl2.x: main.cc Good.cpp Filter_OpenCL2.o
	$(CXX) $(CXXFLAGS) $^ -o $@ -DGOOD -DUOCL=2 $(INC_DIRS:%=-I%) $(LIB_DIRS:%=-L%) $(LIBS_OCL)

cuda.x: main.cc Good.cpp Filter_Cuda.o
	$(CUDACC) $(CUFLAGS) $^ -o $@ -DGOOD -DUCUDA=1

cuda2.x: main.cc Good.cpp Filter_Cuda2.o
	$(CUDACC) $(CUFLAGS) $^ -o $@ -DGOOD -DUCUDA=2

%.o: %.cpp Filter.o
	$(CXX) $(CXXFLAGS) -c $^ -o $@

.PHONY: clean
clean:
	rm -rf *.x *.o *.txt

.PHONY: test
test: $(file)
	for a in $(file); do ./$$a test.dat; done

.PHONY: check
check: $(file)
	rm -rf *.txt
	for a in $(file); do ./$$a test.dat; done
	for a in *.txt; do \
		diff $$a Bad_states.txt; \
		echo "Made diff to: " $$a; \
		read -p "Press [Enter] key to diff next file..."; \
		done

