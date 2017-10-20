
#ifndef __DPU_H__
#define __DPU_H__

// SVCOMP'17 headers, as defined in
// https://sv-comp.sosy-lab.org/2017/rules.php

#include <sys/types.h> // defines loff_t
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef char * pchar;
typedef void * pointer;

// definition of missing types if <linux/types.h> has not been previously included
#ifndef _LINUX_TYPES_H
#define _LINUX_TYPES_H
typedef uint64_t sector_t;
typedef uint32_t u32;
typedef unsigned char uchar;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned short ushort;
#endif

#define __VERIFIER_error() \
   do { \
      printf (__FILE__ ":%d: %s: __VERIFIER_error called, calling abort()\n", \
            __LINE__, __func__); \
      __VERIFIER_error_internal (0); \
   } while (0)

#define __VERIFIER_assume(expr) \
   do { \
      if ((long) (expr)) break; \
      printf (__FILE__ ":%d: %s: __VERIFIER_assume: " \
            "Impossible to assume `" #expr \
            "', dpu has no support for assumptions.\n", \
            __LINE__, __func__); \
      printf (__FILE__ ":%d: %s: __VERIFIER_assume: " \
            "Calling pthread_exit() instead, you might experience unexpected " \
            "errors from this point on.\n", \
            __LINE__, __func__); \
      __VERIFIER_assume_internal (expr, 0); \
   } while (0)

_Bool __VERIFIER_nondet_bool ();
char __VERIFIER_nondet_char ();
int __VERIFIER_nondet_int ();
float __VERIFIER_nondet_float ();
double __VERIFIER_nondet_double ();
loff_t __VERIFIER_nondet_loff_t ();
long __VERIFIER_nondet_long ();
pchar __VERIFIER_nondet_pchar ();
pointer __VERIFIER_nondet_pointer ();
pthread_t __VERIFIER_nondet_pthread_t ();
sector_t __VERIFIER_nondet_sector_t ();
short __VERIFIER_nondet_short ();
size_t __VERIFIER_nondet_size_t ();
u32 __VERIFIER_nondet_u32 ();
uchar __VERIFIER_nondet_uchar ();
uint __VERIFIER_nondet_uint ();
ulong __VERIFIER_nondet_ulong ();
unsigned __VERIFIER_nondet_unsigned ();
ushort __VERIFIER_nondet_ushort ();

void __VERIFIER_atomic_begin ();
void __VERIFIER_atomic_end ();

#endif
