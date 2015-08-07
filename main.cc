
#include "Serializer.h"
#ifdef BAD_GOOD
    #define thistest "Bad_Good"
    #include "Good.h"
#elif defined GOOD
    #define thistest "Good"
    #include "Good.h"
#elif defined(BAD)
    #define thistest "Bad"
    #include "Bad.h"
#endif

#if (defined UOCL || defined UOCL2)
    #undef thistest
    #define thistest "OpenCL"
#endif

int main(int argc, char **argv){
    if(argc<2){
        printf("Usage ./executable input.dat\n");
        exit(EXIT_FAILURE);
        }

    printf("Test for %s\n",thistest);
    
#if (defined BAD_GOOD) || (defined GOOD)
    #ifdef GOOD
      sizes size(argv[1]);
    #elif defined BAD_GOOD
      Run run(argv[1]);
      sizes size(run);
    #endif
      
    size.fitKalman();
    size.save_results();    
    
#elif defined(BAD)
    Run run(argv[1]);
    Serializer ser("Bad_states.txt");
    run.filterall();
    ser(run);
#endif
    return 0;
    }
