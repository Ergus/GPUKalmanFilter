
#include "Serializer.h"
#ifdef GOOD
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
#ifdef GOOD
    sizes size(argv[1]);
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
