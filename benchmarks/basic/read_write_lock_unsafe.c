/* Testcase from Threader's distribution. For details see:
   http://www.model.in.tum.de/~popeea/research/threader

   This file is adapted from the example introduced in the paper:
   Thread-Modular Verification for Shared-Memory Programs 
   by Cormac Flanagan, Stephen Freund, Shaz Qadeer.
*/

#include <pthread.h>
#include <assert.h>

int w=0;
int r=0;
int x; 
int y;

void * writer (void * arg) { //writer
  //__VERIFIER_atomic_take_write_lock();  
  if(w ==0)
    if(r ==0)
      w = 1;
    
  x = 3;
  w = 0;
  
  return 0;
}

void * reader (void * arg) { //reader
  //__VERIFIER_atomic_take_read_lock();
  if(w==0){
    r++;
  }
  y = x;
  assert (y == x);
  r--;
 
  return 0;
}

int main (int argc, char** argv) {
    pthread_t tids[4];

    pthread_create (tids + 0, 0, writer, 0);
    pthread_create (tids + 1, 0, writer, 0);
    pthread_create (tids + 2, 0, reader, 0);
    pthread_create (tids + 3, 0, reader, 0);

    pthread_join (tids[0], 0);
    pthread_join (tids[1], 0);
    pthread_join (tids[2], 0);
    pthread_join (tids[3], 0);
    
    return 0;
}
