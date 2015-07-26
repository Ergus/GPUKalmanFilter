#ifndef FILTER_CL_H_
#define FILTER_CL_H_ 1

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
               bool *backward,
               float *sum2,
               float *fullout,
               size_t events,
               size_t tracks,
               size_t hits);

#ifdef __cplusplus
}
#endif
        

#endif
