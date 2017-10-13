
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <execinfo.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

#include "c15u/c15unfold.hh" // must be before verbosity.h
#include "verbosity.h"
#include "opts.hh"
#include "probdist.hh"
#include "resources.hh"

namespace dpu
{

void devel_hook ()
{
   // put here your code :)
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

void print_tree_stats (C15unfolder &unf)
{
   // type of event (create, join, exit, lock, unlock...)
   Probdist<ActionType> et;
   // depths of process trees
   Probdist<unsigned> *pd = new Probdist<unsigned>[unf.u.num_procs()];
   // branching factor of process trees
   Probdist<unsigned> *pb = new Probdist<unsigned>[unf.u.num_procs()];
   // depths of address trees
   std::map<uint64_t,Probdist<unsigned>> ad;
   // branching factor of address trees
   std::map<uint64_t,Probdist<unsigned>> ab;
   unsigned i;

   // collect the data
   for (i = 0; i < unf.u.num_procs(); i++)
   {
      for (Event &e : *unf.u.proc(i))
      {
         et.sample (e.action.type);
         pd[i].sample (e.depth_proc());
         pb[i].sample (e.node[0].post.size());
         if (e.action.type == ActionType::MTXLOCK
               or e.action.type == ActionType::MTXUNLK)
         {
            decltype(ad)::iterator it1 = ad.find (e.action.addr);
            decltype(ab)::iterator it2 = ab.find (e.action.addr);
            if (it1 == ad.end ())
            {
               it1 = ad.emplace(std::make_pair(e.action.addr,Probdist<unsigned>())).first;
               it2 = ab.emplace(std::make_pair(e.action.addr,Probdist<unsigned>())).first;
            }
            it1->second.sample (e.depth_other());
            it2->second.sample (e.node[1].post.size());
         }
      }
   }
   ASSERT (ad.size() == ab.size());

   // trees
   for (i = 0; i < unf.u.num_procs(); i++)
   {
      PRINT ("dpu: stats: trees: depths: t%u: min/max/avg=%s {depth=count/mass}={%s}",
         i,
         pd[i].summary_mma().c_str(),
         pd[i].summary_freq_maxc(4).c_str());
   }
   for (auto &kv : ad)
   {
      PRINT ("dpu: stats: trees: depths: %p: min/max/avg=%s {depth=count/mass}={%s}",
         (void*) kv.first,
         kv.second.summary_mma().c_str(),
         kv.second.summary_freq_maxc(4).c_str());
   }
   for (i = 0; i < unf.u.num_procs(); i++)
   {
      PRINT ("dpu: stats: trees: branch-out: t%u: size/nc=%s; "
         "min/max/avg=%s; {factor=count/mass}={%s}",
         i,
         pb[i].summary_snc().c_str(),
         pb[i].summary_mma().c_str(),
         pb[i].summary_freq_maxc(4).c_str());
   }
   for (auto &kv : ab)
   {
      PRINT ("dpu: stats: trees: branch-out: %p: size/nc=%s; "
         "min/max/avg=%s; {factor=count/mass}={%s}",
         (void*) kv.first,
         kv.second.summary_snc().c_str(),
         kv.second.summary_mma().c_str(),
         kv.second.summary_freq_maxc(4).c_str());
   }

   // events
   PRINT ("dpu: stats: events: pthread_create: %u (%.1f%%)",
      et.count (ActionType::THCREAT),
      100 * et.mass (ActionType::THCREAT));
   PRINT ("dpu: stats: events: pthread_join: %u (%.1f%%)",
      et.count (ActionType::THJOIN),
      100 * et.mass (ActionType::THJOIN));
   PRINT ("dpu: stats: events: pthread_mutex_lock: %u (%.1f%%)",
      et.count (ActionType::MTXLOCK),
      100 * et.mass (ActionType::MTXLOCK));
   PRINT ("dpu: stats: events: pthread_mutex_unlock: %u (%.1f%%)",
      et.count (ActionType::MTXUNLK),
      100 * et.mass (ActionType::MTXUNLK));
   PRINT ("dpu: stats: events: (thread start): %u (%.1f%%)",
      et.count (ActionType::THSTART),
      100 * et.mass (ActionType::THSTART));
   PRINT ("dpu: stats: events: pthread_exit: %u (%.1f%%)",
      et.count (ActionType::THEXIT),
      100 * et.mass (ActionType::THEXIT));

   unsigned rest =
      et.count (ActionType::THCREAT) +
      et.count (ActionType::THJOIN) +
      et.count (ActionType::MTXLOCK) +
      et.count (ActionType::MTXUNLK) +
      et.count (ActionType::THSTART) +
      et.count (ActionType::THEXIT);
   if (rest != et.size())
   {
      PRINT ("dpu: stats: events: others: %lu (%.1f%%)",
         et.size() - rest,
         100 * (et.size() - rest) / (float) et.size());
   }

   delete[] pd;
   delete[] pb;
}

#ifdef CONFIG_STATS_DETAILED
void print_causality_stats (C15unfolder &unf)
{
   long unsigned calls, trivial, nontrivial, null, eq, invdep, empty;

   // causality
   calls   = Event::counters.causality.calls;
   null    = Event::counters.causality.trivial_null;
   eq      = Event::counters.causality.trivial_eq;
   invdep  = Event::counters.causality.trivial_invdep;
   trivial = null + eq + invdep;
   nontrivial = calls - trivial;

   //SHOW (Event::counters.causality.calls, "lu");
   //SHOW (Event::counters.causality.trivial_null, "lu");
   //SHOW (Event::counters.causality.trivial_eq, "lu");
   //SHOW (Event::counters.causality.trivial_invdep, "lu");
   //SHOW (trivial, "lu");

   PRINT ("dpu: stats: <: on events:");
   PRINT ("dpu: stats: <:   %lu calls", calls);
   PRINT ("dpu: stats: <:   %lu trivial (%.1f%%), solved by null/eq/invdep %lu/%lu/%lu checks",
         trivial,
         calls ?  trivial * 100.0 / calls : 0.0,
         null, eq, invdep);
   PRINT ("dpu: stats: <:   %lu non-trivial (%.1f%%)",
      nontrivial,
      calls ?  nontrivial * 100.0 / calls : 0.0);

   PRINT ("dpu: stats: <: on sequential trees:");
   PRINT ("dpu: stats: <:   max depth: size/nc=%s; "
         "min/max/avg=%s; {depth=count/mass}={%s}",
         Event::counters.causality.depth.summary_snc().c_str(),
         Event::counters.causality.depth.summary_mma().c_str(),
         Event::counters.causality.depth.summary_freq_maxc(5).c_str());
   PRINT ("dpu: stats: <:   depth diff: size/nc=%s; "
         "min/max/avg=%s; {diff=count/mass}={%s}",
         Event::counters.causality.diff.summary_snc().c_str(),
         Event::counters.causality.diff.summary_mma().c_str(),
         Event::counters.causality.diff.summary_freq_maxc(0).c_str());

   // conflict
   calls      = Event::counters.conflict.calls_event;
   eq         = Event::counters.conflict.trivial_eq;
   empty      = Event::counters.conflict.trivial_empty;
   trivial    = eq + empty;
   nontrivial = calls - trivial;

   PRINT ("dpu: stats: #: on events (e#e):");
   PRINT ("dpu: stats: #:   %lu calls", calls);
   PRINT ("dpu: stats: #:   %lu trivial (%.1f%%), solved by eq/empty %lu/%lu checks",
         trivial,
         calls ? trivial * 100.0 / calls : 0.0,
         eq, empty);
   PRINT ("dpu: stats: #:   %lu non-trivial (%.1f%%)",
      nontrivial, calls ? nontrivial * 100.0 / calls : 0.0);

   PRINT ("dpu: stats: #: on event-config (e#C):");
   PRINT ("dpu: stats: #:   %lu calls, all non-trivial",
         Event::counters.conflict.calls_conf);

   // FIXME -- we need to use Node<Event,X> where X is the skip step that we
   // have used to define the class Event, otherwise won't get the right
   // statistics!!
   unsigned constexpr ss = CONFIG_SKIP_STEP;
   calls      = Node<Event,ss>::nodecounters.conflict.calls;
   trivial    = Node<Event,ss>::nodecounters.conflict.trivial_eq;
   nontrivial = calls - trivial;
   PRINT ("dpu: stats: #: on sequential trees:");
   PRINT ("dpu: stats: #:   %lu calls", calls);
   PRINT ("dpu: stats: #:   %lu trivial (%.1f%%), solved by eq (depth) checks",
         trivial, calls ? trivial * 100.0 / calls : 0.0);
   PRINT ("dpu: stats: #:   %lu non-trivial (%.1f%%)",
      nontrivial, calls ? nontrivial * 100.0 / calls : 0.0);
   PRINT ("dpu: stats: #:   max depth: size/nc=%s; "
         "min/max/avg=%s; {depth=count/mass}={%s}",
         Node<Event,ss>::nodecounters.conflict.depth.summary_snc().c_str(),
         Node<Event,ss>::nodecounters.conflict.depth.summary_mma().c_str(),
         Node<Event,ss>::nodecounters.conflict.depth.summary_freq_maxc(5).c_str());
   PRINT ("dpu: stats: #:   depth diff: size/nc=%s; "
         "min/max/avg=%s; {diff=count/mass}={%s}",
         Node<Event,ss>::nodecounters.conflict.diff.summary_snc().c_str(),
         Node<Event,ss>::nodecounters.conflict.diff.summary_mma().c_str(),
         Node<Event,ss>::nodecounters.conflict.diff.summary_freq_maxc(0).c_str());
   PRINT ("dpu: stats: </#: steps: size/nc=%s; "
         "min/max/avg=%s; {steps=count/mass}={%s}",
         Node<Event,ss>::nodecounters.conflict.steps.summary_snc().c_str(),
         Node<Event,ss>::nodecounters.conflict.steps.summary_mma().c_str(),
         Node<Event,ss>::nodecounters.conflict.steps.summary_freq_maxc(6).c_str());
}
#endif

void print_res_stats (C15unfolder &unf, Resources &res, unsigned long events)
{
   unsigned long min;
   float sec;

   // sample time and memory usage
   res.update ();

   PRINT_ ("dpu: stats: resources: %.3f s wall time", res.walltime / 1000000.0);
   if (res.walltime < 10000) // 10 ms
   {
      PRINT (" (%.3f ms)", res.walltime / 1000.0);
   }
   else if (res.walltime >= 60000000) // 1 min
   {
      min = res.walltime / 60000000;
      sec = (res.walltime - min * 60000000) / 1000000.0;
      PRINT (" (%lumin %.3fsec)", min, sec);
   }
   else
   {
      PRINT ("");
   }

   PRINT ("dpu: stats: resources: %.3f s cpu time", res.cputime / 1000000.0);
   PRINT ("dpu: stats: resources: %luM max RSS", res.maxrss / 1024);

   // performance statistics
   sec = res.walltime / 1000000.0;
   PRINT ("dpu: stats: perf: %ld executions/sec",
         (long) (unf.counters.runs / sec));
   if (unf.counters.ssbs)
      PRINT ("dpu: stats: perf: %ld max-confs/sec",
            (long) (unf.counters.maxconfs / sec));
   PRINT ("dpu: stats: perf: %ld ev/sec", (long) (events / sec));
}

void print_stats (C15unfolder &unf, Resources &res)
{
   unsigned long events, calls, built, expl;
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
   PRINT ("dpu: stats: unfolding: %u process slots used", unf.u.num_procs());
   PRINT ("dpu: stats: unfolding: %lu events (aprox. %zu%s of memory)",
      events, UNITS_SIZE (size), UNITS_UNIT (size));
   for (i = 0; i < unf.u.num_procs(); i++)
   {
      PRINT ("dpu: stats: unfolding: t%u: %u events (%zu%s, %.1f%%)",
            i, unf.u.proc(i)->counters.events,
            UNITS_SIZE(unf.u.proc(i)->memory_size()),
            UNITS_UNIT(unf.u.proc(i)->memory_size()),
            (100.0 * unf.u.proc(i)->memory_size()) / size);
   }
   if (verb_info)
   {
      size_t size2 = get_precise_memory_size (unf);
      PRINT ("dpu: stats: unfolding: %zu%s total allocated memory (%.1f bytes/event)",
         UNITS_SIZE (size2), UNITS_UNIT (size2),
         size2 / (float) events);
   }

   if (verb_info) print_tree_stats (unf);
#ifdef CONFIG_STATS_DETAILED
   if (verb_info) print_causality_stats (unf);
#endif

   // POR statistics
   PRINT ("dpu: stats: por: %lu executions", unf.counters.runs);
   PRINT ("dpu: stats: por: %lu SSBs", unf.counters.ssbs);
   PRINT ("dpu: stats: por: %.1f average ev/trail",
         unf.counters.avg_max_trail_size);

   // alternatives
   calls = unf.counters.alt.calls;
   built = unf.counters.alt.calls_built_comb;
   expl = unf.counters.alt.calls_explore_comb;
   PRINT ("dpu: stats: por: alt: %lu calls", calls);
   PRINT ("dpu: stats: por: alt: %lu calls built a comb (%.1f%%)",
         built, calls ? built * 100.0 / calls : 0.0);
   PRINT ("dpu: stats: por: alt: %lu calls explored a comb (%.1f%%)",
         expl, calls ? expl * 100.0 / calls : 0.0);
   PRINT ("dpu: stats: por: |comb|: size/nc=%s; "
         "min/max/avg=%s; {size=count/mass}={%s}",
         unf.counters.alt.spikes.summary_snc().c_str(),
         unf.counters.alt.spikes.summary_mma().c_str(),
         unf.counters.alt.spikes.summary_freq_maxc(3).c_str());
#ifdef CONFIG_STATS_DETAILED
   PRINT ("dpu: stats: por: |comb.spike| (unfilt): size/nc=%s; "
         "min/max/avg=%s; {size=count/mass}={%s}",
         unf.counters.alt.spikesizeunfilt.summary_snc().c_str(),
         unf.counters.alt.spikesizeunfilt.summary_mma().c_str(),
         unf.counters.alt.spikesizeunfilt.summary_freq_maxc(6).c_str());
   PRINT ("dpu: stats: por: |comb.spike| (filtrd): size/nc=%s; "
         "min/max/avg=%s; {size=count/mass}={%s}",
         unf.counters.alt.spikesizefilt.summary_snc().c_str(),
         unf.counters.alt.spikesizefilt.summary_mma().c_str(),
         unf.counters.alt.spikesizefilt.summary_freq_maxc(6).c_str());
#endif

   // print times here as well!
   print_res_stats (unf, res, events);

   PRINT ("\ndpu: summary: "
         "%zu defects, %lu max-configs, %lu SSBs, %lu events, %.3f sec, %luM%s",
         unf.report.defects.size(),
         unf.counters.maxconfs, unf.counters.ssbs, events,
         res.walltime / 1000000.0,
         res.maxrss / 1024,
         unf.counters.timeout ? " (timeout)" : "");
}

int main (int argc, char **argv)
{
   unsigned i;
   Resources res;

   breakme ();

   // install signal handler for segfaults
   //signal (SIGSEGV, handler);

   // parse commandline options
   opts::parse (argc, argv);
   if (verb_debug) opts::dump ();
   PRINT ("dpu %s running, pid %u", CONFIG_VERSION, getpid());

   if (opts::alt_algo != Altalgo::OPTIMAL and
         opts::maxcts != UINT_MAX)
   {
      PRINT ("%s: error: limiting the number of context swiches requires -a0", opts::progname);
      return 1;
   }

   // analysis
   try
   {
      C15unfolder unf (opts::alt_algo, opts::kbound, opts::maxcts);

      // load code and set argv
      unf.load_bytecode (std::string (opts::inpath));

      // set up argv
      INFO ("dpu: setting commandline arguments:");
      INFO ("dpu: argc = %zu", opts::argv.size());
      INFO_ ("dpu: argv = [");
      for (i = 0; i < opts::argv.size(); i++)
         INFO_ ("\"%s\"%s", opts::argv[i], i + 1 < opts::argv.size() ? ", " : "");
      INFO ("]");
      unf.set_args (opts::argv);

      // set up environ(7)
      INFO ("dpu: set up environment variables:");
      unf.set_default_environment();
      INFO ("dpu: |environ| = %zu", unf.exec->environ.size());

      switch (opts::alt_algo) {
      case Altalgo::KPARTIAL :
         PRINT ("dpu: using '%u-partial' alternatives", opts::kbound);
         break;
      case Altalgo::OPTIMAL :
         PRINT ("dpu: using 'optimal' alternatives");
         break;
      case Altalgo::ONLYLAST :
         PRINT ("dpu: using 'only-last' (1-partial) alternatives");
         break;
      case Altalgo::SDPOR :
         PRINT ("dpu: using 'sdpor' alternatives");
         break;
      }

      // build entire unfolding
      PRINT ("dpu: starting unfolding construction");
      unf.explore ();
      if (unf.counters.timeout)
         PRINT ("dpu: TIMEOUT! stopped unfolding construction");
      else
         PRINT ("dpu: finished unfolding construction");

      // print dot
      if (opts::dotpath.size())
      {
         PRINT ("dpu: dumping unfolding to '%s'", opts::dotpath.c_str());
         std::ofstream f (opts::dotpath.c_str());
         unf.u.print_dot (f);
         f.close ();
      }

      // save the defects report if we got defects
      if (unf.report.defects.size())
      {
         PRINT ("dpu: saving defects report to '%s'", opts::defectspath.c_str());
         unf.report.save (opts::defectspath.c_str());
      }

      // dump unfolding to stdout
      if (verb_debug) unf.u.dump ();

      // report statistics
      print_stats (unf, res);
      llvm::outs().flush();
      llvm::errs().flush();
      fflush (stdout);
      fflush (stderr);

   } catch (const std::exception &e) {
      PRINT ("%s: error: unhandled exception: %s", opts::progname, e.what ());
      PRINT ("%s: aborting!", opts::progname);
      llvm::outs().flush();
      llvm::errs().flush();
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
