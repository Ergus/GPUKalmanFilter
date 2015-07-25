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
    statein = (float*) calloc(4*nbtracks,sizeof(float));

    // This is the output array, have different shape for to be used
    // with structs [xyzxyzxyz]
    stateout =(float*) malloc(11*nbtracks*sizeof(float));

    // This are external values to be passed from outside to the filter
    // in real function this is calculated in MakeLHCB.
    sum2     =(float*) malloc(nbtracks*sizeof(float));
    backward =(bool*) malloc(nbtracks*sizeof(bool));
    
    
    int ih=0, it=0, ie=0, hpt, tmp;
    while(getline(&line,&len,fp) != -1){
        const char val=line[0];
        // This next IF is not really needed,
        // but only for to be sure when debugging
        if (isalpha(val)){ 
            if(val=='T'){
                tracks_start[it]=ih;
                sscanf(line,"Track: %*d, Sum2: %g, Backward: %d, Hits: %d\n",
                       &sum2[it],&tmp,&hpt);
                backward[it]=(bool)tmp;
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

