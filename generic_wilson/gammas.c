/************************* gammas.c **********************************/
/* Routines for encoding gamma matrices and doing general gamma matrix
   arithmetic on spin wilson vectors.. Used for making general clover
   meson propagators */

/* MIMD version 7 */
/* C. DeTar 5/30/07 */

#include "generic_wilson_includes.h"
#include "../include/gammatypes.h"
#include <string.h>

/* This list must agree exactly with the enum gammatype in gammatypes.h */

static char *gammalabel[MAXGAMMA] = {"GX", "GY", "GZ", "GT", "G5", 
				     "GYZ", "GZX", "GXY", "GXT", "GYT", "GZT", 
				     "G5X", "G5Y", "G5Z", "G5T", "G1" };

/* First four gamma matrices are initialized according to the
   conventions in gammatypes.h and the ones for gamma_x gamma_y
   gamma_z gamma_t are used to generate the rest */

static gamma_matrix_t gamma_matrix[MAXGAMMA] = {
   {{{3,1},  {2,1},  {1,3},  {0,3}}},
   {{{3,2},  {2,0},  {1,0},  {0,2}}},
   {{{2,1},  {3,3},  {0,3},  {1,1}}},
   {{{2,0},  {3,0},  {0,0},  {1,0}}}, 
};

static int gamma_initialized = 0;


/************* make_gammas.c **************************/

/* Starting from gamma_x, gamma_y, gamma_z, gamma_t, form the rest of the
   16 gamma matrices 
   Gamma matrices are represented via a "gamma_matrix" structure */

/* Does g3 = phase*g1 * g2  - used only to initialize the gamma matrices */
static void mult_gamma(int phase, gamma_matrix_t *g1, gamma_matrix_t *g2, gamma_matrix_t *g3)
{
  int r,s;
  for(r=0;r<4;r++)
    {
      s = g1->row[r].column;
      g3->row[r].column = g2->row[s].column;
      g3->row[r].phase  = (g1->row[r].phase + g2->row[s].phase + phase) % 4;
    }
}

static void make_gammas(void)
{
  int r;
  gamma_matrix_t *gamma = gamma_matrix;  

  /* gamma_yz = i * gamma_y * gamma_z = sigma_{yz}, etc */
  mult_gamma(1,&gamma[GY ],&gamma[GZ ],&gamma[GYZ]);
  mult_gamma(1,&gamma[GZ ],&gamma[GX ],&gamma[GZX]);
  mult_gamma(1,&gamma[GX ],&gamma[GY ],&gamma[GXY]);

  mult_gamma(1,&gamma[GX ],&gamma[GT ],&gamma[GXT]);
  mult_gamma(1,&gamma[GY ],&gamma[GT ],&gamma[GYT]);
  mult_gamma(1,&gamma[GZ ],&gamma[GT ],&gamma[GZT]);

  /* gamma_5t = gamma_5 * gamma_t = gamma_x * gamma_y * gamma_z */
  /* phase 3 -> -i compensates for the i in gamma_xy */
  mult_gamma(3,&gamma[GX ],&gamma[GYZ],&gamma[G5T]);
  /* gamma_5 = gamma_x * gamma_y * gamma_z * gamma_t */
  mult_gamma(0,&gamma[G5T],&gamma[GT ],&gamma[G5 ]);
  mult_gamma(0,&gamma[G5 ],&gamma[GX ],&gamma[G5X]);
  mult_gamma(0,&gamma[G5 ],&gamma[GY ],&gamma[G5Y]);
  mult_gamma(0,&gamma[G5 ],&gamma[GZ ],&gamma[G5Z]);
 
  /* G1 is the unit matrix */
  for(r=0;r<4;r++)
    {
      gamma[G1].row[r].column = r;
      gamma[G1].row[r].phase = 0;
    }
  gamma_initialized = 1;
}


/************* msw_gamma_l.c (in su3.a) **************************/
/*
  Multiply a "spin Wilson vector" by a gamma matrix
  acting on the row index
  (This is the first index, or equivalently, multiplication on the left)
  usage:  mult_sw_by_gamma_l( src, dest, dir)
	spin_wilson_vector *src,*dest;
	int dir;    dir = any of the gamma matrix types in gammatypes.h


  Originally from MILC su3.a

  Modifications:
  UMH - modified for spin Wilson vector
  4/29/97 C. DeTar generalized to any gamma matrix.
*/


void mult_sw_by_gamma_l(spin_wilson_vector * src,spin_wilson_vector * dest, int dir)
{
  register int c2,s1,s2,s;	/* column indices, color and spin */

  if(gamma_initialized==0)make_gammas();

  /* For compatibility */
  if(dir == GAMMAFIVE)dir = G5;

  if(dir >= MAXGAMMA)
    {
      printf("mult_sw_by_gamma_l: Illegal gamma index %d\n",dir);
      terminate(1);
    }

  for(s1=0;s1<4;s1++){
    s = gamma_matrix[dir].row[s1].column;
    switch (gamma_matrix[dir].row[s1].phase){
    case 0:
      for(s2=0;s2<4;s2++)for(c2=0;c2<3;c2++){
	dest->d[s1].d[s2].c[c2] = src->d[s].d[s2].c[c2];}
      break;
    case 1:
      for(s2=0;s2<4;s2++)for(c2=0;c2<3;c2++){
	TIMESPLUSI( src->d[s].d[s2].c[c2], dest->d[s1].d[s2].c[c2] );}
      break;
    case 2:
      for(s2=0;s2<4;s2++)for(c2=0;c2<3;c2++){
	TIMESMINUSONE( src->d[s].d[s2].c[c2], dest->d[s1].d[s2].c[c2] );}
      break;
    case 3:
      for(s2=0;s2<4;s2++)for(c2=0;c2<3;c2++){
	TIMESMINUSI( src->d[s].d[s2].c[c2], dest->d[s1].d[s2].c[c2] );}
    }
  }
}

/************* msw_gamma_r.c (in su3.a) **************************/


/* 
  Multiply a "Wilson matrix" (spin_wilson_vector) by a gamma matrix
  acting on the column index
  (This is the second index, or equivalently, multiplication on the right)
  usage:  mb_gamma_r( src, dest, dir)
	spin_wilson_vector *src,*dest;
	int dir;    dir = XUP, YUP, ZUP, TUP or GAMMAFIVE
*/

void mult_sw_by_gamma_r(spin_wilson_vector * src,spin_wilson_vector * dest, int dir)
{
  register int c2,s1,s2,s;	/* column indices, color and spin */

  if(gamma_initialized==0)make_gammas();

  /* For compatibility */
  if(dir == GAMMAFIVE)dir = G5;
  if(dir >= MAXGAMMA)
    {
      printf("mult_sw_by_gamma_r: Illegal gamma index %d\n",dir);
      terminate(1);
    }

  for(s=0;s<4;s++){
    s2 = gamma_matrix[dir].row[s].column;
    switch (gamma_matrix[dir].row[s].phase){
    case 0:
      for(s1=0;s1<4;s1++)for(c2=0;c2<3;c2++){
	dest->d[s1].d[s2].c[c2] = src->d[s1].d[s].c[c2];}
      break;
    case 1:
      for(s1=0;s1<4;s1++)for(c2=0;c2<3;c2++){
	TIMESPLUSI( src->d[s1].d[s].c[c2], dest->d[s1].d[s2].c[c2] );}
      break;
    case 2:
      for(s1=0;s1<4;s1++)for(c2=0;c2<3;c2++){
	TIMESMINUSONE( src->d[s1].d[s].c[c2], dest->d[s1].d[s2].c[c2] );}
      break;
    case 3:
      for(s1=0;s1<4;s1++)for(c2=0;c2<3;c2++){
	TIMESMINUSI( src->d[s1].d[s].c[c2], dest->d[s1].d[s2].c[c2] );}
    }
  }
}

/* Accessor */

gamma_matrix_t gamma_mat(enum gammatype i){
  if(gamma_initialized == 0)make_gammas();

  return gamma_matrix[i];
}

/* Map a label to the gamma index */

int gamma_index(char *label){
  int i;
  for(i = 0; i < MAXGAMMA; i++){
    if(strcmp(label,gammalabel[i]) == 0)return i;
  }
  return -1;  /* Error condition */
}
