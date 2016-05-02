
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <ctype.h>

void dpu_rt_header ()
{
   printf ("what             addr            value\n");
   printf ("==== ================ ================\n");
}

const char *dpu_rt_quote (char c)
{
   static char str[5];

   if (c == '\n') return "\\n";
   if (c == '\0') return "\\0";
   if (c == '\r') return "\\r";
   if (c == '\t') return "\\t";
   if (isprint (c))
   {
      str[0] = c;
      str[1] = 0;
      return str;
   }
   sprintf (str, "\\x%02x", c);
   return str;
}

void dpu_rt_load8 (uint8_t *addr, uint8_t v)
{
   printf ("ld8  %16p %16x '%s'\n", addr, v, dpu_rt_quote (v));
}
void dpu_rt_load16 (uint16_t *addr, uint16_t v)
{
   printf ("ld16 %16p %16"PRIx16"\n", addr, v);
}
void dpu_rt_load32 (uint32_t *addr, uint32_t v)
{
   printf ("ld32 %16p %16"PRIx32"\n", addr, v);
}
void dpu_rt_load64 (uint64_t *addr, uint64_t v)
{
   printf ("ld64 %16p %16"PRIx64"\n", addr, v);
}

void dpu_rt_store8 (uint8_t *addr, uint8_t v)
{
   printf ("st8  %16p %16x '%s'\n", addr, v, dpu_rt_quote (v));
}
void dpu_rt_store16 (uint16_t *addr, uint16_t v)
{
   printf ("st16 %16p %16"PRIx16"\n", addr, v);
}
void dpu_rt_store32 (uint32_t *addr, uint32_t v)
{
   printf ("st32 %16p %16"PRIx32"\n", addr, v);
}
void dpu_rt_store64 (uint64_t *addr, uint64_t v)
{
   printf ("st64 %16p %16"PRIx64"\n", addr, v);
}

void dpu_rt_alloca (uint8_t *addr, uint32_t size)
{
   printf ("allo %16p %16x (stack)\n", addr, size);
}
void dpu_rt_malloc (uint8_t *addr, uint32_t size)
{
   printf ("mllo %16p %16x (heap)\n", addr, size);
}
void dpu_rt_ret (uint32_t id)
{
   printf ("ret                   %16d\n", id);
}

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
   x -= x;
   printf ("xxxxxxxx %d xxxxxxxxx %p xxxxxxxx\n", x, &x);
   return 123;

   
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
   return 7;


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

//int __user_main (int argc, char **argv);

int dpu_rt_start (int argc, char **argv)
{
   // instrumented main coming from the rt
   int ret;

   // copy arguments into the stack
   int myargc = argc;
   char *myargv[argc];
   for (int i = 0; i < argc; i++) myargv[i] = argv[i];

   // call main
   printf ("rt: calling main...\n");
   dpu_rt_header ();
   ret = main (myargc, myargv);
   printf ("================\n");
   printf ("rt: main returned %d...\n", ret);
   return ret;
}

