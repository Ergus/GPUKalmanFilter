#ifndef FILTER_CL_H_
#define FILTER_CL_H_ 1

#define PROGRAM_FILE "Filter_OpenCL.cl"
#define KERNEL_FUNC "Kalman_Filter"
#define CL_USE_DEPRECATED_OPENCL_2_0_APIS

#ifndef LOCALSIZE
    #define LOCALSIZE 64
#endif

#define DEVICE_NUMBER 0

#define DEVICE_CPU 0
#define DEVICE_GPU 1
#define DEVICE_ACCELERATOR 2
#define DEVICE_PREFERENCE DEVICE_GPU


#include <CL/cl.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif
    
cl_device_id create_device();
cl_program build_program(cl_context ctx, cl_device_id dev, const char* filename);

float clFilter(int *evstart,
               int *trstart,
               float *ttrack,
               float *fullin,
               int *backward,
               float *sum2,
               float *fullout,
               size_t events,
               size_t tracks,
               size_t hits);

#ifdef __cplusplus
}
#endif
        

#endif
