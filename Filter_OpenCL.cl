/// Helper function to filter one hits
float filter(const float z, float *x, float *tx,
                    float *covXX,float *covXTx,
                    float *covTxTx, const float zhit,
                    const float xhit, const float whit) {
    // compute the prediction
    private const float dz = zhit - z;
    private const float predx = (*x) + dz * (*tx);

    private const float dz_t_covTxTx = dz * (*covTxTx);
    private const float predcovXTx = (*covXTx) + dz_t_covTxTx;
    private const float dx_t_covXTx = dz * (*covXTx);

    private const float predcovXX =(*covXX)+2*dx_t_covXTx+dz*dz_t_covTxTx;
    private const float predcovTxTx = (*covTxTx);
    // compute the gain matrix
    private const float R = 1.0 / (1.0 / whit + predcovXX);
    private const float Kx = predcovXX * R;
    private const float KTx = predcovXTx * R;
    // update the state vector
    private const float r = xhit - predx;
    *x = predx + Kx * r;
    *tx = (*tx) + KTx * r;
    // update the covariance matrix.
    *covXX = (1 - Kx) * predcovXX;
    *covXTx = (1 - Kx) * predcovXTx;
    *covTxTx = predcovTxTx - KTx * predcovXTx;
    // return the chi2
    return r*r*R;
    }

__kernel void Kalman_Filter(__constant float* ttrack,
                            __constant int* trstart,
                            __global float* fullin,
                            __constant int* backward,
                            __constant float* sum2,
                            __global float* fullout,
                            const unsigned int tracks,
                            const unsigned int hits){

    //Declared before because it is the most important var
    const int idx = get_global_id(0);

    if(idx>=tracks) return;
    
    int first = trstart[idx],
        last = trstart[idx+1],
        direction = (backward[idx] ? 1 : -1),
        dhit, size=last-first-1;
    const float noise2PerLayer=sum2[idx];
    
    float ax[24],ay[24],
        az[24],aerrx[24],aerry[24];

    for(int i=first, j=0;i<last;i++, j++){
        ax[j]   = fullin[i];
        ay[j]   = fullin[i+hits];
        az[j]   = fullin[i+2*hits];
        aerrx[j]= fullin[i+3*hits];
        aerry[j]= fullin[i+4*hits];        
        }

    if((az[size]-az[0])*direction<0){
        dhit=-1;
        first=size;
        last=-1;
        }
    else{
        dhit=1;
        last=size+1;
        first=0;
        }
    
    float x = ax[first],
        tx = ttrack[idx],
         y = ay[first],
        ty = ttrack[idx+tracks],
         z = az[first],
        wx = aerrx[first],
        wy = aerry[first];
    
    // initialize the covariance matrix
    float covXTx  = 0.0f;  // no initial correlation
    float covYTy  = 0.0f;
    float covTxTx = 1.0f;  // randomly large error
    float covTyTy = 1.0f;
    float covXX = 1.0f /(wx*wx);
    float covYY = 1.0f /(wy*wy);
    float chi2=0.0f;
    
    for (int i=first+dhit; i!=last; i+=dhit) {    
        wx=aerrx[i];
        wy=aerry[i];
        covTxTx+=noise2PerLayer;
        covTyTy+=noise2PerLayer;

        // filter X
        chi2 += filter(z, &x, &tx, &covXX, &covXTx, &covTxTx,az[i], ax[i], wx*wx);
        // filter Y
        chi2 += filter(z, &y, &ty, &covYY, &covYTy, &covTyTy,az[i], ay[i], wy*wy);
        z=az[i];
        }

    // add the noise at the last hit
    covTxTx += noise2PerLayer;
    covTyTy += noise2PerLayer;

    // finally, fill the state
    int tmp=11*idx;
    if(idx<10)
        printf("%d %g %g\n", idx, x, covXX);
    fullout[tmp+0]=x;
    fullout[tmp+1]=y;
    fullout[tmp+2]=z;
    fullout[tmp+3]=tx;
    fullout[tmp+4]=ty;

    fullout[tmp+5] = covXX;
    fullout[tmp+6] = covXTx;
    fullout[tmp+7] = covTxTx;
    fullout[tmp+8] = covYY;
    fullout[tmp+9] = covYTy;
    fullout[tmp+10]= covTyTy;
    
    }

