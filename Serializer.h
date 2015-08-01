#include "Bad.h"

class Serializer{
    public:
        Serializer(const char fn[]){
            fp=fopen(fn,"a");
            }
        ~Serializer(){
            fclose(fp);
            }
        void operator()(State state, int trackNb){
            fprintf(fp,"Track: %d\n", trackNb);
            fprintf(fp,"\t     %g %g %g %g %g\n\tCov: %g %g %g %g %g %g\n",
                    state.x(),state.y(),state.z(),
                    state.tx(),state.ty(),
                    state.covariance()(0, 0),
                    state.covariance()(0, 2),
                    state.covariance()(2, 2),
                    state.covariance()(1, 1),
                    state.covariance()(1, 3),
                    state.covariance()(3, 3)
                    );
            }
        void operator()(Run &run){
            int tracks, evts=run.size();
            for(int i=0;i<evts;i++){
                fprintf(fp,"Event: %d\n",i);
                tracks=run.m_allstates[i].size();
                for(int j=0;j<tracks;j++){
                    (*this)(run.m_allstates[i][j],j);
                    }
                }
            }


    private:
        FILE *fp;
        int m_trackNb;
    };
