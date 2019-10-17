/*
 * See copyright in copyright.h and the accompanying file COPYING
 */

/*
 *========================================================================
 * This test concatenates tsample of unsigned ints (obtained from the rng)
 * as a bit string X of length sts_n = tsamples * 32.
 *
 * For each m=1,2,...,nb = 16 (in the current code, but can be replaced by
 * any number less than 32) we count the number of occurences count[x] of
 * factor x in X_looped for all 2^m possible m-bit strings. Then we compute
 *     psi_square [m] = (2^{m}/sts_n)\sum_x (count[x]^2) - sts_n
 * as described in section 2.11.4 of NIST STS. We let psi_square[0]=0.
 *
 * Then the first and second differences are computed for m=2,...,16:
 *   delpsi[m] = psi_square[m] - psi_square[m-1];
 *   delpsi2[m] = psi_square[m] - 2psi_square[m-1] + psi_square[m-2];
 * and the p-values are computed using asymptotic approximation:
 *   pvalue1 = igamc(2^{m-2},delpsi[m]/2),
 *   pvalue2 = igamc(2^{m-3},delpsi2[m]/2).
 * [Here NIST description is inconsistent: 2-28 (5) does not include "/2",
 * but the example later does include division by 2. The numerical values
 * 0.9057 and 0.8805 in this example are incorrect, but correct values are
 * given in 2.11.6. The description above follows the NIST code.]
 *
 * For m=1 the binomial distribution is considered instead.
 * =======================================================================
 * --- refactored for speed/clarity --- AS
 *========================================================================
 */

#include <dieharder/libdieharder.h>

#include "static_get_bits.c"

int sts_serial(Test **test,int irun)
{

	int nb = 16;    /* Just ignore sts_serial_ntuple for now */
	int nb1 = nb+1; /* needed in some arrays */
	long powernb=pow(2,nb); /* number of all nb-bit factors */

	long tsamples = test[0]->tsamples;
	long sts_n = tsamples*32l; /* bit length of the string */

	uint * uintbuf = (uint *)malloc((tsamples+1)*sizeof(uint));
	/* array for the uints from rng, plus one additional for looping */
	for(int t=0;t<tsamples;t++){
		uintbuf[t] = get_rand_bits_uint(32,0xFFFFFFFF,rng);
	}
	uintbuf[tsamples] = uintbuf[0];   /* Periodic wraparound */
	/* buffer filled with uints from rng, the additional uint added*/

	long *count;
	count = (long *) malloc(powernb*sizeof(long));
    for (int i=0; i<powernb; i++){count[i]= 0;}
    /*count is an long array of 2^nb elements to count how many times each
     factor of length nb appears in the bit string */

    uint nb_mask = (((uint)1)<<nb)-1; /* 0^(32-nb) 1^nb; uses that nb < 32 */
    for (long pos=0; pos<sts_n; pos++){
        uint sts_factor;
        long intpos= pos/32; /* integer array element where the bit substrings starts */
        int intshift= pos%32; /* # of unused bits in this element before the substring */
        if (intshift+nb<=32){ /* one integer element is enough */
            sts_factor= (uintbuf[intpos]>>(32-nb-intshift)) & nb_mask;
        }else{ /* two integer elements are enough since nb<32 */
            sts_factor= ((uintbuf[intpos]<<(intshift+nb-32)) & nb_mask) |
            (uintbuf[intpos+1]>>(2*32-nb-intshift));
        }
        /* sts_factor is nb-bit substring in the bit string in position pos */
        count[sts_factor]++;
    }
    /* each nb-bit factor appears count[factor] times */
    free(uintbuf);

    double* psi_square = (double *) malloc((nb+1)*sizeof(double)); /* [0..nb] */
    int m = nb;
    /* psi_square [i] are correctly filled for i>m;
     count [0..2^m-1] are counts for m-bit factors */
    long count0, count1; /* # of 0 and 1 in the string, for later use */
    count0= 0l; count1=0l; /* dummy operation to prevent compiler unused-warning */
    while (m>0){
        unsigned int powerm = pow (2,m);
        double sum_squares = 0.0;
        for (int i=0; i<powerm; i++){
            sum_squares+=((double)count[i])*((double)count[i]);
        }
        psi_square[m]= ((double)powerm)*sum_squares/((double) sts_n)
                 - (double) sts_n;
        
        /* psi_quare [m] is correctly filled */
        m--; powerm/=2; /* first part of the invariant restored */
        /* count[0..2^(m+1)-1] are counts for (m+1)-bit factors */
        if (m==0){count0=count[0]; count1=count[1];} /* save number of 0/1 for future use */
        for (int i=0; i< powerm;i++){
            count[i]+= count[i+powerm];
        }
     }
    /* psi_square[1..nb] are filled; note that count [0] = sts_n */
    if (count[0]!=sts_n){printf("sts_serial internal error"); exit(1);}
    psi_square [0]=0.0; /* by definition */
    /* psi_square[m] for each m in 1..nb is computed, p.2-27 of NIST */
    free(count);

    int j=0;
    /* This is sts_monobit, basically.*/
    double mono_mean =  (double)count0-(double)count1;
    double mono_sigma = sqrt((double)sts_n);
    if(irun == 0){
    	test[j]->ntuple = 1;
    }
    test[j++]->pvalues[irun] = gsl_cdf_gaussian_P(mono_mean,mono_sigma);
    /* registering p-value for the #0-#1-test */

    for(int m=2;m<nb1;m++){
    	double del = psi_square[m] - psi_square[m-1];
    	if(irun == 0){ /* registering the size of substring if this is the first run */
    		test[j]->ntuple = m;
    	}
    	test[j++]->pvalues[irun] = gsl_sf_gamma_inc_Q(pow(2,m-2),del/2.0);
    	if(m>2){ /* second difference test not used for m=2, deviation from STS */
    		double del2 = psi_square[m] - 2.0*psi_square[m-1] + psi_square[m-2];
    		if(irun == 0){
    			test[j]->ntuple = m;
    		}
    		test[j++]->pvalues[irun] = gsl_sf_gamma_inc_Q(pow(2,m-3),del2/2.0);
    	}
    }

    free(psi_square);
    return(0);
}
