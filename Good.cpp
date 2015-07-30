#include "Good.h"

sizes::sizes(const char fn[]):
    ///
    nbevts(0),
    nbtracks(0),
    nbhits(0){
    fp=fopen(fn,"r");
    if(!fp){
        printf("Error opening the file\n");
        exit(EXIT_FAILURE);
        }
            
    char *line=NULL;
    size_t len=100;
    
    while(getline(&line,&len,fp) != -1){
        const char val=line[0];
        if (!isalpha(val)) nbhits++;
        else{
            if(val=='T') nbtracks++;
            else if(val=='E') nbevts++;
            }
        }
            
    printf("To import-> Hits: %d, Tracks: %d Events: %d\n",
           nbhits, nbtracks, nbevts);
            
    rewind(fp);

    // This is for hits array [xxx..yyy...zzz.. etc]
    full=(float*) malloc(5*nbhits*sizeof(float));

    // pointers that will be improved to strict for
    // better performance.
    x    =&full[0*nbhits];
    y    =&full[1*nbhits];
    z    =&full[2*nbhits];
    wxerr=&full[3*nbhits];
    wyerr=&full[4*nbhits];

    event_start =(int*) malloc((nbevts+1)*sizeof(int));
    tracks_start=(int*) malloc((nbtracks+1)*sizeof(int));

    // This values belong to every track;
    // They are allways 0 here, but for a better realistic benchmark
    // considering memory access were included;
    statein = (float*) calloc(2*nbtracks,sizeof(float));

    // This is the output array, have different shape for to be used
    // with structs [xyzxyzxyz]
    stateout =(float*) malloc(11*nbtracks*sizeof(float));

    // This are external values to be passed from outside to the filter
    // in real function this is calculated in MakeLHCB.
    sum2     =(float*) malloc(nbtracks*sizeof(float));
    backward =(int*) malloc(nbtracks*sizeof(int));
    
    
    int ih=0, it=0, ie=0, hpt;
    while(getline(&line,&len,fp) != -1){
        const char val=line[0];
        // This next IF is not really needed,
        // but only for to be sure when debugging
        if (isalpha(val)){ 
            if(val=='T'){
                tracks_start[it]=ih;
                sscanf(line,"Track: %*d, Sum2: %g, Backward: %d, Hits: %d\n",&sum2[it],&backward[it],&hpt);
                for(int i=0;i<hpt;i++,ih++){
                    fscanf(fp,
                          "%f %f %f %*f %*f %f %f %*d %*d",
                          &x[ih],&y[ih],&z[ih],
                          &wxerr[ih],&wyerr[ih]);
                    }
                it++;
                }
            else if(val=='E') event_start[ie++]=it;
            }
        }
    event_start[ie]=it;
    tracks_start[it]=ih;
    #ifdef DEBUG
        print();
    #endif
    }

sizes::~sizes(){
    fclose(fp);
    free(full);
    free(event_start);
    free(tracks_start);
    free(sum2);
    free(backward);
    free(stateout);
    free(statein);
    }

void sizes::print(){
    /// The print function is used only when -DDEBUG is defined
    for(int i=0;i<nbevts;i++){
        int evstart=event_start[i],
            evtend=event_start[i+1];
        printf("Event: %d, Tracks: %d\n",
               i, evtend-evstart);
        for(int j=evstart,cont=0;j<evtend;j++,cont++){
            int trstart=tracks_start[j],
                trend=tracks_start[j+1];
            printf("Track: %d, Sum2: %g, Backward: %d, Hits: %d\n",
                   cont,sum2[j],backward[j],trend-trstart);
            for(int ih=trstart;ih<trend;ih++){
                printf("%f %f %f %f %f\n",
                       x[ih],y[ih],z[ih],
                       wxerr[ih],wyerr[ih]);
                }
            }
        }
    }

void sizes::save_results(){
    FILE* fp=fopen(output,"w");
    for(int i=0;i<nbevts;i++){
        fprintf(fp,"Event: %d\n",i);
        int evstart=event_start[i],
            evtend =event_start[i+1];
        for(int j=evstart,cont=0;j<evtend;j++,cont++){
            int jt=11*j;
            fprintf(fp,"Track: %d\n", cont);
            fprintf(fp,"\t     %g %g %g %g %g\n\tCov: %g %g %g %g %g %g\n",stateout[jt],stateout[jt+1],stateout[jt+2],stateout[jt+3],stateout[jt+4],stateout[jt+5],stateout[jt+6],stateout[jt+7],stateout[jt+8],stateout[jt+9],stateout[jt+10]);
            }
        }
    fclose(fp);
    }

#if (defined UOCL || defined UOCL2)
float sizes::fitKalman(){
    printf("Filter with OpenCL %d\n", dimension);
    return clFilter(event_start,
                    tracks_start,
                    statein,
                    full,
                    backward,
                    sum2,
                    stateout,
                    nbevts,
                    nbtracks,
                    nbhits
                    );
    }
#else
float sizes::fitKalman(){
    printf("Filter with Good method\n");    
    for(int i=0;i<nbevts;i++){
        int evstart=event_start[i],
            evtend =event_start[i+1];
        for(int j=evstart,cont=0;j<evtend;j++,cont++){
            int trstart=tracks_start[j],
                trend=tracks_start[j+1]-1,
                direction=(backward[j]?1:-1),
                size=trend-trstart,
                dhit=1;
            const float noise2PerLayer=sum2[j];
            
            if((z[trend]-z[trstart])*direction<0){
                float tmp=trstart;                
                dhit=-1;
                trstart=trend;
                trend=tmp;
                }
            float lx = x[trstart],
                 ltx = statein[j],
                  ly = y[trstart],
                 lty = statein[j+nbtracks],
                  lz = z[trstart],
                 lwx = wxerr[trstart],
                 lwy = wyerr[trstart];
            // initialize the covariance matrix
            float covXTx  = 0.0f;  //no initial correlation
            float covYTy  = 0.0f;
            float covTxTx = 1.0f;  // randomly large error
            float covTyTy = 1.0f;
            float covXX = 1.0f /(lwx*lwx);
            float covYY = 1.0f /(lwy*lwy);
            float chi2=0.0f;
            
            for (int k=trstart+dhit;k!=trend+dhit;k+=dhit){
                lwx=wxerr[k];
                lwy=wyerr[k];
                covTxTx+=noise2PerLayer;
                covTyTy+=noise2PerLayer;

                // filter X
                chi2 += filter(lz,lx,ltx,covXX,covXTx,covTxTx,z[k], x[k], lwx*lwx);
                // filter Y
                chi2 += filter(lz,ly,lty,covYY,covYTy,covTyTy,z[k], y[k], lwy*lwy);
                lz=z[k];
                }
            
            covTxTx += noise2PerLayer;
            covTyTy += noise2PerLayer;

            int tmp=11*j;
            stateout[tmp+0]=lx;
            stateout[tmp+1]=ly;
            stateout[tmp+2]=lz;
            stateout[tmp+3]=ltx;
            stateout[tmp+4]=lty;

            stateout[tmp+5] = covXX;
            stateout[tmp+6] = covXTx;
            stateout[tmp+7] = covTxTx;
            stateout[tmp+8] = covYY;
            stateout[tmp+9] = covYTy;
            stateout[tmp+10]= covTyTy;            
            }
        }
    };
#endif
