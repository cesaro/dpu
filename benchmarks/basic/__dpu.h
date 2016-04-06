
#ifndef __DPU_H_
#define __DPU_H_

/* SVCOMP headers */
extern void __VERIFIER_error() __attribute__ ((__noreturn__));
extern void __VERIFIER_assume (int expr);
extern void __VERIFIER_nondet (int expr);

/*
bool
char
int
float
loff_t
long
pchar
pointer
pthread_t
sector_t
short
size_t
u32
uchar
uint
ulong
unsigned
ushort
*/

extern void __VERIFIER_atomic_begin ();
extern void __VERIFIER_atomic_end ();

/* capture __assert_fail */

#define _ASSERT_H_DECLS // avoids definition of headers in <assert.h>

#define __assert_fail(str,file,line,fun)           __VERIFIER_error ()
#define __assert_perror_fail(errnum,file,line,fun) __VERIFIER_error ()
#define __assert(str,file,line)                    __VERIFIER_error ()

#endif
