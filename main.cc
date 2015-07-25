
#include "Serializer.h"
#ifdef GOOD
    #include "Good.h"
#elif defined(BAD)
    #include "Bad.h"
#endif

#include "Filter_OpenCL.h"

int main(int argc, char **argv){
    if(argc<2){
        printf("Usage ./executable input.dat\n");
        exit(EXIT_FAILURE);
        }

    Serializer ser("Bad_states.txt");
    
#ifdef GOOD
    sizes size(argv[1]);
    printf("Good memory test\n");
    
#elif defined(BAD)
    Run run(argv[1]);
    run.filterall();
    ser(run);
    printf("Bad memory test\n");
#endif
    return 0;
    }
