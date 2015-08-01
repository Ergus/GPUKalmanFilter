#ifndef GOOD_H
#define GOOD_H

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "Filter.h"

#if (defined UOCL || defined UOCL2)
#include "Filter_OpenCL.h"
#endif

#ifdef UOCL
#define output "Opencl_states.txt"
#elif defined UOCL2
#define output "Opencl_states2.txt"
#else
#include <time.h>
#define output "Good_states.txt"
#endif

class sizes{
    public:
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

    };

#endif
