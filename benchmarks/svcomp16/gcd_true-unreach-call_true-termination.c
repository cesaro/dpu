
// adapted from
// https://github.com/dbeyer/sv-benchmarks/blob/master/c/pthread-atomic/gcd_true-unreach-call_true-termination.c

// Copyright (c) 2015 Michael Tautschnig <michael.tautschnig@qmul.ac.uk>
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


/*
VerifyThis ETAPS 2015, Challenge 2

PARALLEL GCD (60 minutes)
=========================

Algorithm description
---------------------

Various parallel GCD algorithms exist. In this challenge, we consider a
simple Euclid-like algorithm with two parallel threads. One thread
subtracts in one direction, the other thread subtracts in the other
direction, and eventually this procedure converges on GCD.


Implementation
--------------

In pseudocode, the algorithm is described as follows:

(
  WHILE a != b DO                                        
      IF a>b THEN a:=a-b ELSE SKIP FI
  OD
||
  WHILE a != b DO                                        
       IF b>a THEN b:=b-a ELSE SKIP FI
  OD
);
OUTPUT a


Verification tasks
------------------

Specify and verify the following behaviour of this parallel GCD algorithm:

Input:  two positive integers a and b
Output: a positive number that is the greatest common divisor of a and b

Feel free to add synchronisation where appropriate, but try to avoid
blocking of the parallel threads.


Sequentialization
-----------------

If your tool does not support reasoning about parallel threads, you may
verify the following pseudocode algorithm:

WHILE a != b DO
    CHOOSE(
         IF a > b THEN a := a - b ELSE SKIP FI,
         IF b > a THEN b := b - a ELSE SKIP FI
    )
OD;
OUTPUT a
*/

#include "../pthread.h"

unsigned a, b;

#if 0
void __VERIFIER_atomic_dec_a()
{
  if(a>b)
    a=a-b;
}

void __VERIFIER_atomic_dec_b()
{
  if(b>a)
    b=b-a;
}
#endif

void* dec_a(void* arg)
{
  unsigned a2, b2;

  a2 = a;
  while (a2 != b)
  {
    // __VERIFIER_atomic_dec_a();
    if (a2 > b)
    {
      b2 = b;
      a = a2 - b2;
    }
    a2 = a;
  }
}

void* dec_b(void* arg)
{
  unsigned a2, b2;

  b2 = b;
  while (a != b2)
  {
    // __VERIFIER_atomic_dec_b();
    if (b2 > a)
    {
      a2 = a;
      b = b2 - a2;
    }
    b2 = b;
  }
}

#if 0
unsigned start(unsigned a_in, unsigned b_in)
{
  a=a_in;
  b=b_in;

  pthread_t t1, t2;

  pthread_create(&t1, 0, dec_a, 0);
  pthread_create(&t2, 0, dec_b, 0);

  pthread_join(t1, 0);
  pthread_join(t2, 0);

  //return a;
}

void check_gcd(unsigned a_in, unsigned b_in, unsigned gcd)
{
  unsigned guessed_gcd=__VERIFIER_nondet_uint();
  __VERIFIER_assume(guessed_gcd>1);
  __VERIFIER_assume(a_in%guessed_gcd==0);
  __VERIFIER_assume(b_in%guessed_gcd==0);

  __VERIFIER_assert(a_in%gcd==0);
  __VERIFIER_assert(b_in%gcd==0);

  __VERIFIER_assert(gcd>=guessed_gcd);
}
#endif

int main()
{
  pthread_t t1, t2;

  a = 12;
  b = 30;

  pthread_create (t1, NULL, dec_a, NULL);
  pthread_create (t2, NULL, dec_b, NULL);

  pthread_join(t1, NULL);
  pthread_join(t2, NULL);

  assert (a == 6);
  assert (b == 6);
}

