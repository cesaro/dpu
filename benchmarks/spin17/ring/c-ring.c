/* The Computer Language Benchmarks Game
 * http://benchmarksgame.alioth.debian.org/

 * contributed by Premysl Hruby
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <limits.h>

#define THREADS (503)

#ifdef STATIC_TH_STACK
struct stack {
    char x[PTHREAD_STACK_MIN];
};
static struct stack* stacks;
//static struct stack stacks[THREADS];
#endif

/* staticaly initialize mutex[0] mutex */
static pthread_mutex_t* mutex;
static int* data;
static int nbth;

/* stacks must be defined staticaly, or my i386 box run of virtual memory for this
 * process while creating thread +- #400 */

static void* thread(void *num)
{
  int l = (int)num;
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
      pthread_mutex_unlock(mutex + r);
    } else {
      data[r] = 0;
      pthread_mutex_unlock(mutex + r);
      pthread_exit( 0 );
    }
  }
}



int main(int argc, char **argv)
{
    int i;
  pthread_t* cthread;
  pthread_attr_t stack_attr;

  if ( argc < 2 )
    exit(255);

  if ( argc >= 3 ){
      nbth = atoi(argv[2]);
  } else {
      nbth = THREADS;
  }

  cthread = (pthread_t*) malloc( nbth * sizeof( pthread_t ) );
  data = (int*) malloc( nbth * sizeof( int ) );
  mutex = (pthread_mutex_t*) malloc( nbth * sizeof( pthread_mutex_t ) );
#ifdef STATIC_TH_STACK
  stacks = (struct stack*) malloc( nbth * sizeof( struct stack ) );
#endif
  pthread_attr_init(&stack_attr);

  data[0] = atoi(argv[1]);

#ifndef STATIC_TH_STACK
  pthread_attr_setstacksize( &stack_attr, /* PTHREAD_STACK_MIN*/ sizeof( char ) );
#endif
  for (i = 0; i < nbth; i++) {
    pthread_mutex_init(mutex + i, NULL);
    pthread_mutex_lock(mutex + i);
#ifdef STATIC_TH_STACK
    pthread_attr_setstack(&stack_attr, &stacks[i], sizeof(struct stack));
#endif
    pthread_create( &(cthread[i]), &stack_attr, thread, (void*)i );
  }

  // pthread_mutex_unlock( mutex );
  for (i = 0; i < nbth; i++) {
      pthread_join(cthread[i], NULL);
  }
  //   pthread_mutex_unlock( mutex );

  if( NULL != cthread ) { free( cthread ); cthread = NULL; }
  if( NULL != data ) { free( data ); data = NULL; }
  if( NULL != mutex ) { free( mutex ); mutex = NULL; }
  pthread_attr_destroy( &stack_attr );
#ifdef STATIC_TH_STACK
  if( NULL != stacks ) { free( stacks ); stacks = NULL; }
#endif
 return EXIT_SUCCESS;
}
