/*
 * See copyright in copyright.h and the accompanying file COPYING
 */

/*
 *========================================================================
 * This is the Diehard BITSTREAM test, rewritten from the description
 * in tests.txt on George Marsaglia's diehard site.
 *
 *                   THE BITSTREAM TEST                          ::
 * The file under test is viewed as a stream of bits. Call them  ::
 * b1,b2,... .  Consider an alphabet with two "letters", 0 and 1 ::
 * and think of the stream of bits as a succession of 20-letter  ::
 * "words", overlapping.  Thus the first word is b1b2...b20, the ::
 * second is b2b3...b21, and so on.  The bitstream test counts   ::
 * the number of missing 20-letter (20-bit) words in a string of ::
 * 2^21 overlapping 20-letter words.  There are 2^20 possible 20 ::
 * letter words.  For a truly random string of 2^21+19 bits, the ::
 * number of missing words j should be (very close to) normally  ::
 * distributed with mean 141,909 and sigma 428.  Thus            ::
 *  (j-141909)/428 should be a standard normal variate (z score) ::
 * that leads to a uniform [0,1) p value.  The test is repeated  ::
 * twenty times.                                                 ::
 *
 * Except that in dieharder, the test is not run 20 times, it is
 * run a user-selectable number of times, default 100.
 *========================================================================
 *                       NOTE WELL!
 * If you use non-overlapping samples, sigma is 290, not 428.  This
 * makes sense -- overlapping samples aren't independent and so you
 * have fewer "independent" samples than you think you do, and
 * the variance is consequently larger.
 *========================================================================
 */

/* corrected/refactored version */

#include <dieharder/libdieharder.h>

/*
 * Include inline uint generator
 */
#include "static_get_bits.c"

int diehard_bitstream(Test **test, int irun)
{

 uint i,t;
 Xtest ptest;
 char *w;
 uint *bitstream,bitstreamlen,w20;
    
 /*
  * for display only.  0 means "ignored".
  */
 test[0]->ntuple = 0;

 /*
  * p = 141909, with sigma 428, for test[0]->tsamples = 2^21 20 bit ntuples.
  * a.k.a. the number of 20 bit integers missing from 2^21 random
  * samples drawn from this field.  At some point, I should be able
  * to figure out the expected value for missing integers as a rigorous
  * function of the size of the field sampled and number of samples drawn
  * and hence make this test capable of being run with variable sample
  * sizes, but at the moment I cannot do this and so test[0]->tsamples cannot be
  * varied.  Hence we work with diehard's values and hope that they are
  * correct.
  *
  * ptest.x = number of "missing ntuples" given 2^21 trials
  * ptest.y = 141909
  *
  * for non-overlapping samples we need (2^21)*5/8 = 1310720 uints, but
  * for luck we add one as we'd hate to run out.  For overlapping samples,
  * we need 2^21 BITS or 2^16 = 65536 uints, again plus one to be sure
  * we don't run out.
  */
#define BS_OVERLAP 65537
#define BS_NO_OVERLAP 1310720
    
ptest.y = 141909;
if(overlap){
    ptest.sigma = 428.0;
    bitstreamlen = BS_OVERLAP;
} else {
    ptest.sigma = 290.0;
    bitstreamlen = BS_NO_OVERLAP;
}
bitstream = (uint *)malloc(bitstreamlen*sizeof(uint));
for(i = 0; i < bitstreamlen; i++){
     bitstream[i] = get_rand_bits_uint(32,0xffffffff,rng);
}

/* array bitstream[0]bitstream[1].... as written in this order and in binary gives a sequence of bits */

w = (char *)malloc(M*sizeof(char)); /* can be bool, only 0 and 1 values are needed */
memset(w,0,M*sizeof(char));

unsigned int mask20= (1<<20)-1;
for(t=0;t<test[0]->tsamples;t++){
    unsigned int pos, intpos, intshift;
    if (overlap){pos=t;}else{pos=20*t;}
    intpos= pos/32; /* integer array element where the bit substrings starts */
    intshift= pos%32; /* number of unused bit in this element before the substring */
    if (intshift+20<=32){ /* one integer element is enough */
        w20= (bitstream[intpos]>>(32-20-intshift)) & mask20;
    }else{
        w20= ((bitstream[intpos]<<(intshift+20-32)) & mask20) |
        (bitstream[intpos+1]>>(2*32-20-intshift));
    }
    w[w20]=1;
}
ptest.x = 0;
for(i=0;i<M;i++){
   if(w[i] == 0){
     ptest.x++;
     /* printf("ptest.x = %f  Hole: w[%u] = %u\n",ptest.x,i,w[i]); */
   }
}
 if(verbose == D_DIEHARD_BITSTREAM || verbose == D_ALL){
   printf("%f %f %f\n",ptest.y,ptest.x,ptest.x-ptest.y);
 }

 Xtest_eval(&ptest);
 test[0]->pvalues[irun] = ptest.pvalue;

 /*
  * Don't forget to free or we'll leak.  Hate to have to wear
  * depends...
  */
 nullfree(w);
 nullfree(bitstream);

 return(0);
}
