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
    
    const int idy = get_global_id(1);
    if(idx==0 && idy==0)
        printf("OpenCL dim 2 from idx= %d, idy= %d\n", idx, idy);  
    
    int first = trstart[idx],
        last = trstart[idx+1],
        direction = (backward[idx] ? 1 : -1),
        dhit, size=last-first-1;
    const float noise2PerLayer=sum2[idx];
    
    float ax[24],az[24],aerrx[24];

    for(int i=first, j=0;i<last;i++, j++){
        ax[j]   = fullin[i+idy*hits];
        az[j]   = fullin[i+2*hits];
        aerrx[j]= fullin[i+(idy+3)*hits];
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
         z = az[first],
        wx = aerrx[first];
    
    // initialize the covariance matrix
    float covXTx  = 0.0f;  // no initial correlation
    float covTxTx = 1.0f;  // randomly large error
    float covXX = 1.0f /(wx*wx);
    float chi2=0.0f;
    
    for (int i=first+dhit; i!=last; i+=dhit) {    
        wx=aerrx[i];
        covTxTx+=noise2PerLayer;

        // filter X
        chi2 += filter(z, &x, &tx, &covXX, &covXTx, &covTxTx,az[i], ax[i], wx*wx);
        // filter Y
        z=az[i];
        }

    // add the noise at the last hit
    covTxTx += noise2PerLayer;

    // finally, fill the state
    int tmp=11*idx;
    fullout[tmp+idy]=x;
    if(idy==0) fullout[tmp+2]=z;
    fullout[tmp+3+idy]=tx;

    fullout[tmp+5+3*idy] = covXX;
    fullout[tmp+6+3*idy] = covXTx;
    fullout[tmp+7+3*idy] = covTxTx;
    
    }
