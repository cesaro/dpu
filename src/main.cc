
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "test.hh"
#include "opts.hh"

namespace dpu
{

int main (int argc, char **argv)
{
   const char *user;

   // parse commandline options
   opts::parse (argc, argv);
   opts::dump ();

   // --devel passed on commandline
   if (opts::development)
   {
      user = getenv ("USER");
      if (user and strcmp (user, "cesar") == 0)
      {
         // for Cesar
         test54 ();
      }
      else
      {
         // for the rest
         test54();
      }
      return 0;
   }

   // analysis
   return 0;
}

} // namespace

int main (int argc, char **argv)
{
   return dpu::main (argc, argv);
}
