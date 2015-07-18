#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

class sizes{
    public:
        sizes(const char fn[]);
        ~sizes();
        void print();
        float *x,*y,*z,*wxerr,*wyerr;
        int *event_start, *tracks_start;
        

    private:
        FILE *fp;
        int nbevts, nbtracks, nbhits;
        float *full;
        
    };
