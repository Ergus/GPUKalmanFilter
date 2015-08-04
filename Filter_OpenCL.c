
#include "Filter_OpenCL.h"

void clChoosePlatform(cl_device_id** devices, cl_platform_id* platform) {
    // Choose the first available platform
    cl_uint numPlatforms;
    clGetPlatformIDs(0, NULL, &numPlatforms);
    if(numPlatforms > 0){
        cl_platform_id* platforms = (cl_platform_id*) malloc(numPlatforms * sizeof(cl_platform_id));
        clGetPlatformIDs(numPlatforms, platforms, NULL);
        #ifdef DEBUG
        char vendor[1024], name[1024];
        fprintf(stderr,"Number of available platforms %d\n",numPlatforms);
        for(int i=0;i<numPlatforms;i++){
            clGetPlatformInfo(platforms[i], CL_PLATFORM_VENDOR, sizeof(vendor), vendor, NULL);
            clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME, sizeof(name), name, NULL);
            fprintf(stderr," %c Vendor: %s, Name: %s\n",(PLATFORM_NUMBER==i?'*':' '),vendor,name);
            }
        #endif
        *platform = platforms[PLATFORM_NUMBER];
        free(platforms);
        }
    else{
        fprintf(stderr,"No platforms available error\n");
        exit(-1);
        }

    // Choose a device from the platform according to DEVICE_PREFERENCE
    cl_uint nbdevices[3]={};
    const cl_device_type types[]={CL_DEVICE_TYPE_CPU,CL_DEVICE_TYPE_GPU,CL_DEVICE_TYPE_ACCELERATOR};
    
    #ifdef DEBUG    
    const char devnames[3][10]={"CPU","GPU","Acelerator"};
    fprintf(stderr,"Devices available: \n");
    #endif

    for(int i=0;i<3;i++){
        clGetDeviceIDs(*platform, types[i], 0, NULL, &nbdevices[i]);
        #ifdef DEBUG 
        fprintf(stderr," %s: %u\n",devnames[i],nbdevices[i]);
        #endif        
        }

    if (nbdevices[DEVICE_PREFERENCE]>0){
        *devices = (cl_device_id*) malloc(nbdevices[DEVICE_PREFERENCE] * sizeof(cl_device_id));
        clGetDeviceIDs(*platform, types[DEVICE_PREFERENCE],
                       nbdevices[DEVICE_PREFERENCE], *devices,
                       &nbdevices[DEVICE_PREFERENCE]);
        
        #ifdef DEBUG
        fprintf(stderr,"Choosing %s\n",devnames[DEVICE_PREFERENCE]);
        for(int i=0;i<nbdevices[DEVICE_PREFERENCE];i++){
            char name[1024],vendor[1024];
            clCheck(clGetDeviceInfo((*devices)[i], CL_DEVICE_NAME, sizeof(name), name, NULL));
            clCheck(clGetDeviceInfo((*devices)[i], CL_DEVICE_VENDOR, sizeof(vendor), vendor, NULL));
            printf(" %c %d Device Name = %s, vendor: %s\n", (i==DEVICE_NUMBER?'*':' '),i,name, vendor);
            }        
        #endif
        }
    else {
        // We couldn't match the preference.
        // Let's try the first device that appears available.
        cl_uint numDevices = 0;
        clGetDeviceIDs(*platform, CL_DEVICE_TYPE_ALL, 0, NULL, &numDevices);
        if (numDevices > 0) {
            fprintf(stderr,"Preference device couldn't be met\n");
            fprintf(stderr,"Choosing an available OpenCL capable device\n");
            fprintf(stderr,"There are other %d OpenCL devices available\n",numDevices);
            *devices = (cl_device_id*) malloc(numDevices * sizeof(cl_device_id));
            clGetDeviceIDs(*platform, CL_DEVICE_TYPE_ALL, numDevices, *devices, NULL);
            for(int i=0;i<numDevices;i++){
                char buffer[1024];
                clCheck(clGetDeviceInfo((*devices)[i], CL_DEVICE_NAME, sizeof(buffer), buffer, NULL));
                printf("  Device Name = %s\n", buffer);
                }        
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
    char *program_buffer, *program_log;
    size_t program_size, log_size;
    cl_int err;

    // Read program file and place content into buffer
    FILE *program_handle = fopen(filename, "r");
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
    //err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
    char buildOptions[1024];
    sprintf(buildOptions,BUILD_OPTIONS,UOCL);
    #ifdef DEBUG
    printf("Kernel build options: %s\n",buildOptions);
    #endif
    err = clBuildProgram(program, 1, &dev, buildOptions, NULL, NULL);
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
    cl_ulong gpu_start, gpu_end;
    double cpu_start, cpu_end;

    cl_device_id* devices;
    cl_platform_id platform = NULL;
    clChoosePlatform(&devices, &platform);

    cl_context context = clCreateContext(NULL, 1, devices, NULL, NULL, &err);
    checkClError(err);
    if(err < 0) {
        perror("Couldn't create a context");
        exit(1);   
        }

    // Create a command queue
    cl_command_queue queue = clCreateCommandQueue(context, devices[DEVICE_NUMBER], CL_QUEUE_PROFILING_ENABLE, &err);
    checkClError(err);
    if(err < 0) {
        perror("Couldn't create a command queue");
        exit(1);   
        };
    
    size_t local_size[2] = {LOCALSIZE, 2},
        blocks=(tracks+LOCALSIZE-1)/LOCALSIZE,
        global_size[2] = {blocks*LOCALSIZE, 2};

    // Build program
    cl_program program = build_program(context, devices[DEVICE_NUMBER], PROGRAM_FILE);

    // Allocate arrays to the device
    //Array int start events [nbevents+1]
    cl_mem dev_evstart = clCreateBuffer(context, CL_MEM_READ_ONLY, (events+1)*sizeof(int), NULL, &err); checkClError(err);
    //Array int start tracks [nbtracks+1]
    cl_mem dev_trstart = clCreateBuffer(context, CL_MEM_READ_ONLY, (tracks+1)*sizeof(int), NULL, &err); checkClError(err);
    //Array for tx,ty/track  [2*tracks]
    cl_mem dev_ttrack = clCreateBuffer(context, CL_MEM_READ_ONLY, 2*tracks*sizeof(float), NULL, &err); checkClError(err);
    //Array float data hits  [5*nbhits]
    cl_mem dev_fullin = clCreateBuffer(context, CL_MEM_READ_WRITE, 5*hits*sizeof(float), NULL, &err); checkClError(err);
    //Array bool backward    [nbtracks]
    cl_mem dev_backward = clCreateBuffer(context, CL_MEM_READ_ONLY, tracks*sizeof(int), NULL, &err); checkClError(err);
    //array float parameter  [nbtracks]
    cl_mem dev_sum2 = clCreateBuffer(context, CL_MEM_READ_ONLY, tracks*sizeof(float), NULL, &err); checkClError(err);
    //Array float results.   [11*nbtracks]
    cl_mem dev_fullout = clCreateBuffer(context, CL_MEM_READ_WRITE, 11*tracks*sizeof(float), NULL, &err); checkClError(err);
    if(err < 0) {
        perror("Couldn't create a buffer");
        exit(1);   
        };
    //---------------------------
        // Allocate arrays to the device
    clCheck(clEnqueueWriteBuffer(queue, dev_evstart, CL_TRUE, 0, (events+1)*sizeof(int), evstart, 0, NULL, NULL));
    clCheck(clEnqueueWriteBuffer(queue, dev_trstart, CL_TRUE, 0, (tracks+1)*sizeof(int), trstart, 0, NULL, NULL));
    clCheck(clEnqueueWriteBuffer(queue, dev_ttrack,  CL_TRUE, 0, 2*tracks*sizeof(float), ttrack,  0, NULL, NULL));
    clCheck(clEnqueueWriteBuffer(queue, dev_fullin, CL_TRUE, 0, 5*hits*sizeof(float), fullin, 0, NULL, NULL));
    clCheck(clEnqueueWriteBuffer(queue, dev_backward, CL_TRUE, 0, tracks*sizeof(int), backward, 0, NULL, NULL));
    clCheck(clEnqueueWriteBuffer(queue, dev_sum2, CL_TRUE, 0, tracks*sizeof(float), sum2, 0, NULL, NULL));
    
    //----------------------------
    
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

    clCheck(clFinish(queue));    
    
    cl_event kernelEvent;
    // Enqueue kernel
    cpu_start=mtimes();
    clCheck(clEnqueueNDRangeKernel(queue,
                                 kernel,
                                 dimension, NULL,
                                 global_size, 
                                 local_size,
                                 0, NULL,
                                 &kernelEvent));
    
    clCheck(clWaitForEvents(1 , &kernelEvent));
    cpu_end=mtimes();    

    clGetEventProfilingInfo(kernelEvent, CL_PROFILING_COMMAND_START, sizeof(gpu_start), &gpu_start, NULL);
    clGetEventProfilingInfo(kernelEvent, CL_PROFILING_COMMAND_END, sizeof(gpu_end), &gpu_end, NULL);

    printf("Time GPU Kernel %s%d Events: %zu Tracks: %zu Hits: %zu = %lg s\n",
                        method,UOCL,    events,    tracks,    hits, (double)(gpu_end - gpu_start)*1E-9 );
    printf("Time CPU Kernel %s%d Events: %zu Tracks: %zu Hits: %zu = %lg s\n",
                        method,UOCL,    events,    tracks,    hits, (cpu_end-cpu_start) );

    // Read the kernel's output
    clCheck(clEnqueueReadBuffer(queue,
                              dev_fullout,
                              CL_TRUE,
                              0, 11*tracks*sizeof(float),
                              fullout,
                              0,
                              NULL,
                              NULL));


    free(devices);
    clReleaseEvent(kernelEvent);
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

//This is a debugging function
const char *getErrorString (cl_int error) {
    switch(error){
        // run-time and JIT compiler errors
    case 0: return "CL_SUCCESS";
    case -1: return "CL_DEVICE_NOT_FOUND";
    case -2: return "CL_DEVICE_NOT_AVAILABLE";
    case -3: return "CL_COMPILER_NOT_AVAILABLE";
    case -4: return "CL_MEM_OBJECT_ALLOCATION_FAILURE";
    case -5: return "CL_OUT_OF_RESOURCES";
    case -6: return "CL_OUT_OF_HOST_MEMORY";
    case -7: return "CL_PROFILING_INFO_NOT_AVAILABLE";
    case -8: return "CL_MEM_COPY_OVERLAP";
    case -9: return "CL_IMAGE_FORMAT_MISMATCH";
    case -10: return "CL_IMAGE_FORMAT_NOT_SUPPORTED";
    case -11: return "CL_BUILD_PROGRAM_FAILURE";
    case -12: return "CL_MAP_FAILURE";
    case -13: return "CL_MISALIGNED_SUB_BUFFER_OFFSET";
    case -14: return "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST";
    case -15: return "CL_COMPILE_PROGRAM_FAILURE";
    case -16: return "CL_LINKER_NOT_AVAILABLE";
    case -17: return "CL_LINK_PROGRAM_FAILURE";
    case -18: return "CL_DEVICE_PARTITION_FAILED";
    case -19: return "CL_KERNEL_ARG_INFO_NOT_AVAILABLE";

        // compile-time errors
    case -30: return "CL_INVALID_VALUE";
    case -31: return "CL_INVALID_DEVICE_TYPE";
    case -32: return "CL_INVALID_PLATFORM";
    case -33: return "CL_INVALID_DEVICE";
    case -34: return "CL_INVALID_CONTEXT";
    case -35: return "CL_INVALID_QUEUE_PROPERTIES";
    case -36: return "CL_INVALID_COMMAND_QUEUE";
    case -37: return "CL_INVALID_HOST_PTR";
    case -38: return "CL_INVALID_MEM_OBJECT";
    case -39: return "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";
    case -40: return "CL_INVALID_IMAGE_SIZE";
    case -41: return "CL_INVALID_SAMPLER";
    case -42: return "CL_INVALID_BINARY";
    case -43: return "CL_INVALID_BUILD_OPTIONS";
    case -44: return "CL_INVALID_PROGRAM";
    case -45: return "CL_INVALID_PROGRAM_EXECUTABLE";
    case -46: return "CL_INVALID_KERNEL_NAME";
    case -47: return "CL_INVALID_KERNEL_DEFINITION";
    case -48: return "CL_INVALID_KERNEL";
    case -49: return "CL_INVALID_ARG_INDEX";
    case -50: return "CL_INVALID_ARG_VALUE";
    case -51: return "CL_INVALID_ARG_SIZE";
    case -52: return "CL_INVALID_KERNEL_ARGS";
    case -53: return "CL_INVALID_WORK_DIMENSION";
    case -54: return "CL_INVALID_WORK_GROUP_SIZE";
    case -55: return "CL_INVALID_WORK_ITEM_SIZE";
    case -56: return "CL_INVALID_GLOBAL_OFFSET";
    case -57: return "CL_INVALID_EVENT_WAIT_LIST";
    case -58: return "CL_INVALID_EVENT";
    case -59: return "CL_INVALID_OPERATION";
    case -60: return "CL_INVALID_GL_OBJECT";
    case -61: return "CL_INVALID_BUFFER_SIZE";
    case -62: return "CL_INVALID_MIP_LEVEL";
    case -63: return "CL_INVALID_GLOBAL_WORK_SIZE";
    case -64: return "CL_INVALID_PROPERTY";
    case -65: return "CL_INVALID_IMAGE_DESCRIPTOR";
    case -66: return "CL_INVALID_COMPILER_OPTIONS";
    case -67: return "CL_INVALID_LINKER_OPTIONS";
    case -68: return "CL_INVALID_DEVICE_PARTITION_COUNT";

        // extension errors
    case -1000: return "CL_INVALID_GL_SHAREGROUP_REFERENCE_KHR";
    case -1001: return "CL_PLATFORM_NOT_FOUND_KHR";
    case -1002: return "CL_INVALID_D3D10_DEVICE_KHR";
    case -1003: return "CL_INVALID_D3D10_RESOURCE_KHR";
    case -1004: return "CL_D3D10_RESOURCE_ALREADY_ACQUIRED_KHR";
    case -1005: return "CL_D3D10_RESOURCE_NOT_ACQUIRED_KHR";
    default: return "Unknown OpenCL error";
        }
    }

