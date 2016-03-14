/* Copyright (c) Colorado School of Mines, 1990.
/* All rights reserved.                       */

/*
FUNCTION:  prime factor fft:  complex to real transform

PARAMETERS:
isign       i sign of isign is the sign of exponent in fourier kernel
n           i length of transform (see notes below)
cz          i array of n/2+1 complex values (may be equivalenced to rz)
rz          o array of n real values (may be equivalenced to cz)
 
NOTES:
Because pfacr uses pfacc to do most of the work, n must be even 
and n/2 must be a valid length for pfacc.  The simplest way to
obtain a valid n is via n = npfar(nmin).  A more optimal n can be 
obtained with npfaro.

REFERENCES:  
Press et al, 1988, Numerical Recipes in C, p. 417.

Also, see notes and references for function pfacc.

AUTHOR:  I. D. Hale, Colorado School of Mines, 06/13/89
*/

#include "cwp.h"

void pfacr (int isign, int n, complex cz[], float rz[])
{
    int i,ir,ii,jr,ji,no2;
    float *z,tempr,tempi,sumr,sumi,difr,difi;
    double wr,wi,wpr,wpi,wtemp,theta;

    /* copy input to output and fix dc and nyquist */
    z = (float*)cz;
    for (i=2; i<n; i++)
        rz[i] = z[i];
    rz[1] = z[0]-z[n];
    rz[0] = z[0]+z[n];
    z = rz;

    /* initialize cosine-sine recurrence */
    theta = 2.0*PI/(double)n;
    if (isign>0) theta = -theta;
    wtemp = sin(0.5*theta);
    wpr = -2.0*wtemp*wtemp;
    wpi = sin(theta);
    wr = 1.0+wpr;
    wi = wpi;

    /* twiddle */
    no2 = n/2;
    for (ir=2,ii=3,jr=n-2,ji=n-1; ir<=no2; ir+=2,ii+=2,jr-=2,ji-=2) {
        sumr = z[ir]+z[jr];
        sumi = z[ii]+z[ji];
        difr = z[ir]-z[jr];
        difi = z[ii]-z[ji];
        tempr = wi*difr-wr*sumi;
        tempi = wi*sumi+wr*difr;
        z[ir] = sumr+tempr;
        z[ii] = difi+tempi;
        z[jr] = sumr-tempr;
        z[ji] = tempi-difi;
        wtemp = wr;
        wr += wr*wpr-wi*wpi;
        wi += wi*wpr+wtemp*wpi;
    }

    /* do complex to complex transform */
    pfacc(isign,n/2,(complex*)z);
}
