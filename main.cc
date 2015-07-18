
#ifdef GOOD
    #include "Good.h"
#elif defined(BAD)
    #include "Bad.h"
#endif

int main(int argc, char **argv){
    if(argc<2){
        printf("Usage ./executable input.dat\n");
        exit(EXIT_FAILURE);
        }
#ifdef GOOD
    sizes size(argv[1]);
    printf("Good memory test\n");
#elif defined(BAD)
    Run run(argv[1]);
    printf("Bad memory test\n");
#endif
    return 0;
    }
