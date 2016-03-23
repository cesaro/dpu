
// adapted from
// https://raw.githubusercontent.com/dbeyer/sv-benchmarks/master/c/pthread/sigma_false-unreach-call.c

#include "../pthread.h"

#define SIGMA 7

int array[SIGMA];
int array_index = 0;

void *thread0 (void * arg)
{
	int i = array_index;
	array[i] = 1;
	// return 0;
}
void *thread1 (void * arg)
{
	int i = array_index;
	array[i] = 1;
	// return 0;
}

void *thread2 (void * arg)
{
	int i = array_index;
	array[i] = 1;
	// return 0;
}

void *thread3 (void * arg)
{
	int i = array_index;
	array[i] = 1;
	// return 0;
}

void *thread4 (void * arg)
{
	int i = array_index;
	array[i] = 1;
	// return 0;
}

void *thread5 (void * arg)
{
	int i = array_index;
	array[i] = 1;
	// return 0;
}

void *thread6 (void * arg)
{
	int i = array_index;
	array[i] = 1;
	// return 0;
}


void main()
{
	int i, sum;
	pthread_t t0, t1, t2, t3, t4, t5, t6;

	i = 0;
	pthread_create (t0, NULL, thread0, NULL);
	array_index = i;
	i = i + 1;
	pthread_create (t1, NULL, thread1, NULL);
	array_index = i;
	i = i + 1;
	pthread_create (t2, NULL, thread2, NULL);
	array_index = i;
	i = i + 1;
	pthread_create (t3, NULL, thread3, NULL);
	array_index = i;
	i = i + 1;
	pthread_create (t4, NULL, thread4, NULL);
	array_index = i;
	i = i + 1;
	pthread_create (t5, NULL, thread5, NULL);
	array_index = i;
	i = i + 1;
	pthread_create (t6, NULL, thread6, NULL);

	pthread_join (t0, NULL);
	pthread_join (t1, NULL);
	pthread_join (t2, NULL);
	pthread_join (t3, NULL);
	pthread_join (t4, NULL);
	pthread_join (t5, NULL);
	pthread_join (t6, NULL);

	sum = 0;
	for (i = 0; i < SIGMA; i = i + 1) sum = sum + array[i];

	// __VERIFIER_assert(sum == SIGMA); // unsafe, different threads might use the same array offset when writing
	if (sum != SIGMA) __poet_fail ();
}

