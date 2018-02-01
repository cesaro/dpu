#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

// default values for PARAM1 and PARAM2
#ifndef PARAM1
#define PARAM1 3
#endif
#ifndef PARAM2
#define PARAM2 2
#endif

#define NUM PARAM1 // number of writer threads
#define LEN PARAM2 // length of the sequence of conflicts

int x = 0;
int y = 0;
int z = 0;
int v = 0;
int c = 0;
pthread_mutex_t mc;
pthread_mutex_t mx;
pthread_mutex_t my;
pthread_mutex_t mz;

pthread_mutex_t mut[NUM];
int tab[NUM];

int main1();
int main2();

void *thread1 (void *arg)
{
   unsigned id = (unsigned long) arg;

   pthread_mutex_lock (mut + id);
   //printf ("w%d: lock\n", id);
   tab[id] = 123;
   pthread_mutex_unlock (mut + id);

   return 0;
}

void *thread_rz (void *arg)
{
   int l, i;

   pthread_mutex_lock (&mz);
   l = z;
   //printf ("rz: read z %d\n", l);
   pthread_mutex_unlock (&mz);
   if (l == 1 && NUM > 0)
   {
#if 0
      for (i = 0; i < NUM; i++)
      {
         tab[i] = 8899;
         printf ("rz: write i %d\n", i);
      }
#else
      pthread_mutex_lock (&mc);
      i = c;
      //printf ("rz: read c %d\n", i);
      pthread_mutex_unlock (&mc);

      pthread_mutex_lock (mut + i);
      tab[i] = 8899;
      //printf ("rz: lock %d\n", i);
      pthread_mutex_unlock (mut + i);
#endif
   }
   else
   {
      //printf ("rz: main2\n");
      main2 ();
   }

   return 0;
}

void *thread_wc (void *arg)
{
   int i;

   // race on z
   pthread_mutex_lock (&mz);
   z = 1;
   //printf ("wc: z = 1\n");
   pthread_mutex_unlock (&mz);

   // NUM races on c
   for (i = 1; i < NUM; i++)
   {
      pthread_mutex_lock (&mc);
      c = i;
      //printf ("wc: write c %d\n", c);
      pthread_mutex_unlock (&mc);
   }
   return 0;
}

int main1 ()
{
   pthread_t t[NUM];
   pthread_t t2;
   pthread_t t3;
   int i;

   //printf ("== start ==\n");

   pthread_mutex_init (&mz, 0);
   pthread_mutex_init (&mc, 0);

   for (i = 0; i < NUM; i++)
   {
      pthread_mutex_init (mut + i, 0);
      pthread_create (&t[i], 0, thread1, (void*) (long) i);
   }
   pthread_create (&t3, 0, thread_wc, 0);
   pthread_create (&t2, 0, thread_rz, 0);

   //pthread_exit (0);

   for (i = 0; i < NUM; i++)
   {
      pthread_join (t[i], 0);
   }
   pthread_join (t2, 0);
   pthread_join (t3, 0);
   //printf ("== end ==\n\n");
   return 0;
}

void *thread2 (void *arg)
{
   int i;
   for (i = 0; i < LEN; i++)
   {
      pthread_mutex_lock (&mx);
      x = 10;
      //printf ("thread2: write x\n");
      pthread_mutex_unlock (&mx);
   }
   return 0;
}

int main2 ()
{
   int i;
   pthread_t t;
   pthread_mutex_init (&mx, 0);
   pthread_create (&t, 0, thread2, 0);

   for (i = 0; i < LEN; i++)
   {
      pthread_mutex_lock (&mx);
      x = 20;
      //printf ("main2: write x\n");
      pthread_mutex_unlock (&mx);
   }

   pthread_join (t, 0);
   return 0;
}

int main ()
{
   main1 ();
   return 0;
}
