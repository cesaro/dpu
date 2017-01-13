
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "c15u/c15unfold.hh" // must be before verbosity.h
#include "verbosity.h"
#include "test.hh"
#include "opts.hh"

namespace dpu
{

void devel_hook ()
{
   const char *user;
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
   exit (0);
}

int main (int argc, char **argv)
{
   // parse commandline options
   opts::parse (argc, argv);
   opts::dump ();

   // --devel passed on commandline
   if (opts::development)
   {
      devel_hook ();
   }

   // analysis
   try
   {
      C15unfolder unf;

      // load code and set argv
      unf.load_bytecode (std::string (opts::inpath));
      unf.set_args (opts::argv);

      // build entire unfolding
      unf.explore ();

      // print dot
      unf.u.dump ();
      std::ofstream f ("/tmp/unf.dot");
      unf.u.print_dot (f);
      f.close ();

   } catch (const std::exception &e) {
      PRINT ("%s: error: unhandled exception: %s", opts::progname, e.what ());
      PRINT ("%s: aborting!", opts::progname);
      return 1;
   }

   return 0;
}

} // namespace

int main (int argc, char **argv)
{
   return dpu::main (argc, argv);
}
