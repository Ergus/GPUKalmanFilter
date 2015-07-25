#ifndef FILTER_CL_H_
#define FILTER_CL_H_ 1

#include <CL/cl.h>

cl_device_id create_device();
cl_program build_program(cl_context ctx, cl_device_id dev, const char* filename);

#endif
