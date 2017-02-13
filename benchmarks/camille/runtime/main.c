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

#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>

#define TEST_TIME 6

static unsigned int nproc;
static unsigned int isend;

static void test_exec(void);
static void test_print_results(int sig);

int main()
{
	struct sigaction sa;
	
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = test_print_results;
	if (0 != sigaction(SIGALRM, &sa, NULL)) {
		perror("sigaction");
		exit(1);
	}

	isend = 0;
	nproc = 16; // sysconf(_SC_NPROCESSORS_ONLN);

	alarm(TEST_TIME);
	test_exec();
	
	return 0;
}
