
#include <vector>

#include "redbox.hh"
#include "memory-region.hh"
#include "memory-pool.hh"
#include "datarace-analysis.hh"

#include "verbosity.h"

namespace dpu {

void DataRaceAnalysis::run ()
{
   report_init ();

   ASSERT (race == nullptr);
   ASSERT (counters.replays == 0);

   PRINT ("dpu: dr: starting data-race detection analysis on %zu executions...",
      replays.size());

   for (const stid::Replay &r : replays)
   {
      Config c (add_one_run (r));
      counters.replays++;
      if (verb_debug) c.dump ();
      race = find_data_races (c);
      // FIXME: add an option to scan all traces instead of stopping on the
      // first one
#if 1
      if (race) break;
#else
      if (race)
      {
         PRINT ("dpu: dr: data race FOUND");
         report.add_defect (*race);
         race->dump ();
      }
#endif
   }
   PRINT ("dpu: dr: finished data-race detection");

   if (race)
   {
      PRINT ("dpu: dr: result: one data race FOUND (but the program may have more)");
      report.add_defect (*race);
      race->dump ();
   }
   else
   {
      PRINT ("dpu: dr: result: NO data race found");
   }
   resources.update();
}

DataRace * DataRaceAnalysis::find_data_races (const Config &c)
{
   int i, j, nrp;
   const Event *e1, *e2;
   MemoryRegion<Addr> region;

   DEBUG ("dpu: dr: find: c %p", &c);

   nrp = c.num_procs ();

   // FIXME: document what's being implemented in this loop!

   for (i = 0; i < nrp - 1; ++i)
   {
      for (e1 = c[i]; e1; e1 = e1->pre_proc())
      {
         // if e1 does not read or write to at least 1 memory location, skip it
         if (e1->dat == nullptr) continue;
         ASSERT (! e1->data<const Redbox>().empty ());
         ASSERT (e1->pid() == i);

         for (j = i + 1; j < nrp; ++j)
         {
            // iterate over the maximal event for process j in [e1] and all its
            // (process) causal successors e2 that are **not** causal successors
            // of e1, except if e1 is the maximal event for process i in [e2]
            for (e2 = c[j]; e2; e2 = e2->pre_proc())
            {
               // if e2 does not read or write to at least 1 memory location,
               // skip it
               if (e2->is_predeq_of (e1) and e2 != e1->cone[j]) break;
               if (e2->dat == nullptr) continue;
               if (e1->is_predeq_of (e2) and e1 != e2->cone[i]) continue;

               // for every pair of events e1, e2 in c whose red boxes
               // represent concurrent statements we have a data race iff:
               // - e1 writes where e2 reads, or
               // - e1 writes where e2 writes, or
               // - e1 reads where e2 writes
               
               const Redbox &b1 = e1->data<const Redbox>();
               const Redbox &b2 = e2->data<const Redbox>();
               if (b1.writepool.overlaps (b2.readpool, region) or
                  b1.writepool.overlaps (b2.writepool, region) or
                  b1.readpool.overlaps (b2.writepool, region))
               {
                  return new DataRace (e1, e2, region.lower, region.size());
               }
            }
         }
      }
   }
   
   return nullptr;
}

void DataRaceAnalysis::report_init ()
{
   // fill the fields stored in the Unfolder base class
   ASSERT (exec);
   std::vector<std::string> myargv (exec->argv.begin(), exec->argv.end());
   std::vector<std::string> myenv (exec->environ.begin(), --(exec->environ.end()));

   report.dpuversion = CONFIG_VERSION;
   report.path = path;
   report.argv = myargv;
   report.environ = myenv;
   report.memsize = exec->config.memsize;
   report.defaultstacksize = exec->config.defaultstacksize;
   report.tracesize = exec->config.tracesize;
   report.optlevel = exec->config.optlevel;

   report.nr_exitnz = 0;
   report.nr_abort = 0;
   report.defects.clear ();

   // FIXME the report contains other fields that irrelevant for this analysis!

   report.alt = 0;
   report.kbound = 0;
}

stid::ExecutorConfig DataRaceAnalysis::prepare_executor_config () const
{
   stid::ExecutorConfig conf;

   conf.memsize = opts::memsize;
   conf.defaultstacksize = opts::stacksize;
   conf.optlevel = opts::optlevel;
   conf.tracesize = CONFIG_GUEST_TRACE_BUFFER_SIZE;

   conf.flags.dosleep = opts::dosleep ? 1 : 0;
   conf.flags.verbose = opts::verbosity >= 3 ? 1 : 0;

   unsigned i = opts::strace ? 1 : 0;
   conf.strace.fs = i;
   conf.strace.pthreads = i;
   conf.strace.malloc = i;
   conf.strace.proc = i;
   conf.strace.others = i;

   conf.do_load_store = true;
   return conf;
}

} // namespace
