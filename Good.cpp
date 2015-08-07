#include "Good.h"

sizes::sizes(Run &run):fp(NULL){
    
    nbevts=run.nbevts;
    nbtracks=run.nbtracks;
    nbhits=run.nbhits;
    allocate();
    // Start time measure here, because memory allocation can be heavy and
    // that space for sure will be already allocated
    
    double t1=mtimes();
    printf("To bad->good: Hits: %d, Tracks: %d Events: %d\n", nbhits, nbtracks, nbevts);

    Events *m_events=&run.m_events;
    vector<vector<float> > *lsum2=&run.sum2;
    vector<vector<bool> > *lbackward=&run.backward;
    
    int nbtr, nbht, it=0, ih=0;
    for(int i=0;i<nbevts;i++){
        event_start[i]=it;        
        nbtr=(*m_events)[i].size();
        for(int j=0; j<nbtr; j++,it++){
            auto tmp=(*m_events)[i].tracks()[j].hits();
            tracks_start[it]=ih;
            sum2[it]=(*lsum2)[i][j];
            backward[it]=(int)(*lbackward)[i][j];
            statein[it]=(*m_events)[i].tracks()[j].m_tx;
            statein[it+nbtracks]=(*m_events)[i].tracks()[j].m_ty;
            nbht=tmp.size();
            for(int k=0;k<nbht;k++,ih++){
                x[ih]=tmp[k].x();
                y[ih]=tmp[k].y();
                z[ih]=tmp[k].z();
                wxerr[ih]=tmp[k].wxerr();
                wyerr[ih]=tmp[k].wyerr();
                };
            }
        }
    event_start[nbevts]=it;
    tracks_start[nbtracks]=ih;
    cumtime=mtimes()-t1;
    }

sizes::sizes(const char fn[]):
    ///
    nbevts(0),
    nbtracks(0),
    nbhits(0),
    cumtime(0.0){
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
            
    printf("To import-> Hits: %d, Tracks: %d Events: %d\n",nbhits, nbtracks, nbevts);            
    rewind(fp);

    //This function allocates memory and assign pointer directions
    allocate();

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
    }

void sizes::allocate(){
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
    }

sizes::~sizes(){
    if(fp) fclose(fp);
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
            printf("Track: %d, Sum2: %g, Backward: %d, Hits: %d\n",cont,sum2[j],backward[j],trend-trstart);
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

#ifdef UOCL
void sizes::fitKalman(){
    printf("Filter with OpenCL %d\n", UOCL);

    const double begin = mtimes();
    clFilter(event_start,
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
    const double end = mtimes();
    printf("Time CPU total %s%d Events: %d Tracks: %d Hits: %d = %le s\n\n", method, UOCL, nbevts,  nbtracks,  nbhits, end - begin );
    }

#elif defined UCUDA
void sizes::fitKalman(){
    printf("Filter with Cuda %d\n", UCUDA);

    const double begin = mtimes();
    cudaFilter(event_start,
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
    const double end = mtimes();
    printf("Time CPU total %s%d Events: %d Tracks: %d Hits: %d = %le s\n\n",
                     method, UCUDA, nbevts,  nbtracks,  nbhits, end - begin );
    }
#else
void sizes::fitKalman(){
    printf("Filter with %s method\n",method);
    const double begin = mtimes();
    for(int i=0;i<nbevts;i++){
        int evstart=event_start[i],
            evtend =event_start[i+1];
        for(int j=evstart,cont=0;j<evtend;j++,cont++){
            int trstart=tracks_start[j],
                trend=tracks_start[j+1]-1,
                direction=(backward[j]?1:-1),
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
                chi2 += filter(lz,lx,ltx,covXX,covXTx,covTxTx,z[k], x[k], lwx*lwx);
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
    const double end = mtimes();
    cumtime+=(end-begin);
    printf("Time CPU total %s Events: %d Tracks: %d Hits: %d = %le s\n\n", method, nbevts,  nbtracks,  nbhits, cumtime);
    };
#endif
