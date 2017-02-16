#include <pthread.h>
#include <stdio.h>
#include <assert.h>

#define T 6
#define K 11

pthread_mutex_t m[K];

/*
The stair pattern
a
|
b . b   
|   |
d   c . c
    |   |
    e   d . d
        |   |
        f   e . e
            |   |
            g   f . f
*/

// parametric threads
void *th(void *arg)
{
 unsigned id = (unsigned long) arg;

 int i = id;
 int j = 1;

 // Locks
 while (i < K && j < 4) {
   // printf ("th %u: lock %d\n", id, i);
   pthread_mutex_lock(&m[i]);
   i+=j;
   j++;
 }

 // Unlocks
 while (i > id) {
   j--;
   i-=j;
   // printf ("th %u: unlock %d\n", id, i);
   pthread_mutex_unlock(&m[i]);
 }
}

int main()
{
 pthread_t ids[T];

 for (int i = 0; i < K; i++)
 {
   pthread_mutex_init(&m[i], NULL);
 }
 for (int i = T-1; i >= 0; i--)
 {
   pthread_create(&ids[i],  NULL, th, (void*) (long) i);
 }

 for (int i = 0; i < T; i++)
 {
   pthread_join(ids[i],NULL);
 }

}
