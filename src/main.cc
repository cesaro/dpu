
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
      test35 ();
   }
   else
   {
      // for the rest
      test33();
   }
   return 0;
}
