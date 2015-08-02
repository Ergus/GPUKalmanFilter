#include "Filter_Cuda.h"

__device__ float filter(const float z, float *x, float *tx,
                    float *covXX,float *covXTx,
                    float *covTxTx, const float zhit,
                    const float xhit, const float whit) {
    // compute the prediction
    const float dz = zhit - z;
    const float predx = (*x) + dz * (*tx);

    const float dz_t_covTxTx = dz * (*covTxTx);
    const float predcovXTx = (*covXTx) + dz_t_covTxTx;
    const float dx_t_covXTx = dz * (*covXTx);

    const float predcovXX =(*covXX)+2*dx_t_covXTx+dz*dz_t_covTxTx;
    const float predcovTxTx = (*covTxTx);
    // compute the gain matrix
    const float R = 1.0 / (1.0 / whit + predcovXX);
    const float Kx = predcovXX * R;
    const float KTx = predcovXTx * R;
    // update the state vector
    const float r = xhit - predx;
    *x = predx + Kx * r;
    *tx = (*tx) + KTx * r;
    // update the covariance matrix.
    *covXX = (1 - Kx) * predcovXX;
    *covXTx = (1 - Kx) * predcovXTx;
    *covTxTx = predcovTxTx - KTx * predcovXTx;
    // return the chi2
    return r*r*R;
    }

#if UCUDA == 1
__global__ void Kalman_Filter(float* ttrack,
                            int* trstart,
                            float* fullin,
                            int* backward,
                            float* sum2,
                            float* fullout,
                            const unsigned int tracks,
                            const unsigned int hits){

    //Declared before because it is the most important var
    const int idx = blockIdx.x*blockDim.x+threadIdx.x;

    if(idx>=tracks) return;
    
    #if (defined DEBUG && __CUDA_ARCH__ >= 200)
    if(idx==0)
        printf("Using kernel 1 for Cuda for %d tracks\n",tracks);
    #endif
    
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

#elif UCUDA == 2
__global__ void Kalman_Filter(float* ttrack,
                              int* trstart,
                              float* fullin,
                              int* backward,
                              float* sum2,
                              float* fullout,
                              const unsigned int tracks,
                              const unsigned int hits){

    //Declared before because it is the most important var
    const int idx = blockIdx.x*blockDim.x+threadIdx.x;
    if(idx>=tracks) return;
    
    const int idy = blockIdx.y*blockDim.y+threadIdx.y;
    #if (defined DEBUG && __CUDA_ARCH__ >= 200)
    if((idx==0) && (idy==0))
        printf("Using kernel 2 for Cuda for %d tracks\n",tracks);
    #endif
    
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
#endif


float cudaFilter(int *evstart,
               int *trstart,
               float *ttrack,
               float *fullin,
               int *backward,
               float *sum2,
               float *fullout,
               size_t events,
               size_t tracks,
               size_t hits){


    cudaEvent_t gpu_start, gpu_stop;
    double cpu_start, cpu_stop;
    float gpu_time;
    cudaEventCreate(&gpu_start);
    cudaEventCreate(&gpu_stop);
    
    dim3 dimBlock(LOCALSIZE,UCUDA),
         dimGrid( (tracks+LOCALSIZE-1)/LOCALSIZE,1);

    int *dev_evstart, *dev_trstart, *dev_backward;
    float *dev_ttrack, *dev_fullin, *dev_sum2, *dev_fullout;

    //=======Allocate memory in the device==================
    cudaMalloc((void**)&dev_evstart, (events+1)*sizeof(int)); //Array int start events [nbevents+1]
    cudaCheck(cudaMalloc((void**)&dev_trstart, (tracks+1)*sizeof(int))); //Array int start tracks [nbtracks+1]
    cudaCheck(cudaMalloc((void**)&dev_backward, (tracks)*sizeof(int)));  //Array bool backward    [nbtracks]

    cudaCheck(cudaMalloc((void**)&dev_ttrack, 2*tracks*sizeof(float)));  //Array for tx,ty/track  [2*tracks]
    cudaCheck(cudaMalloc((void**)&dev_fullin, 5*hits*sizeof(float)));    //Array float data hits  [5*nbhits]
    cudaCheck(cudaMalloc((void**)&dev_sum2, tracks*sizeof(float)));      //Array float parameter  [nbtracks]
    cudaCheck(cudaMalloc((void**)&dev_fullout, 11*tracks*sizeof(float)));//Array float results.   [11*nbtracks]

    //=======Copy arrays to the device======================
    cudaCheck(cudaMemcpy( dev_evstart, evstart, (events+1)*sizeof(int), cudaMemcpyHostToDevice ));
    cudaCheck(cudaMemcpy( dev_trstart, trstart, (tracks+1)*sizeof(int), cudaMemcpyHostToDevice ));
    cudaCheck(cudaMemcpy( dev_backward, backward, tracks*sizeof(int), cudaMemcpyHostToDevice ));
    
    cudaCheck(cudaMemcpy( dev_ttrack, ttrack, 2*tracks*sizeof(float), cudaMemcpyHostToDevice ));
    cudaCheck(cudaMemcpy( dev_fullin, fullin, 5*hits*sizeof(float), cudaMemcpyHostToDevice ));
    cudaCheck(cudaMemcpy( dev_sum2, sum2, tracks*sizeof(float), cudaMemcpyHostToDevice ));
    
    //----------------------------
    cudaEventRecord(gpu_start, 0);
    cpu_start=mtimes();
    Kalman_Filter<<<dimGrid,dimBlock>>>(dev_ttrack,
                                     dev_trstart,
                                     dev_fullin,
                                     dev_backward,
                                     dev_sum2,
                                     dev_fullout,
                                     tracks,
                                     hits);
    cudaDeviceSynchronize();
    cpu_stop=mtimes();
    cudaEventRecord(gpu_stop, 0);
    cudaEventSynchronize(gpu_stop);
    cudaEventElapsedTime(&gpu_time, gpu_start, gpu_stop);
        
    cudaMemcpy(fullout, dev_fullout, 11*tracks*sizeof(float), cudaMemcpyDeviceToHost);

    printf("Kernel execution time GPU= %0.3f ns\n", gpu_time*1000);
    printf("Kernel execution time CPU= %0.3lf ns\n", (cpu_stop-cpu_start)*1.0E6);

    cudaFree(dev_ttrack);
    cudaFree(dev_evstart);    
    cudaFree(dev_trstart);
    cudaFree(dev_fullin);
    cudaFree(dev_backward);
    cudaFree(dev_sum2);
    cudaFree(dev_fullout);
    return 0;
    }
