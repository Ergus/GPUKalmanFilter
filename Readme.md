# README #

This program implements a Kalman Filter using different methods and platforms. There are created many executable files:

 1. Bad.x -> Arrays of structs
 2. Good.x -> Structs of arrays (basically only arrays for now)
 3. Bad_Good -> A mixture of both above that measures also the move time from one distribution to the other.
 4. OpenCL[1,2] -> Two implementations of the filter using OpenCL
 5. Cuda[1,2] -> Two implementations of the filter using Cuda

The program default compilation expects that OpenCL will be installed in the system but the 3 first programs can be compiled without it and tested.

## Usage ##

There are some commands implemented in the Makefile to make the live easy:

 * make: compiles all the programs in the list above (Cuda ones only if there is nvcc compiler in the path)
 * make debug: same than before, but using some compilation flags and macros for debugging. The times are very different because there are *printf* called from the Kernels. This command needs a *make clean* before be called.
 * make test: run a simple test that prints the output on screen for all the compiled programs (*.x)
 * make check: makes a clean and run a test. Then make diff to the outputs comparing with the original output (Bad.x).
 * make clean: ...
 
### Other scripts ###

 * run.sh: a script that runs the benchmark from 1->300 events. This script don't compiles. so it expects the executable files already to be compiled.
 * gnuplot.plt: Creates the graphs for basic testing. (this are not the final graphs).

**Important:** The versions of the GPU codes using pointers instead of a loop for copy and a memcpy for the cuda version are in the branches pointer and memcpy respectively. 

