/*
 *========================================================================
 *  rng_XOR.c
 *
 * This generator takes a list of generators on the dieharder
 * command line and XOR's their output together.
 *========================================================================
 */

#include <dieharder/libdieharder.h>

/*
 * This is a special XOR generator that takes a list of GSL
 * wrapped rngs and XOR's their uint output together to produce
 * each new random number.  Note that it SKIPS THE FIRST ONE which
 * MUST be the XOR rng itself.  So there have to be at least two -g X
 * stanzas on the command line to use XOR, and if there aren't three
 * or more it doesn't "do" anything but use the second one.
 */
static unsigned long int XOR_get (void *vstate);
static double XOR_get_double (void *vstate);
static void XOR_set (void *vstate, unsigned long int s);

typedef struct {
  /*
   * internal gsl random number generator vector
   */
  gsl_rng *grngs[GVECMAX];
  unsigned int XOR_rnd;
  input_params_t *params;
} XOR_state_t;

gsl_rng *XOR_rng_alloc (const gsl_rng_type * T, input_params_t *params)
{

  gsl_rng *r = (gsl_rng *) malloc (sizeof (gsl_rng));

  if (r == 0)
    {
      GSL_ERROR_VAL ("failed to allocate space for rng struct",
                        GSL_ENOMEM, 0);
    };

  r->state = calloc (1, T->size);

  if (r->state == 0)
    {
      free (r);

      GSL_ERROR_VAL ("failed to allocate space for rng state",
                        GSL_ENOMEM, 0);
    };
  XOR_state_t *state = (XOR_state_t *) r->state;
  state->params = params;

  r->type = T;

  gsl_rng_set (r, gsl_rng_default_seed);

  return r;
}
gsl_rng *file_input_alloc (const gsl_rng_type * T, char *filename);
gsl_rng *wrap_gsl_rng_alloc(input_params_t *params, unsigned int i) {
  unsigned int curr_gnum = params->gnumbs[i];
  const gsl_rng_type *T = dh_rng_types[curr_gnum];
  if (curr_gnum == 207) {
    return XOR_rng_alloc(T, params);
  }
  if(strncmp("file_input",T->name,10) == 0){
    return file_input_alloc(T, params->filenames[params->gfilenum[(i > 0)? (i - 1) : 0]]);
  }
  return gsl_rng_alloc(T);
}

static inline unsigned long int
XOR_get (void *vstate)
{
 XOR_state_t *state = (XOR_state_t *) vstate;
 int i;

 /*
  * There is always this one, or we are in deep trouble.  I am going
  * to have to decorate this code with error checks...
  */
 state->XOR_rnd = gsl_rng_get(state->grngs[0]);
 for(i=1;i<state->params->gvcount - 1;i++){
   state->XOR_rnd ^= gsl_rng_get(state->grngs[i]);
 }
 /* xor etalon with tested generator if etalon_xor enabled*/
 if (state->params->is_etalon && etalon_xor) {
   state->XOR_rnd ^= gsl_rng_get(generator.rng);
 }
 return state->XOR_rnd;
 
}

static double
XOR_get_double (void *vstate)
{
  return XOR_get (vstate) / (double) UINT_MAX;
}

static void XOR_set (void *vstate, unsigned long int s) {

 XOR_state_t *state = (XOR_state_t *) vstate;
 int i;
 input_params_t *params = state->params;

 /*
  * OK, here's how it works.  grngs[0] is set to mt19937_1999, seeded
  * as per usual, and used (ONLY) to see the remaining generators.
  * The remaining generators.
  */
 for(i=0;i<state->params->gvcount - 1;i++){
   /*
    * I may need to (and probably should) add a sanity check
    * here or in choose_rng() to be sure that all of the rngs
    * exist.
    */
   if (state->grngs[i] == NULL) {
     state->grngs[i] = wrap_gsl_rng_alloc (params, i + 1);
   }
   gsl_rng_set(state->grngs[i],params->gseeds[i]);

 }

}

static const gsl_rng_type XOR_type =
{"XOR (supergenerator)",        /* name */
 UINT_MAX,			/* RAND_MAX */
 0,				/* RAND_MIN */
 sizeof (XOR_state_t),
 &XOR_set,
 &XOR_get,
 &XOR_get_double};

const gsl_rng_type *gsl_rng_XOR = &XOR_type;
