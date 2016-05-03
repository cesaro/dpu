
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <ctype.h>

uint16_t g = 123;
long long l = 123;
char buff[128];

struct cesar
{
   int  v32_0;
   char v8_1;
   int  v32_2;
};

void mystrcpy (char * dst, const char * src)
{
   while (*src)
   {
      *dst = *src;
      src++;
      dst++;
   }
   *dst = 0;
}

size_t mystrlen (const char * s)
{
   size_t i = 0;
   while (*s++) i++;
   return i;
}

void test1 (int argc, char ** argv)
{
   int x, i, j;
   unsigned char * ptr;
   printf ("test1: argc %d\n", argc);

   x = argc;
   j = 0;
   ptr = (unsigned char *) &x;
   for (i = 0; i < 4; i++)
   {
      j += *ptr++;
   }

   printf ("j %d\n", j);
}

void test2 ()
{
   struct cesar c[10];
   uint64_t x[10];

   x[5] = 23;

   c[2].v32_0 = 123;
   c[2].v8_1 = 123;
   printf ("%d", c[3].v32_2);
}

int main (int argc, char **argv)
{
   int x;
   x -= x; // undefined behaviour !
   printf ("xxxxxxxx %d xxxxxxxxx %p xxxxxxxx\n", x, &x);
   //return 123;

   uint32_t i = 0x10121314;
   uint32_t j = 10;
   uint64_t rbp = 0x123; // current stack frame
   uint64_t rsp = 0x123; // stack pointer

   __asm__ (
      "movl %0, %1\n"
      "movq %%rbp, %2\n"
      "movq %%rsp, %3\n"
      : "+r" (i),
        "+r" (j),
        "=r" (rbp),
        "=r" (rsp));

   printf ("i %0x\n", i);
   printf ("j %0x\n", i);
   printf ("rbp %0lx\n", rbp);
   printf ("rsp %0lx\n", rsp);
   //return 7;


   printf ("argc %d &argc %p\n", argc, &argc);
   printf ("argv %p\n", argv);
   printf ("buff %p\n", buff);
   printf ("i %d &i %p\n", i, &i);
   for (i = 0; i < argc; ++i)
   {
      printf ("argv[%d] %p '%s'\n", i, argv[i], argv[i]);
   }

   printf ("malloc(4) %p\n", malloc (4));
   printf ("malloc(8) %p\n", malloc (8));
   printf ("malloc(16) %p\n", malloc (16));
   printf ("malloc(256) %p\n", malloc (256));

   return 0;

   char * ptr = buff;
   mystrcpy (ptr, "hello\n");

   printf ("calling test\n");
   test1 (argc, argv);
   printf ("test done\n");

   return argc + mystrlen (ptr);
}

