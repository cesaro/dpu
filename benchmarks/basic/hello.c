
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

uint16_t g = 123;
long long l = 123;
char buff[128];

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

int main (int argc, char ** argv)
{
   printf ("argc %d\n", argc);
   for (int i = 0; i < argc; ++i)
   {
      printf ("i %d argv '%s'\n", i, argv[i]);
   }

   char * ptr = buff;
   mystrcpy (ptr, "hello world\n");

   return argc + strlen (ptr);
}
