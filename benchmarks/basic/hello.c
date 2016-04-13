
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

int main (int argc, char ** argv)
{
   printf ("argc %d\n", argc);
   for (int i = 0; i < argc; ++i)
   {
      printf ("i %d argv '%s'\n", i, argv[i]);
   }

   char * ptr = malloc  (128);
   strcpy (ptr, "hello world\n");

   return argc + strlen (ptr);
}
