#ifndef FILTER_CUDA_H_
#define FILTER_CUDA_H_ 1

#ifndef LOCALSIZE
#define LOCALSIZE 32
#endif

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

    #ifdef __CUDACC__
    #define cudaCheck(stmt) __cudaCheck( stmt, __FILE__, __LINE__ )
    
    inline void __cudaCheck( cudaError err, const char *file, const int line ){
        #ifdef DEBUG
        if ( cudaSuccess != err ){
            fprintf( stderr, "Error at %s:%i : %s\n",
                     file, line, cudaGetErrorString( err ) );
            exit( -1 );
            }
        #endif
        return;
        }
    #endif
    float cudaFilter(int *evstart,
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
