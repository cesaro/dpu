
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#include "verifier.h"

#ifdef __VERIFIER_error
#undef __VERIFIER_error
#endif

#ifdef __VERIFIER_assume
#undef __VERIFIER_assume
#endif

void __VERIFIER_error_internal (int print)
{
   if (print)
      printf ("dpu: __VERIFIER_error called, calling abort()\n");
   abort ();
}

void __VERIFIER_assume_internal (int expr, int print)
{
   if (expr) return;
   if (print)
   {
      printf ("dpu: __VERIFIER_assume: impossible to assume a false "
            "expression, dpu has no support for assumptions.\n");
      printf ("dpu: calling pthread_exit() instead, you might experience "
            "unexpected errors from this point on.\n");
   }
   pthread_exit (0);
}

void __VERIFIER_error()
{
   __VERIFIER_error_internal (1);
}

void __VERIFIER_assume (int expr)
{
   __VERIFIER_assume_internal (expr, 1);
}

_Bool __VERIFIER_nondet_bool ()
{
   return 0;
}

char __VERIFIER_nondet_char ()
{
   return 0;
}

int __VERIFIER_nondet_int ()
{
   return 0;
}

float __VERIFIER_nondet_float ()
{
   return 0;
}

double __VERIFIER_nondet_double ()
{
   return 0;
}

loff_t __VERIFIER_nondet_loff_t ()
{
   return 0;
}

long __VERIFIER_nondet_long ()
{
   return 0;
}

pchar __VERIFIER_nondet_pchar ()
{
   return 0;
}

pointer __VERIFIER_nondet_pointer ()
{
   return 0;
}

pthread_t __VERIFIER_nondet_pthread_t ()
{
   return 0;
}

sector_t __VERIFIER_nondet_sector_t ()
{
   return 0;
}

short __VERIFIER_nondet_short ()
{
   return 0;
}

size_t __VERIFIER_nondet_size_t ()
{
   return 0;
}

u32 __VERIFIER_nondet_u32 ()
{
   return 0;
}

uchar __VERIFIER_nondet_uchar ()
{
   return 0;
}

uint __VERIFIER_nondet_uint ()
{
   return 0;
}

ulong __VERIFIER_nondet_ulong ()
{
   return 0;
}

unsigned __VERIFIER_nondet_unsigned ()
{
   return 0;
}

ushort __VERIFIER_nondet_ushort ()
{
   return 0;
}

pthread_mutex_t __dpu_global_mutex = PTHREAD_MUTEX_INITIALIZER;

void __VERIFIER_atomic_begin ()
{
   pthread_mutex_lock (&__dpu_global_mutex);
}

void __VERIFIER_atomic_end ()
{
   pthread_mutex_unlock (&__dpu_global_mutex);
}
