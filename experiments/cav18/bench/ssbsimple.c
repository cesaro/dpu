#include <pthread.h>
#include <assert.h>

// This is an example of a program
// with three threads that contains SSBs

pthread_mutex_t mx;
pthread_mutex_t my;
pthread_mutex_t mz;
int x = 0;

void *ta(void *arg)
{
 pthread_mutex_lock(&mx);
   x = 1;
 pthread_mutex_unlock(&mx);

 pthread_mutex_lock(&mz);
 pthread_mutex_unlock(&mz);
 return NULL;
}

void *tb(void *arg)
{
 pthread_mutex_lock (&mx);
   if (x)
   {
     pthread_mutex_lock (&my);
     pthread_mutex_unlock (&my);
   }
 pthread_mutex_unlock (&mx);
 return NULL;
}

void *tc(void *arg)
{
 pthread_mutex_lock (&my);
   pthread_mutex_lock (&mz);
   pthread_mutex_unlock (&mz);
 pthread_mutex_unlock (&my);
 return NULL;
}

int main()
{
 pthread_t ida;
 pthread_t idb;
 pthread_t idc;
 pthread_mutex_init(&mx, NULL);
 pthread_mutex_init(&my, NULL);
 pthread_mutex_init(&mz, NULL);

 pthread_create(&idc,  NULL, tc, NULL);
 pthread_create(&idb,  NULL, tb, NULL);
 pthread_create(&ida,  NULL, ta, NULL);

 //pthread_join(ida,NULL);
 //pthread_join(idb,NULL);
 //pthread_join(idc,NULL);

 pthread_exit (0);
 return 0;
}
