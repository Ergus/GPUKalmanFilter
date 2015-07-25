CXX = g++
CXXFLAGS = -O3 -std=c++11

#---OpenCL, this will go inside an if
CC=gcc
CFLAGS=-std=c99 -Wall -DUNIX
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
	$(CC) $(CFLAGS) -o $@ -c $^ $(INC_DIRS:%=-I%) $(LIB_DIRS:%=-L%) $(LIBS_OCL)

#------END OpenCL------------------

file = good.x bad.x opencl.x
libs = Good.o Bad.o

all: $(file)

debug: CXXFLAGS = -O0 -DDEBUG -g -std=c++11
debug: $(file)

good.x: main.cc $(libs)
	$(CXX) $(CXXFLAGS) $^ -o $@ -DGOOD

bad.x: main.cc $(libs)
	$(CXX) $(CXXFLAGS) $^ -o $@ -DBAD

opencl.x: main.cc $(libs)
	$(CXX) $(CXXFLAGS) $^ -o $@ -DGOOD -DUOCL $(LIB_DIRS:%=-L%) $(LIBS_OCL)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $^ -o $@

.PHONY: clean
clean:
	rm -rf *.x *.o *.txt

.PHONY: test
test: $(file)
	for a in $(file); do ./$$a test.dat; done
