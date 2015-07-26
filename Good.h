#ifndef GOOD_H
#define GOOD_H

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#ifdef UOCL
#include "Filter_OpenCL.h"
#endif 

class sizes{
    public:
        sizes(const char fn[]);
        ~sizes();
        void print();
        #ifdef UOCL
        float Filter_OpenCL();
        #endif
        
        //Hits information, only the pointers
        float *x,*y,*z,*wxerr,*wyerr;
        
        //This is only 1 vale per track
        float *sum2;  
        bool *backward;
        

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

        //The state for output 1/track
        float *fullstate;
    };

#endif
