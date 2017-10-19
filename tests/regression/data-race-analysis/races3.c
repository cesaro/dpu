#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#ifndef N
#define N 20
#endif

#ifndef DIFF
#define DIFF 0
#endif

#ifndef WAIT2
#define WAIT2 1
#endif


struct descriptor
{
  int tid;
  unsigned offset;
  unsigned size;
  int *tab;
};

void bubble_sort(int list[], unsigned n)
{
  int c, d, t;

  for (c = 0 ; c < ( n - 1 ); c++)
  {
    for (d = 0 ; d < n - c - 1; d++)
    {
      if (list[d] > list[d+1])
      {
        t = list[d];
        list[d] = list[d+1];
        list[d+1] = t;
      }
    }
  }
}

void print (int tab[], unsigned n)
{
  int i;
  for (i = 0; i < n; i++)
  {
     if (i == n / 2) printf ("--\n");
     printf("i %3d tab %3d\n", i, tab[i]);
  }
}

void *thread (void *arg)
{
  struct descriptor *d = arg;
  printf ("t%d: sorting %p, offset %u size %u!\n",
        d->tid, d->tab, d->offset, d->size);
  bubble_sort (d->tab + d->offset, d->size);
  return 0;
}

int main()
{
  pthread_t a, b;
  struct descriptor d[2];
  int i;
  unsigned n;

  n = N & ~1;
  int tab[n];

  srand (123);
  for (i = 0; i < n; i++) tab[i] = rand () % 500;

  d[0].tid = 0;
  d[0].offset = 0;
  d[0].size = n / 2;
  d[0].tab = tab;

  d[1].tid = 1;
  d[1].offset = d[0].offset + d[0].size - DIFF;
  d[1].size = n / 2;
  d[1].tab = tab;

  printf ("m: sorting %p, size %u!\n", tab, n);

  pthread_create (&a, NULL, thread, d + 0);
  pthread_create (&b, NULL, thread, d + 1);

  pthread_join (a, NULL);
  if (WAIT2)
    pthread_join (b, NULL);
    
  //bubble_sort (tab, n);
 
  print (tab, n);
  pthread_exit (0);
 
  return 0;
}
