
#include <stdio.h>
#include <assert.h>

int main (int argc, char ** argv)
{
   printf ("argc %d\n", argc);
   for (int i = 0; i < argc; ++i)
   {
      printf ("i %d argv '%s'\n", i, argv[i]);
   }
   return argc;
}
