#include <pthread.h>
#include <stdio.h>
#include <assert.h>

#ifndef LD1
#define LD1 0
#endif

#ifndef LD2
#define LD2 0
#endif

#ifndef LD3
#define LD3 0
#endif

#ifndef LD4
#define LD4 0
#endif

#ifndef LD5
#define LD5 0
#endif

#ifndef LD6
#define LD6 0
#endif

#ifndef LD7
#define LD7 0
#endif

#ifndef LD8
#define LD8 0
#endif

#ifndef LD9
#define LD9 0
#endif

#ifndef LD10
#define LD10 0
#endif

#ifndef LD11
#define LD11 0
#endif

#ifndef ST1
#define ST1 0
#endif

#ifndef ST2
#define ST2 0
#endif

#ifndef ST3
#define ST3 0
#endif

#ifndef ST4
#define ST4 0
#endif

#ifndef ST5
#define ST5 0
#endif

#ifndef ST6
#define ST6 0
#endif

#ifndef ST7
#define ST7 0
#endif

#ifndef ST8
#define ST8 0
#endif

#ifndef ST9
#define ST9 0
#endif

#ifndef ST10
#define ST10 0
#endif

#ifndef ST11
#define ST11 0
#endif


#define LOCATION(N) \
   if (LD##N) local += shared; \
   if (ST##N) shared = local;

pthread_mutex_t m1 = PTHREAD_MUTEX_INITIALIZER;

int shared = 133;

void *ta (void *arg)
{
   // if you make it 0, compiler optimizations will remove some of the store
   // instructions ;)
   int local = 17;

   (void) arg;
   printf ("ta: running\n");

   LOCATION (1)
   pthread_mutex_lock (&m1);
   LOCATION (2)
   pthread_mutex_unlock (&m1);
   LOCATION (3)

   return (void*) (size_t) local;
}

void *tb (void *arg)
{
   int local = 17;

   (void) arg;
   printf ("tb: running\n");

   LOCATION (4)
   pthread_mutex_lock (&m1);
   LOCATION (5)
   pthread_mutex_unlock (&m1);
   LOCATION (6)

   return (void*) (size_t) local;
}

int main()
{
   pthread_t a, b;
   int local = 17;

   LOCATION (7);
   pthread_create (&a, NULL, ta, NULL);
   LOCATION (8);
   pthread_create (&b, NULL, tb, NULL);
   LOCATION (9);

   pthread_join (b, NULL);
   LOCATION (10);
   pthread_join (a, NULL);
   LOCATION (11);

   return local;
}

