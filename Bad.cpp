/*
 * Implementation of the same clases in the normal Gaudi
 * This code have all the same clases that gaudi for to 
 * test the kallman filter with the origina function.
 */

#include "Bad.h"


namespace {
/// Helper function to filter one hits
inline float filter(const float z, float &x, float &tx,
                    float &covXX,float &covXTx,
                    float &covTxTx, const float zhit, 
                    const float xhit, const float whit) {
    // compute the prediction
    const float dz = zhit - z;
    const float predx = x + dz * tx;

    const float dz_t_covTxTx = dz * covTxTx;
    const float predcovXTx = covXTx + dz_t_covTxTx;
    const float dx_t_covXTx = dz * covXTx;

    const float predcovXX =covXX+2*dx_t_covXTx+dz*dz_t_covTxTx;
    const float predcovTxTx = covTxTx;
    // compute the gain matrix
    const float R = 1.0 / (1.0 / whit + predcovXX);
    const float Kx = predcovXX * R;
    const float KTx = predcovXTx * R;
    // update the state vector
    const float r = xhit - predx;
    x = predx + Kx * r;
    tx = tx + KTx * r;
    // update the covariance matrix.
    covXX = (1 - Kx) * predcovXX;
    covXTx = (1 - Kx) * predcovXTx;
    covTxTx = predcovTxTx - KTx * predcovXTx;
    // return the chi2
    return r*r*R;
    }
    }

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

    m_events.reserve(nbevts);
    
    int hpt, ign1, ign2;
    float x,y,z,wxerr,wyerr;

    while(getline(&line,&len,fp) != -1){
        const char val=line[0];
        // This next IF is not really needed,
        // but only for to be sure when debugging
        if (isalpha(val)){ 
            if(val=='T'){
                fscanf(fp,"Hits: %d",&hpt);
                PrPixelTrack tmptrack(hpt);
                m_events.back().addTrack(tmptrack);
                auto lasttrack=&(m_events.back().tracks().back());
                for(int i=0;i<hpt;i++){
                    fscanf(fp,"%f %f %f %*f %*f %f %f %*d %*d",
                               &x,&y,&z,&wxerr,&wyerr);
                    PrPixelHit tmphit(x,y,z,wxerr,wyerr);
                    lasttrack->addHit(tmphit);
                    }
                }
            else if(val=='E') {
                Event tmpevt;
                addEvent(tmpevt);
                }
            }
        }
    #ifdef DEBUG
        print();
    #endif
    }

void Run::print(){
    /// The print function is used only when -DDEBUG is defined
    int evt=0, track;
    for(auto i : m_events){
        track=0;
        printf("Event: %d, Tracks: %d\n", evt++, i.size());
        for(auto j : i.tracks()){
            printf("Track: %d\n", track++);
            printf("Hits: %d\n", j.size());
            for(auto k : j.hits()){
                k.print();
                }
            }
        }
    }
