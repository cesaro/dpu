
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "test.hh"

int main (int, char **)
{
   const char *user = getenv ("USER");

   if (user and strcmp (user, "cesar") == 0)
   {
      // for Cesar
      test50 ();
   }
   else
   {
      // for the rest
      test39();
   }
   return 0;
}
