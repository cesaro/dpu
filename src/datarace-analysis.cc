
#include <vector>

#include "redbox.hh"
#include "memory-region.hh"
#include "memory-pool.hh"
#include "datarace-analysis.hh"

#include "verbosity.h"

namespace dpu {

void DataRaceAnalysis::run ()
{
   unsigned count;
   report_init ();

   ASSERT (race == nullptr);

   PRINT ("dpu: dr: starting data-race detection analysis...");
   PRINT ("dpu: dr: analyzing %zu replay sequences", replays.size());
   count = 0;
   for (const stid::Replay &r : replays)
   {
      Config c (add_one_run (r));
      count++;
      //c.dump ();
      race = check_data_races (c);
      if (race) break;
   }
   PRINT ("dpu: dr: finished data-race detection, %u of %zu replays analyzed",
      count, replays.size());

   if (race)
   {
      PRINT ("dpu: dr: at least one DATA RACE FOUND (program may have more)");
      report.add_defect (*race);
      race->dump ();
   }
   else
   {
      PRINT ("dpu: dr: NO data race found");
   }
}

DataRace * DataRaceAnalysis::check_data_races (const Config &c)
{
   int i, j, nrp;
   const Event *e1, *e2;
   const Redbox *b1, *b2;
   MemoryRegion<Addr> region;

   nrp = c.num_procs ();

   for (i = 0; i < nrp - 1; ++i)
   {
      for (e1 = c[i]; e1; e1 = e1->pre_proc())
      {
         // if e1 does not read or write to some memory location, skip it
         if (e1->dat == nullptr) continue;

         for (j = i + 1; j < nrp; ++j)
         {
            for (e2 = c[j]; e2; e2 = e2->pre_proc())
            {
               // if e2 does not read or write to some memory location, skip it
               if (e2->dat == nullptr) continue;
               // if e1 and e2 are not concurrent, skip them
               if (! e1->in_con_with (e2)) continue;

               // for every pair e1, e2 of concurrent events in c check if:
               // - e1 writes where e2 reads, or
               // - e1 writes where e2 writes, or
               // - e1 reads where e2 writes
               
               b1 = static_cast<const Redbox*> (e1->dat);
               b2 = static_cast<const Redbox*> (e2->dat);
               if (b1->writepool.overlaps (b2->readpool, region) or
                  b1->writepool.overlaps (b2->writepool, region) or
                  b1->readpool.overlaps (b2->writepool, region))
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
