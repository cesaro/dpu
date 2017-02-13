/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2010, 2011 "the PM2 team" (see AUTHORS file)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#include <pthread.h>
#include "main.c"

static unsigned long count;
static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

static void *lock_unlock(void *arg)
{
	while (! isend) {
		pthread_mutex_lock(&m);
		count ++;
		pthread_mutex_unlock(&m);
	}

	return arg;
}

static void test_exec(void)
{
	pthread_t t;

	printf("-- Uncontested mutex_lock test (duration: %ds) --\n", 
	       TEST_TIME);

	count = 0;
	pthread_create(&t, NULL, lock_unlock, NULL);
	pthread_join(t, NULL);
}

static void test_print_results(int sig)
{
	isend = 1;
	printf("%ld mutex taken in %d seconds [%ld lock/s]\n",
	       count, TEST_TIME, count/TEST_TIME);
}
