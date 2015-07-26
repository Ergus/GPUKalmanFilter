#define PROGRAM_FILE "Filter_OpenCL.cl"
#define KERNEL_FUNC "Kalman_Filter"

#ifndef LOCALSIZE
    #define LOCALSIZE 64
#endif

#include <Filter_OpenCL.h>

// Find a GPU or CPU associated with the first available platform
cl_device_id create_device() {
    cl_platform_id platform;
    cl_device_id dev;
    int err;

    // Identify a platform
    err = clGetPlatformIDs(1, &platform, NULL);
    if(err < 0) {
        perror("Couldn't identify a platform");
        exit(1);
        } 

    // Access a device
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &dev, NULL);
    if(err == CL_DEVICE_NOT_FOUND) 
        err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 1, &dev, NULL);
    if(err < 0) {
        perror("Couldn't access any devices");
        exit(1);   
        }
    return dev;
    }

// Create program from a file and compile it
cl_program build_program(cl_context ctx, cl_device_id dev, const char* filename) {

    cl_program program;
    FILE *program_handle;
    char *program_buffer, *program_log;
    size_t program_size, log_size;
    int err;


    // Read program file and place content into buffer
    program_handle = fopen(filename, "r");
    if(program_handle == NULL) {
        perror("Couldn't find the program file");
        exit(1);
        }

    fseek(program_handle, 0, SEEK_END);
    program_size = ftell(program_handle);
    rewind(program_handle);
    program_buffer = (char*)malloc(program_size + 1);
    program_buffer[program_size] = '\0';
    fread(program_buffer, sizeof(char), program_size, program_handle);
    fclose(program_handle);

    // Create program from file
    program = clCreateProgramWithSource(ctx, 1, 
                                        (const char**)&program_buffer, &program_size, &err);
    if(err < 0) {
        perror("Couldn't create the program");
        exit(1);
        }
    free(program_buffer);

    // Build program
    err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
    if(err < 0) {

        // Find size of log and print to std output
        clGetProgramBuildInfo(program, dev, CL_PROGRAM_BUILD_LOG, 
                              0, NULL, &log_size);
        program_log = (char*) malloc(log_size + 1);
        program_log[log_size] = '\0';
        clGetProgramBuildInfo(program, dev, CL_PROGRAM_BUILD_LOG, 
                              log_size + 1, program_log, NULL);
        printf("%s\n", program_log);
        free(program_log);
        exit(1);
        }

    return program;
    }

float clFilter(int *evstart,
               int *trstart,
               float *ttrack,
               float *fullin,
               bool *backward,
               float *sum2,
               float *fullout,
               size_t events,
               size_t tracks,
               size_t hits){
    
    // OpenCL structures
    cl_device_id device;
    cl_context context;
    cl_program program;
    cl_kernel kernel;
    cl_command_queue queue;
    cl_int err;
    cl_event evt_kernel;
    
    const size_t local_size = LOCALSIZE,
                global_size = tracks;
    cl_int num_groups = (global_size+LOCALSIZE-1)/LOCALSIZE;

    // Data pointers
    cl_mem dev_evstart,    //Array int start events [nbevents+1]
           dev_trstart,    //Array int start tracks [nbtracks+1]
           dev_ttrack,     //Array for tx,ty/track  [2*tracks]
           dev_fullin,     //Array float data hits  [5*nbhits]
           dev_backward,   //Array bool backward    [nbtracks]
           dev_sum2,       //array float parameter  [nbtracks]
           dev_fullout;    //Array float results.   [11*nbtracks]

    // Create device and context
    device = create_device();
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    if(err < 0) {
        perror("Couldn't create a context");
        exit(1);   
        }

    // Build program
    program = build_program(context, device, PROGRAM_FILE);    

    // Allocate arrays to the host
    dev_evstart = clCreateBuffer(context, CL_MEM_READ_ONLY |
                                 CL_MEM_COPY_HOST_PTR, (events+1)*sizeof(int), evstart, &err);
    dev_trstart = clCreateBuffer(context, CL_MEM_READ_ONLY |
                                 CL_MEM_COPY_HOST_PTR, (tracks+1)*sizeof(int), trstart, &err);

    dev_ttrack = clCreateBuffer(context, CL_MEM_READ_ONLY |
                                 CL_MEM_COPY_HOST_PTR, 2*tracks*sizeof(float), ttrack, &err);
    
    dev_fullin = clCreateBuffer(context, CL_MEM_READ_ONLY |
                                 CL_MEM_COPY_HOST_PTR, 5*hits*sizeof(float), fullin, &err);
    dev_backward = clCreateBuffer(context, CL_MEM_READ_ONLY |
                                 CL_MEM_COPY_HOST_PTR, tracks*sizeof(bool), backward, &err);
    dev_sum2 = clCreateBuffer(context, CL_MEM_READ_ONLY |
                                 CL_MEM_COPY_HOST_PTR, tracks*sizeof(float), sum2, &err);
    dev_fullout = clCreateBuffer(context, CL_MEM_WRITE_ONLY,
                                 11*tracks*sizeof(float), NULL, &err);

    if(err < 0) {
        perror("Couldn't create a buffer");
        exit(1);   
        };
    
    // Create a command queue
    queue = clCreateCommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, &err);
    if(err < 0) {
        perror("Couldn't create a command queue");
        exit(1);   
        };
    
    // Create a kernel
    kernel = clCreateKernel(program, KERNEL_FUNC, &err);
    if(err < 0) {
        perror("Couldn't create a kernel");
        exit(1);
        };    

    // Create kernel arguments
    err  = clSetKernelArg(kernel, 0, sizeof(cl_mem), &dev_ttrack);
    err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &dev_trstart);
    err |= clSetKernelArg(kernel, 2, sizeof(cl_mem), &dev_fullin);
    err |= clSetKernelArg(kernel, 3, sizeof(cl_mem), &dev_backward);
    err |= clSetKernelArg(kernel, 4, sizeof(cl_mem), &dev_sum2);
    err |= clSetKernelArg(kernel, 5, sizeof(cl_mem), &dev_fullout);
    err |= clSetKernelArg(kernel, 6, sizeof(unsigned int), &tracks);
    err |= clSetKernelArg(kernel, 7, sizeof(unsigned int), &hits);        
    if(err < 0) {
        perror("Couldn't create a kernel argument");
        exit(1);
        }

        
    // Enqueue kernel
    err = clEnqueueNDRangeKernel(queue,
                                 kernel,
                                 1, NULL,
                                 &global_size, 
                                 &local_size,
                                 0, NULL,
                                 &evt_kernel); 
    if(err < 0) {
        perror("Couldn't enqueue the kernel");
        exit(1);
        }    

    // Read the kernel's output
    err = clEnqueueReadBuffer(queue,
                              dev_fullout,
                              CL_TRUE,
                              0,11*tracks*sizeof(float),
                              fullout,
                              1,
                              &evt_kernel,
                              NULL);
    if(err < 0) {
        perror("Couldn't read the buffer");
        exit(1);
        }

    clReleaseKernel(kernel);

    clReleaseMemObject(dev_evstart);    
    clReleaseMemObject(dev_trstart);
    clReleaseMemObject(dev_fullin);
    clReleaseMemObject(dev_backward);
    clReleaseMemObject(dev_sum2);
    clReleaseMemObject(dev_fullout);
    
    clReleaseCommandQueue(queue);
    clReleaseProgram(program);
    clReleaseContext(context);    
    return 0;
    }
