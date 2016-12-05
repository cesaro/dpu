
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
      test32 ();
      //test30 ();
   }
   else
   {
      // for the rest
      test30();
   }
   return 0;
}
