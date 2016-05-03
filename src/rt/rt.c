
#include <stdio.h>
#include <inttypes.h>
#include <ctype.h>

#include "rt/rt.h"

struct rt rt;

static const char *dpu_rt_quote (char c)
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

static void dpu_rt_header ()
{
   printf ("what             addr            value\n");
   printf ("==== ================ ================\n");
}

// memory loads
//
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

// memory stores
//
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

// others
//
void dpu_rt_alloca (uint8_t *addr, uint32_t size)
{
   printf ("allo %16p %16x (stack)\n", addr, size);
}
void dpu_rt_malloc (uint8_t *addr, uint64_t size)
{
   printf ("mllo %16p %16lx (heap)\n", addr, size);
}
void dpu_rt_realloc (uint8_t *old, uint8_t *neww, uint64_t size)
{
   printf ("rllo %16p %16p %16lx (heap)\n", old, neww, size);
}
void dpu_rt_free (uint8_t *addr)
{
   printf ("free %16p    (heap)\n", addr);
}

void dpu_rt_call (uint32_t id)
{
   printf ("call                  %16d\n", id);
}
void dpu_rt_ret (uint32_t id)
{
   printf ("ret                   %16d\n", id);
}

int dpu_rt_main (int argc, char **argv, char **env)
{
   int ret;
	int i, n;
	char **v;

	// determine number of environment variables
	for (n = 0, v = env; *v; ++n, ++v)
	;

   printf ("dpu: rt: %d args, %d env; copying into own stack\n", argc, n);

   // copy arguments into our stack
   int myargc = argc;
   char *myargv[argc];
   char *myenv[n];
   for (i = 0; i < argc; i++) myargv[i] = argv[i];
   for (i = 0; i < n; i++) myenv[i] = env[i];

   // call main
   printf ("dpu: rt: calling main: argc %d argv %p env %p\n", myargc, myargv, myenv);
   dpu_rt_header ();
   ret = main (myargc, myargv, env);

	// exit
   printf ("dpu: rt: main returned %d!!\n", ret);
   return ret;
}

