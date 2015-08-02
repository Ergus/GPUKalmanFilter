#ifndef FILTER_H_
#define FILTER_H_ 1

#include <time.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif
    
inline double mtimes(){
    struct timeval tmp;
    double sec;
    gettimeofday( &tmp, (struct timezone *)0 );
    sec = tmp.tv_sec + ((double)tmp.tv_usec)/1000000.0;
    return sec;
    }

#ifdef __cplusplus
}
#endif
/// Helper function to filter one hits
#ifndef UOCL
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
#endif
#endif
