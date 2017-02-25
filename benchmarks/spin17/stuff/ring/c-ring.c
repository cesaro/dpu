/* The Computer Language Benchmarks Game
 * http://benchmarksgame.alioth.debian.org/

 * contributed by Premysl Hruby
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <limits.h>

#define TOKEN 20
#define THREADS 8

/* staticaly initialize mutex[0] mutex */
static pthread_mutex_t* mutex;
static int* data;
static int nbth;

/* stacks must be defined staticaly, or my i386 box run of virtual memory for this
 * process while creating thread +- #400 */

static void* thread(void *num)
{
  int l = (long int)num;
  int r = (l+1) % nbth;
  int token;

  if( 0 == num ) {
      pthread_mutex_unlock( mutex + l );
  }
  
  while(1) {
    pthread_mutex_lock(mutex + l);
    token = data[l];
    if (token) {
      data[r] = token - 1;
      printf ("t%d: recv %d send %d\n", l, token, token - 1);
      pthread_mutex_unlock(mutex + r);
    } else {
      data[r] = 0;
      printf ("t%d: recv 0 send 0; exit!\n", l);
      pthread_mutex_unlock(mutex + r);
      pthread_exit( 0 );
    }
  }
}

void usage ()
{
  printf ("Usage: ring [TOKEN [NTH]]\n");
  printf (" TOKEN is %d by default\n", TOKEN);
  printf (" NTH is %d by default\n", THREADS);
  exit (1);
}

int main(int argc, char **argv)
{
  long int i;
  int token;
  pthread_t* cthread;
  pthread_attr_t stack_attr;

  // parse arguments
  if (argc > 3) usage ();
  if (argc == 3)
      nbth = atoi(argv[2]);
  else
      nbth = THREADS;
  if (argc >= 2)
      token = atoi(argv[1]);
  else
      token = TOKEN;
  printf ("ring: nbth %d, token %d\n", nbth, token);

  cthread = (pthread_t*) malloc( nbth * sizeof( pthread_t ) );
  data = (int*) malloc( nbth * sizeof( int ) );
  mutex = (pthread_mutex_t*) malloc( nbth * sizeof( pthread_mutex_t ) );
  data[0] = token;

  for (i = 0; i < nbth; i++) {
    pthread_mutex_init(mutex + i, NULL);
    pthread_mutex_lock(mutex + i);
    pthread_create( &(cthread[i]), 0, thread, (void*)i );
  }

  // pthread_mutex_unlock( mutex );
  for (i = 0; i < nbth; i++) {
      pthread_join(cthread[i], NULL);
  }
  //   pthread_mutex_unlock( mutex );

  if( NULL != cthread ) { free( cthread ); cthread = NULL; }
  if( NULL != data ) { free( data ); data = NULL; }
  if( NULL != mutex ) { free( mutex ); mutex = NULL; }
 return EXIT_SUCCESS;
}
