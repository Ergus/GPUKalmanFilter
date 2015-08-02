/*
 * Implementation of the same clases in the normal Gaudi
 * This code have all the same clases that gaudi for to 
 * test the kallman filter with the origina function.
 */

#include "Bad.h"
#include "Serializer.h"

float PrPixelTrack::fitKalman(State &state, const int direct,
                              const float noise2PerLayer)const {
    // assume the hits are sorted, but don't assume anything on the direction of
    // sorting
    int firsthit = 0;
    int lasthit = m_hits.size() - 1;
    int dhit = +1;
    
    if ((m_hits[lasthit].z()-m_hits[firsthit].z())*direct<0){
        std::swap(firsthit, lasthit);
        dhit = -1;
        }

    // we filter x and y simultaneously
    // but take them uncorrelated.
    // filter first the first hit.

    const PrPixelHit *hit = &m_hits[firsthit];
    float x = hit->x();
    float tx = m_tx;
    float y = hit->y();
    float ty = m_ty;
    float z = hit->z();

    // initialize the covariance matrix
    float covXX = 1 / hit->wx();
    float covYY = 1 / hit->wy();
    float covXTx = 0;  // no initial correlation
    float covYTy = 0;
    float covTxTx = 1;  // randomly large error
    float covTyTy = 1;

    // add remaining hits
    float chi2(0);
    for (int i=firsthit+dhit; i!=lasthit+dhit; i+=dhit) {
        hit = &m_hits[i];
        // add the noise
        covTxTx += noise2PerLayer;
        covTyTy += noise2PerLayer;
        // filter X
        chi2 += filter(z, x, tx, covXX, covXTx, covTxTx,
                       hit->z(), hit->x(), hit->wx());
        // filter Y
        chi2 +=filter(z, y, ty, covYY, covYTy, covTyTy,
                      hit->z(), hit->y(), hit->wy());
        // update z (not done in the filter, needed only once)
        z = hit->z();
        }

    // add the noise at the last hit
    covTxTx += noise2PerLayer;
    covTyTy += noise2PerLayer;

    // finally, fill the state
    state.setX(x);
    state.setY(y);
    state.setZ(z);
    state.setTx(tx);
    state.setTy(ty);
    state.covariance()(0, 0) = covXX;
    state.covariance()(0, 2) = covXTx;
    state.covariance()(2, 2) = covTxTx;
    state.covariance()(1, 1) = covYY;
    state.covariance()(1, 3) = covYTy;
    state.covariance()(3, 3) = covTyTy;

    return chi2;
    }

//======= End Events ===========
//=========== Run  =============

Run::Run(const char fn[])
    : nbevts(0),nbtracks(0),nbhits(0){
    
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

    m_allstates=vector<States> (nbevts);
    
    sum2.reserve(nbevts);
    backward.reserve(nbevts);
    
    int hpt, read_backward;
    float x,y,z,wxerr,wyerr,read_sum2;

    while(getline(&line,&len,fp) != -1){
        const char val=line[0];
        // This next IF is not really needed,
        // but only for to be sure when debugging
        if (isalpha(val)){
            if(val=='T'){
                sscanf(line,"Track: %*d, Sum2: %g, Backward: %d, Hits: %d\n",
                       &read_sum2,&read_backward,&hpt);
                sum2.back().push_back(read_sum2);
                backward.back().push_back(read_backward);
                PrPixelTrack tmptrack(hpt);
                m_events.back().addTrack(tmptrack);
                auto lasttr=&(m_events.back().tracks().back());
                for(int i=0;i<hpt;i++){
                    fscanf(fp,"%f %f %f %*f %*f %f %f %*d %*d",
                               &x,&y,&z,&wxerr,&wyerr);
                    PrPixelHit tmphit(x,y,z,wxerr,wyerr);
                    lasttr->addHit(tmphit);
                    }
                }
            else if(val=='E') {
                Event tmpevt;
                addEvent(tmpevt);
                m_events.back().tracks().reserve(100);
                sum2.push_back(vector<float>());
                sum2.back().reserve(100);
                backward.push_back(vector<bool>());
                backward.back().reserve(100);
                }
            }
        }

    }

void Run::print(){
    /// The print function is used only when -DDEBUG is defined
    int nbev=m_events.size(), nbtr, nbht;
    for(int i=0;i<nbev;i++){
        nbtr=m_events[i].size();
        printf("Event: %d, Tracks: %d\n", i, nbtr);
        for(int j=0;j<nbtr;j++){
            auto tmp=m_events[i].tracks()[j].hits();
            nbht=tmp.size();
            printf("Track: %d, Sum2: %.8g, Backward: %d, Hits: %d\n",
                   j, sum2[i][j], int(backward[i][j]), nbht);
            
            for(int k=0;k<nbht;k++) tmp[k].print();
            }
        }
    }

void Run::filterall(){
    printf("Filter with Bad method\n");
    const double begin = mtimes();
    int nbev=m_events.size(), nbtr;
    for(int i=0;i<nbev;i++){
        nbtr=m_events[i].size();
        for(int j=0;j<nbtr;j++){            
            State mystate;
            m_events[i].tracks()[j].fitKalman(mystate, backward[i][j] ? 1:-1, sum2[i][j]);
            m_allstates[i].push_back(mystate);   
      
            }
        }
    const double end = mtimes();
    printf("Total time Filtering: %lg s\n\n",end - begin);
    }

