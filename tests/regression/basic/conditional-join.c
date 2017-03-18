#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <assert.h>

// we have two threads, thread1 and thread2, which enter a critical section
// where thread2 sets the flag to 0 and thread1 reads the flag; thread1 will
// create another thread (thread3) and will join for it only when the flag is 1,
// and then create another thread

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
int flag = 1;

void *thread1 (void *arg);
void *thread2 (void *arg);
void *thread3 (void *arg);

void *thread1 (void *arg)
{
   int ret;
   pthread_t th;

   (void) arg;

   // create one thread that we will join conditionally
   ret = pthread_create (&th, 0, thread3, 0);

   pthread_mutex_lock(&m);
   if (flag)
   {
      ret = pthread_join (th, 0); // here
      assert (ret == 0);
   }
   pthread_mutex_unlock(&m);

   // create a second thread, which will reuse the tid of the previously created
   // thread on only one execution out of the two
   ret = pthread_create (&th, 0, thread3, 0);
   assert (ret == 0);

   return 0;
}

void *thread2 (void *arg)
{
   (void) arg;

   pthread_mutex_lock(&m);
   flag = 0;
   pthread_mutex_unlock(&m);

   return 0;
}

void *thread3 (void *arg)
{
   (void) arg;
   return 0;
}

int main (int argc, char ** argv)
{
   int ret;
   pthread_t th;

   (void) argc;
   (void) argv;

   ret = pthread_mutex_init (&m, 0);
   assert (ret == 0);

   ret = pthread_create (&th, 0, thread1, 0);
   assert (ret == 0);
   ret = pthread_create (&th, 0, thread2, 0);
   assert (ret == 0);
   pthread_exit (0);
}
