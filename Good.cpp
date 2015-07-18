#ifndef GOOD_H
#define GOOD_H

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
    full=(float*) malloc(5*nbhits*sizeof(float));
    x    =&full[0*nbhits];
    y    =&full[1*nbhits];
    z    =&full[2*nbhits];
    wxerr=&full[3*nbhits];
    wyerr=&full[4*nbhits];

    event_start=(int*) malloc((nbevts+1)*sizeof(int));
    tracks_start=(int*) malloc((nbtracks+1)*sizeof(int));
    int ih=0, it=0, ie=0, hpt, ign1, ign2;
    while(getline(&line,&len,fp) != -1){
        const char val=line[0];
        // This next IF is not really needed,
        // but only for to be sure when debugging
        if (isalpha(val)){ 
            if(val=='T'){
                tracks_start[it]=ih;
                fscanf(fp,"Hits: %d",&hpt);
                for(int i=0;i<hpt;i++,ih++){
                    fscanf(fp,
                           "%f %f %f %*f %*f %f %f %d %d",
                           &x[ih],&y[ih],&z[ih],
                           &wxerr[ih],&wyerr[ih],
                           &ign1,&ign2);
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
            printf("Track: %d\n",cont);
            printf("Hits: %d\n",trend-trstart);
            for(int ih=trstart;ih<trend;ih++){
                printf("%f %f %f %f %f\n",
                       x[ih],y[ih],z[ih],
                       wxerr[ih],wyerr[ih]);
                }
            }
        }
    }

#endif
