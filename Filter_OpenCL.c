
#include <Filter_OpenCL.h>

void clChoosePlatform(cl_device_id** devices, cl_platform_id* platform) {
    // Choose the first available platform
    cl_uint numPlatforms;
    clGetPlatformIDs(0, NULL, &numPlatforms);
    if(numPlatforms > 0)
    {
        cl_platform_id* platforms = (cl_platform_id*) malloc(numPlatforms * sizeof(cl_platform_id));
        clGetPlatformIDs(numPlatforms, platforms, NULL);
        *platform = platforms[0];
        free(platforms);
        }

    // Choose a device from the platform according to DEVICE_PREFERENCE
    cl_uint numCpus = 0;
    cl_uint numGpus = 0;
    cl_uint numAccelerators = 0;
    clGetDeviceIDs(*platform, CL_DEVICE_TYPE_CPU, 0, NULL, &numCpus);
    clGetDeviceIDs(*platform, CL_DEVICE_TYPE_GPU, 0, NULL, &numGpus);
    clGetDeviceIDs(*platform, CL_DEVICE_TYPE_ACCELERATOR, 0, NULL, &numAccelerators);
    *devices = (cl_device_id*) malloc(numAccelerators * sizeof(cl_device_id));

    fprintf(stderr,"\nDevices available: \n");
    fprintf(stderr,"CPU: %u\nGPU: %u\nAccelerators: %u\n",numCpus,numGpus,numAccelerators);

    if (DEVICE_PREFERENCE == DEVICE_CPU && numCpus > 0) {
        fprintf(stderr,"Choosing CPU\n");
        clGetDeviceIDs(*platform, CL_DEVICE_TYPE_CPU, numCpus, *devices, NULL);
        }
    else if (DEVICE_PREFERENCE == DEVICE_GPU && numGpus > 0) {
        fprintf(stderr,"Choosing GPU\n");
        clGetDeviceIDs(*platform, CL_DEVICE_TYPE_GPU, numGpus, *devices, NULL);
        }
    else if (DEVICE_PREFERENCE == DEVICE_ACCELERATOR && numAccelerators > 0) {
        fprintf(stderr,"Choosing accelerator\n");
        clGetDeviceIDs(*platform, CL_DEVICE_TYPE_ACCELERATOR, numAccelerators, *devices, NULL);
        }
    else {
        // We couldn't match the preference.
        // Let's try the first device that appears available.
        cl_uint numDevices = 0;
        clGetDeviceIDs(*platform, CL_DEVICE_TYPE_ALL, 0, NULL, &numDevices);
        if (numDevices > 0) {
            fprintf(stderr,"Preference device couldn't be met\n");
            fprintf(stderr,"Choosing an available OpenCL capable device\n");
            clGetDeviceIDs(*platform, CL_DEVICE_TYPE_ALL, numDevices, *devices, NULL);
            }
        else {
            fprintf(stderr,"No OpenCL capable device detected\n");
            fprintf(stderr,"Check the drivers, OpenCL runtime or ICDs are available\n");
            exit(-1);
            }
        }
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
               int *backward,
               float *sum2,
               float *fullout,
               size_t events,
               size_t tracks,
               size_t hits){
    
    // OpenCL structures
    cl_int err;

    cl_device_id* devices;
    cl_platform_id platform = NULL;
    clChoosePlatform(&devices, &platform);

    cl_context context = clCreateContext(NULL, 1, devices, NULL, NULL, &err);
    if(err < 0) {
        perror("Couldn't create a context");
        exit(1);   
        }

    // Create a command queue
    cl_command_queue queue = clCreateCommandQueue(context, devices[DEVICE_NUMBER], CL_QUEUE_PROFILING_ENABLE, &err);
    if(err < 0) {
        perror("Couldn't create a command queue");
        exit(1);   
        };
    
    size_t local_size[2] = {LOCALSIZE, 2},
        blocks=(tracks+LOCALSIZE-1)/LOCALSIZE,
        global_size[2] = {blocks*LOCALSIZE, 2};

    // Build program
    cl_program program = build_program(context, devices[DEVICE_NUMBER], PROGRAM_FILE);

    // Allocate arrays to the host
    //Array int start events [nbevents+1]
    cl_mem dev_evstart = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, (events+1)*sizeof(int), evstart, &err);
    //Array int start tracks [nbtracks+1]
    cl_mem dev_trstart = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, (tracks+1)*sizeof(int), trstart, &err);
    //Array for tx,ty/track  [2*tracks]
    cl_mem dev_ttrack = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, 2*tracks*sizeof(float), ttrack, &err);
    //Array float data hits  [5*nbhits]
    cl_mem dev_fullin = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, 5*hits*sizeof(float), fullin, &err);
    //Array bool backward    [nbtracks]
    cl_mem dev_backward = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, tracks*sizeof(int), backward, &err);
    //array float parameter  [nbtracks]
    cl_mem dev_sum2 = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, tracks*sizeof(float), sum2, &err);
    //Array float results.   [11*nbtracks]
    cl_mem dev_fullout = clCreateBuffer(context, CL_MEM_READ_WRITE, 11*tracks*sizeof(float), NULL, &err);

    if(err < 0) {
        perror("Couldn't create a buffer");
        exit(1);   
        };
    
    // Create a kernel
    cl_kernel kernel = clCreateKernel(program, KERNEL_FUNC, &err);
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

    cl_event kernelEvent;
    // Enqueue kernel
    err = clEnqueueNDRangeKernel(queue,
                                 kernel,
                                 dimension, NULL,
                                 global_size, 
                                 local_size,
                                 0, NULL,
                                 &kernelEvent);
    
    clWaitForEvents(1 , &kernelEvent);


    // Read the kernel's output
    err = clEnqueueReadBuffer(queue,
                              dev_fullout,
                              CL_TRUE,
                              0, 11*tracks*sizeof(float),
                              fullout,
                              0,
                              NULL,
                              NULL);
    if(err < 0) {
        perror("Couldn't read the buffer");
        exit(1);
        }

    clReleaseKernel(kernel);

    clReleaseMemObject(dev_ttrack);
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
