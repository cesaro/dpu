#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <assert.h>

void *main1_thd (void *arg)
{
   (void) arg;

   printf ("thread!\n");
   return arg;
}

int main1 (int argc, char ** argv)
{
   int ret;
   pthread_t th[2];

   (void) argc;
   (void) argv;

   ret = pthread_create (th + 0, 0, main1_thd, 0);
   assert (ret == 0);
   ret = pthread_create (th + 1, 0, main1_thd, 0);
   assert (ret == 0);

   ret = pthread_join (th[0], 0);
   assert (ret == 0);
   ret = pthread_join (th[1], 0);
   assert (ret == 0);
   return 0;
}

void * main2_thd (void *arg)
{
   int ret;
   pthread_t *th = (pthread_t *) arg;

   printf ("thread: arg %p\n", arg);

   if (! th) return 0;

   ret = pthread_create (th, 0, main2_thd, 0);
   assert (ret == 0);
   return 0;
}

int main2 (int argc, char ** argv)
{
   int ret;
   pthread_t th[2];

   (void) argc;
   (void) argv;

   printf ("main: creating 1 thread\n");
   ret = pthread_create (th + 0, 0, main2_thd, th + 1);
   assert (ret == 0);

   ret = pthread_join (th[0], 0);
   printf ("main: joined th[0]\n");
   assert (ret == 0);
   ret = pthread_join (th[1], 0);
   printf ("main: joined th[1]\n");
   assert (ret == 0);
   return 0;
}

pthread_mutex_t m3 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t m33 = PTHREAD_MUTEX_INITIALIZER;

void *main3_thd (void *arg)
{
   int ret;
   pthread_t *th = (pthread_t *) arg;

   printf ("thread: arg %p\n", arg);

   if (! th) return 0;

   // enter cs
   ret = pthread_mutex_lock (&m3);
   assert (ret == 0);
   // create a new thread
   ret = pthread_create (th, 0, main3_thd, 0);
   assert (ret == 0);
   // exit cs
   ret = pthread_mutex_unlock (&m3);
   assert (ret == 0);
   return 0;
}

int main3 (int argc, char ** argv)
{
   int ret;
   pthread_t th[3];

   (void) argc;
   (void) argv;

   ret = pthread_create (th + 0, 0, main3_thd, th + 2);
   assert (ret == 0);

   // enter cs
   ret = pthread_mutex_lock (&m3);
   assert (ret == 0);
   // create a second thread, "concurrently" to the thread creation in the other
   // thread
   ret = pthread_create (th + 1, 0, main3_thd, 0);
   assert (ret == 0);
   // exit cs
   ret = pthread_mutex_unlock (&m3);
   assert (ret == 0);

   // wait for all created threads
   ret = pthread_join (th[0], 0);
   assert (ret == 0);
   ret = pthread_join (th[1], 0);
   assert (ret == 0);
   ret = pthread_join (th[2], 0);
   assert (ret == 0);
   return 0;
}

void *main4_thd (void *arg)
{
   int ret, i;
   (void) arg;

   for (i = 0; i < 5; i++)
   {
      // enter cs
      ret = pthread_mutex_lock (&m3);
      assert (ret == 0);

      //printf ("t1 in cs\n");

      // exit cs
      ret = pthread_mutex_unlock (&m3);
      assert (ret == 0);
   }
   return 0;
}

int main4 (int argc, char ** argv)
{
   int ret, i;
   pthread_t th;

   (void) argc;
   (void) argv;

   ret = pthread_create (&th, 0, main4_thd, 0);
   assert (ret == 0);

   for (i = 0; i < 10; i++)
   {
      // enter cs
      ret = pthread_mutex_lock (&m3);
      assert (ret == 0);
      //printf ("t0 in cs\n");
      // exit cs
      ret = pthread_mutex_unlock (&m3);
      assert (ret == 0);
   }

   // wait for all created threads
   ret = pthread_join (th, 0);
   assert (ret == 0);
   return 0;
}

int main (int argc, char ** argv)
{
   assert (argc == 2);
   if (strcmp (argv[1], "main1") == 0) return main1 (argc, argv);
   if (strcmp (argv[1], "main2") == 0) return main2 (argc, argv);
   if (strcmp (argv[1], "main3") == 0) return main3 (argc, argv);
   if (strcmp (argv[1], "main4") == 0) return main4 (argc, argv);
   assert (0);
   return 0;
}

