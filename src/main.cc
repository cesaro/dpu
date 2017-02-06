
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

void print_statistics (C15unfolder &unf)
{
   unsigned long events;
   unsigned i;

   events = 0;
   for (i = 0; i < unf.u.num_procs(); i++)
      events += unf.u.proc(i)->counters.events;

   //PRINT ("\ndpu: unfolding statistics:");
   PRINT ("dpu: stats: unfolding: %lu max-configs", unf.counters.maxconfs);
   PRINT ("dpu: stats: unfolding: %lu threads created", unf.u.num_procs());
   PRINT ("dpu: stats: unfolding: %lu events", events);
   for (i = 0; i < unf.u.num_procs(); i++)
   {
      PRINT ("dpu: stats: unfolding: thread%u: %lu events",
            i, unf.u.proc(i)->counters.events);
   }

   //PRINT ("\ndpu: POR statistics:");
   PRINT ("dpu: stats: por: %lu executions", unf.counters.runs);
   PRINT ("dpu: stats: por: %lu SSBs", unf.counters.ssbs);
   PRINT ("dpu: stats: por: %.2f average max trail size", unf.counters.avg_max_trail_size);

   PRINT ("\ndpu: summary: %lu max-configs, %lu SSBs, %lu events",
      unf.counters.maxconfs, unf.counters.ssbs, events);
}

int main (int argc, char **argv)
{
   // parse commandline options
   opts::parse (argc, argv);
   if (verb_debug) opts::dump ();

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
      PRINT ("dpu: loading bytecode");
      unf.load_bytecode (std::string (opts::inpath));
      PRINT ("dpu: setting argv");
      unf.set_args (opts::argv);

      // build entire unfolding
      PRINT ("dpu: starting unfolding construction");
      unf.explore ();
      PRINT ("dpu: finished unfolding construction");

      // print dot
      PRINT ("dpu: dumping unfolding to /tmp/unf.dot");
      std::ofstream f ("/tmp/unf.dot");
      unf.u.print_dot (f);
      f.close ();

      // dump unfolding to stdout
      if (verb_debug) unf.u.dump ();

      // report statistics
      print_statistics (unf);
      fflush (stdout);
      fflush (stderr);

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
