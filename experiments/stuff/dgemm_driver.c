#include <stdlib.h>
#include <stdio.h>

#define dgemm dgemm_ 
#define dlarnv dlarnv_

#define DEFAULTSIZE  128
#define NBREP        10

/* constants */
static int NEXEC=1;
static int IONE=1;
static double c__1=1.0;
char Lower='L', Transpose='T', NoTranspose='N', Forward='F', 
  Rowwise='R',Columnwise='C', Left='L', NonUnit='N', Right='R', Up='U', NormM='M';

/* Lapack/Blas interface */
void dgemm(char *transa, char *transb, int *m, int *n, int *k, 
             double *alpha, double *a, int *lda, double *b, int *ldb, 
             double *beta, double *c, int *ldc);
void dlarnv(int *idist, int *iseed, int *n, double *x);

/* Internal interfaces */
int  i, N, INFO, NN, THREADS;
double *A, *B, *C, time1, time2, sum, GFlops;
int ISEED[4] = {0,0,0,1};   /* initial seed for dlarnv() */
   
int main (int argc, char **argv){
  int N, NN, i;
  
  if( argc < 2 ) {
    N = DEFAULTSIZE;
  } else {
        N = atoi( argv[1] );
  }
  NN  = N*N;
  
  /* Allocate data */
  A    = (double *)malloc(N*N*sizeof(double));
  B    = (double *)malloc(N*N*sizeof(double));
  C    = (double *)malloc(N*N*sizeof(double));
  
  /* Fill the matrix with random values */
  dlarnv(&IONE, ISEED, &NN, A);
  dlarnv(&IONE, ISEED, &NN, B);
  dlarnv(&IONE, ISEED, &NN, C);
  
  /* Call the kernel */
  for( i = 0 ; i < NBREP ; i++ ) {
    dgemm(&NoTranspose,&NoTranspose,&N,&N,&N,&c__1,A,&N,B,&N,&c__1,C,&N);
  }
  
  /* Done */
  free(A);
  free(B);
  free(C);
  
    return EXIT_SUCCESS;
}

