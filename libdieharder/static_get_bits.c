/*
 *========================================================================
 * See copyright in copyright.h and the accompanying file COPYING
 *========================================================================
 */

/*
 *========================================================================
 * This should be a nice, big case switch where we add EACH test
 * we might want to do and either just configure and do it or
 * prompt for input (if absolutely necessary) and then do it.
 *========================================================================
 */

/*
 * This is a not-quite-drop-in replacement for my old get_rand_bits()
 * routine contributed by John E. Davis.
 *
 * It should give us the "next N bits" from the random number stream,
 * without dropping any.  The return can even be of arbitrary size --
 * we make the return a void pointer whose size is specified by the
 * caller (and guaranteed to be big enough to hold the result).
 */

/* Refactored; new bits are directly obtained from the generator only if
 buffer is empty, so it is now is a clean optimization -- AS */

inline static uint get_rand_bits_uint (uint nbits, uint mask, gsl_rng *rng)
/* get nint <= 32 bits from the input stread;
 assumes that mask is either 0 or 111...111 (nbits ones)
 -- for optimization  */
{
    if (nbits==0) {return 0;}
    if(nbits > 32){
        fprintf(stderr,"Warning!  dieharder cannot yet work with\b");
        fprintf(stderr,"           %u > 32 bit chunks.  Exiting!\n\n",nbits);
        exit(0);
    }
    /* 0 < nbits <= 32 */
    static uint bit_buffer;
    static uint bits_left_in_bit_buffer = 0;
    
    /* the unused part of the stream is bits_left_in_bit_buffer
     least significant bits in bit_buffer, to be read from left to right,
     plus yet unread bits from the generator */

    /* optimization for the case when no bits are in the buffer and
     the same number of bits is requested (nbits) and provided
     by the rng (rmax_bits) */
    if((nbits == rmax_bits)&&(bits_left_in_bit_buffer==0)){
        return gsl_rng_get(rng);
    }
  
    /* produce mask with nbits 1s; shift trick works only for n<32 */
    if(mask == 0){
        if (nbits < 32){mask = (1u << nbits) - 1;}else{mask = 0xFFFFFFFF;}
    }
    
    if (bits_left_in_bit_buffer >= nbits) { /* buffer has enough bits */
        bits_left_in_bit_buffer -= nbits; /* new length stored */
        return (bit_buffer >> bits_left_in_bit_buffer) & mask;
    }
    /* bits_left_in_bit_buffer < nbits, concatenation needed */
    uint answer; /* here the answer will be composed */
    nbits-= bits_left_in_bit_buffer; /* planned number of missing bits */
    /* nbits > 0 */
    if (nbits < 32){
        answer= (bit_buffer << nbits) & mask;
    }else{
        answer= 0; /* special case neded since 32 shift does not work! */
    }
    /* buffer is empty; it remains to fill nbits>0 least significant bits
     in answer, now being zeros, by fresh bits from rng */
    while (1) {
        bit_buffer = gsl_rng_get (rng);
        bits_left_in_bit_buffer = rmax_bits;
        /* non-used part of the bit_buffer is filled by zeros */
        if (bits_left_in_bit_buffer >= nbits) {
            bits_left_in_bit_buffer -= nbits;
            answer |= (bit_buffer >> bits_left_in_bit_buffer);
            return (answer);
        } /* nbits > bits_left_in_bit_buffer */
        nbits -= bits_left_in_bit_buffer; /* will be missing */
        answer |= (bit_buffer << nbits);
        bits_left_in_bit_buffer= 0; /* for clarity; actually redundant */
    }
}


/* unchanged starting from this point - AS */

/*
 * This is a drop-in-replacement for get_bit_ntuple() contributed by
 * John E. Davis.  It speeds up this code substantially but may
 * require work if/when rngs that generate 64-bit rands come along.
 * But then, so will other programs.
 */
inline static uint get_bit_ntuple_from_uint (uint bitstr, uint nbits, 
                                      uint mask, uint boffset)
{
   uint result;
   uint len;
   
   /* Only rmax_bits in bitstr are meaningful */
   boffset = boffset % rmax_bits;
   result = bitstr >> boffset;
   
   if (boffset + nbits <= rmax_bits)
     return result & mask;
   
   /* Need to wrap */
   len = rmax_bits - boffset;
   while (len < nbits)
     {
	result |= (bitstr << len);
	len += rmax_bits;
     }
   return result & mask;
}

/*
 * David Bauer doesn't like using the routine above to "fix" the
 * problem that some generators don't return 32 bit random uints.  This
 * version of the routine just ignore rmax_bits.  If a routine returns
 * 31 or 24 bit uints, tough.  This is harmless enough since nobody cares
 * about obsolete generators that return signed uints or worse anyway, I
 * imagine.  It MIGHT affect people writing HW generators that return only
 * 16 bits at a time or the like -- they need to be advised to wrap their
 * call routines up to return uints.  It's faster, too -- less checking
 * of the stream, fewer conditionals.
 */
inline static uint get_bit_ntuple_from_whole_uint (uint bitstr, uint nbits, 
		uint mask, uint boffset)
{
 uint result;
 uint len;

 result = bitstr >> boffset;

 if (boffset + nbits <= 32) return result & mask;

 /* Need to wrap */
 len = 32 - boffset;
 while (len < nbits) {
   result |= (bitstr << len);
   len += 32;
 }

 return result & mask;

}
 
