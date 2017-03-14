
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <execinfo.h>
#include <signal.h>

#include "c15u/c15unfold.hh" // must be before verbosity.h
#include "verbosity.h"
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
      //test54 ();
   }
   else
   {
      // for the rest
      //test54();
   }
   exit (0);
}

void handler (int sig) {
   void *array[10];
   size_t size;

   // get void*'s for all entries on the stack
   size = backtrace(array, 10);

   // print out all the frames to stderr
   PRINT ("%s: error: received signal %d (SIGSEGV):\n", opts::progname, sig);
   backtrace_symbols_fd (array, size, 0);
   PRINT ("\n%s: aborting!", opts::progname);
   exit(1);
}

size_t get_precise_memory_size (C15unfolder &unf)
{
   unsigned i;
   size_t size;

   size = 0;
   for (i = 0; i < unf.u.num_procs(); i++)
   {
      size += unf.u.proc(i)->memory_size ();
      size += unf.u.proc(i)->pointed_memory_size (); // slow
   }
   return size;
}

void print_statistics (C15unfolder &unf)
{
   unsigned long events;
   unsigned i;
   size_t size;

   events = 0;
   size = 0;
   for (i = 0; i < unf.u.num_procs(); i++)
   {
      events += unf.u.proc(i)->counters.events;
      size += unf.u.proc(i)->memory_size();
   }

   //PRINT ("\ndpu: unfolding statistics:");
   PRINT ("dpu: stats: unfolding: %lu max-configs", unf.counters.maxconfs);
   PRINT ("dpu: stats: unfolding: %lu threads created", unf.counters.stid_threads);
   PRINT ("dpu: stats: unfolding: %lu process slots used", unf.u.num_procs());
   if (verb_trace)
   {
      size_t size2 = get_precise_memory_size (unf);
      PRINT ("dpu: stats: unfolding: %u%s total memory (%.1f bytes/event)",
         UNITS_SIZE (size2), UNITS_UNIT (size2),
         size2 / (float) events);
   }
   PRINT ("dpu: stats: unfolding: %lu events (aprox. %u%s of memory)",
      events, UNITS_SIZE (size), UNITS_UNIT (size));
   for (i = 0; i < unf.u.num_procs(); i++)
   {
      PRINT ("dpu: stats: unfolding: t%u: %lu events (%u%s, %.1f%%)",
            i, unf.u.proc(i)->counters.events,
            UNITS_SIZE(unf.u.proc(i)->memory_size()),
            UNITS_UNIT(unf.u.proc(i)->memory_size()),
            (100.0 * unf.u.proc(i)->memory_size()) / size);
   }

   //PRINT ("\ndpu: POR statistics:");
   PRINT ("dpu: stats: por: %lu executions", unf.counters.runs);
   PRINT ("dpu: stats: por: %lu SSBs", unf.counters.ssbs);
   PRINT ("dpu: stats: por: %.1f average max trail size",
         unf.counters.avg_max_trail_size);
   PRINT ("dpu: stats: por: %lu calls to Alt(C,D) -- after trivial simplification", unf.counters.altcalls);
   PRINT ("dpu: stats: por: %lu largest |D| (unjust. only) on call to Alt(C,D) -- after trivial simplification",
         unf.counters.max_unjust_when_alt_call);
   PRINT ("dpu: stats: por: %.2f average |D| (unjust. only) on call to Alt(C,D) -- after trivial simplification",
         unf.counters.avg_unjust_when_alt_call);

   PRINT ("\ndpu: summary: %lu max-configs, %lu SSBs, %lu events, %.1f ev/trail",
         unf.counters.maxconfs, unf.counters.ssbs, events,
         unf.counters.avg_max_trail_size);
}

int main (int argc, char **argv)
{
   unsigned i;

   // install signal handler for segfaults
   //signal (SIGSEGV, handler);

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
      C15unfolder unf (opts::alt_algo, opts::kbound);

      // load code and set argv
      PRINT ("dpu: loading bytecode");
      unf.load_bytecode (std::string (opts::inpath));
      INFO ("dpu: setting commandline arguments:");
      INFO ("dpu: argc = %zu", opts::argv.size());
      INFO_ ("dpu: argv = [");
      for (i = 0; i < opts::argv.size(); i++)
         INFO_ ("\"%s\"%s", opts::argv[i], i + 1 < opts::argv.size() ? ", " : "");
      INFO ("]");

      unf.set_args (opts::argv);

      switch (opts::alt_algo) {
      case C15unfolder::Alt_algorithm::KPARTIAL :
         PRINT ("dpu: using '%u-partial' alternatives", opts::kbound);
         break;
      case C15unfolder::Alt_algorithm::OPTIMAL :
         PRINT ("dpu: using 'optimal' alternatives");
         break;
      case C15unfolder::Alt_algorithm::ONLYLAST :
         PRINT ("dpu: using 'only-last' (1-partial) alternatives");
         break;
      case C15unfolder::Alt_algorithm::SDPOR :
         PRINT ("dpu: using 'sdpor' alternatives");
         break;
      }

      // build entire unfolding
      PRINT ("dpu: starting unfolding construction");
      unf.explore ();
      PRINT ("dpu: finished unfolding construction");

      // print dot
      if (opts::dotpath.size())
      {
         PRINT ("dpu: dumping unfolding to '%s'", opts::dotpath.c_str());
         std::ofstream f (opts::dotpath.c_str());
         unf.u.print_dot (f);
         f.close ();
      }

      // dump unfolding to stdout
      if (verb_debug) unf.u.dump ();

      // report statistics
      print_statistics (unf);
      fflush (stdout);
      fflush (stderr);

   } catch (const std::exception &e) {
      PRINT ("%s: error: unhandled exception: %s", opts::progname, e.what ());
      PRINT ("%s: aborting!", opts::progname);
      fflush (stdout);
      fflush (stderr);
      return 1;
   }

   return 0;
}

} // namespace

int main (int argc, char **argv)
{
   return dpu::main (argc, argv);
}
