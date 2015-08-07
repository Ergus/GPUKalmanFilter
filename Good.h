#ifndef GOOD_H
#define GOOD_H

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "Filter.h"
#include "Bad.h"

#ifdef UOCL
  #include "Filter_OpenCL.h"
  #if UOCL == 1
    #define output "Opencl_states.txt"
  #elif UOCL == 2
    #define output "Opencl_states2.txt"
  #endif
#elif defined UCUDA
  #include "Filter_Cuda.h"
  #if UCUDA == 1
    #define output "Cuda_states.txt"
  #elif UCUDA == 2
    #define output "Cuda_states2.txt"
  #endif
#elif defined BAD_GOOD
  #define method "Bad_Good"
  #define output "BG_states.txt"
#else
  #define method "Good"
  #define output "Good_states.txt"
#endif

class sizes{
    public:
        sizes(Run &run);
        sizes(const char fn[]);
        ~sizes();
        void print();
        void save_results();
        void fitKalman();        
        
        //Hits information, only the pointers
        float *x,*y,*z,*wxerr,*wyerr;
        
        //This is only 1 vale per track
        float *sum2;  
        int *backward;
        

        //This will contain the indices
        int *event_start, *tracks_start;

        // In a different way state out have the values grouped
        // because it is a write array
        float *statein;
        float *stateout;
        
    private:
        FILE *fp;
        int nbevts, nbtracks, nbhits;
        // This is one array containing x,y,z,wxerr,wyerr
        // 1/hit
        float *full;
        void allocate();
        double cumtime;
    };

#endif
