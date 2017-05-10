
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include <time.h>
#include <unistd.h>

#include <cstdint>
#include <cstring>
#include <stdlib.h>
#include <cstdio>
#include <string>
#include <algorithm>

#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/TargetSelect.h"
#include <llvm/Support/YAMLTraits.h>
#include <llvm/Support/Format.h>

#include "stid/executor.hh"

#include "c15unfold.hh" // must be before verbosity.h
#include "misc.hh"
#include "verbosity.h"
#include "pes/process.hh"
#include "opts.hh"

namespace dpu
{

static void _ir_write_ll (const llvm::Module *m, const char *filename)
{
   int fd = open (filename, O_WRONLY | O_TRUNC | O_CREAT, 0644);
   ASSERT (fd >= 0);
   llvm::raw_fd_ostream f (fd, true);
   f << *m;
}


C15unfolder::C15unfolder (Altalgo a, unsigned kbound, unsigned maxcts) :
   m (nullptr),
   exec (nullptr),
   altalgo (a),
   kpartial_bound (kbound),
   comb (a, kbound),
   max_context_switches (maxcts),
   pidpool (u)
{
   unsigned i;

   if (altalgo == Altalgo::OPTIMAL)
      kpartial_bound = UINT_MAX;

   if (altalgo != Altalgo::OPTIMAL and maxcts != UINT_MAX)
      throw std::invalid_argument ("Limiting the number of context switches "
            "is only possible with optimal alternatives");

   // initialize the start array (this is an invariant expected and maintained
   // by stream_to_events)
   for (i = 0; i < Unfolding::MAX_PROC; i++) start[i] = nullptr;
}

C15unfolder::~C15unfolder ()
{
   DEBUG ("c15u.dtor: this %p", this);
   delete exec;
}

void C15unfolder::load_bytecode (std::string &&filepath)
{
   llvm::SMDiagnostic err;
   std::string errors;

   ASSERT (filepath.size());
   ASSERT (path.size() == 0);
   ASSERT (exec == 0);
   ASSERT (m == 0);
   path = std::move (filepath);

   // necessary for the JIT engine; we should move this elsewhere
   static bool init = false;
   if (not init)
   {
      init = true;
      llvm::InitializeNativeTarget();
      llvm::InitializeNativeTargetAsmPrinter();
      llvm::InitializeNativeTargetAsmParser();
   }

   // get a context
   llvm::LLVMContext &context = llvm::getGlobalContext();

   // parse the .ll file and get a Module out of it
   PRINT ("dpu: loading bytecode...");
   std::unique_ptr<llvm::Module> mod (llvm::parseIRFile (path, err, context));
   m = mod.get();

   // if errors found, report and terminate
   if (! mod.get ()) {
      llvm::raw_string_ostream os (errors);
      err.print (path.c_str(), os);
      os.flush ();
      DEBUG ("c51u: load-bytecode: '%s': %s\n", path.c_str(), errors.c_str());
      throw std::invalid_argument (errors);
   }

   // print external symbols
   if (verb_trace) print_external_syms ("dpu: ");

   // prepare an Executor, the constructor instruments and allocates guest
   // memory
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

   PRINT ("dpu: O%u-optimization + jitting...", opts::optlevel);
   DEBUG ("c15u: load-bytecode: setting up the bytecode executor...");
   try {
      exec = new stid::Executor (std::move (mod), conf);
   } catch (const std::exception &e) {
      DEBUG ("c15u: load-bytecode: errors preparing the bytecode executor");
      DEBUG ("c15u: load-bytecode: %s", e.what());
      throw e;
   }
   DEBUG ("c15u: load-bytecode: executor successfully created!");

   if (opts::instpath.size())
   {
      TRACE ("c15u: load-bytecode: saving instrumented bytecode to %s", opts::instpath.c_str());
      _ir_write_ll (m, opts::instpath.c_str());
   }

   DEBUG ("c15u: load-bytecode: done!");
}

void C15unfolder::print_external_syms (const char *prefix)
{
   std::string str;
   llvm::raw_string_ostream os (str);
   std::vector<std::pair<llvm::StringRef,llvm::Type*>> funs;
   std::vector<std::pair<llvm::StringRef,llvm::Type*>> globs;
   size_t len;

   ASSERT (m);

   // functions
   len = 0;
   for (llvm::Function &f : m->functions())
   {
      if (not f.isDeclaration()) continue;
      funs.emplace_back (f.getName(), f.getType());
      if (len < funs.back().first.size())
         len = funs.back().first.size();
   }
   std::sort (funs.begin(), funs.end());
   os << prefix << "External functions:\n";
   for (auto &pair : funs)
   {
      os << prefix << llvm::format ("  %-*s ", len, pair.first);
      os << *pair.second << "\n";
   }


   // global variables
   len = 0;
   for (llvm::GlobalVariable &g : m->globals())
   {
      if (not g.isDeclaration()) continue;
      globs.emplace_back (g.getName(), g.getType());
      if (len < globs.back().first.size())
         len = globs.back().first.size();
   }
   std::sort (globs.begin(), globs.end());
   os << prefix << "External variables:\n";
   for (auto &pair : globs)
   {
      os << prefix << llvm::format ("  %-*s ", len, pair.first);
      os << *pair.second << "\n";
   }
   os.flush ();
   PRINT ("%s", str.c_str());
}

void C15unfolder::set_args (std::vector<const char *> argv)
{
   DEBUG ("c15u: set-args: |argv| %zu", argv.size());
   exec->argv = argv;
}

void C15unfolder::set_env (std::vector<const char *> env)
{
   if (env.empty() or env.back() != nullptr)
      env.push_back (nullptr);
   DEBUG ("c15u: set-env: |env| %zu", env.size());
   exec->environ = env;
}

void C15unfolder::set_default_environment ()
{
   char * const * v;
   std::vector<const char *> env;

   // make a copy of our environment
   for (v = environ; *v; ++v) env.push_back (*v);
   env.push_back (nullptr);
   DEBUG ("c15u: set-env: |env| %zu", env.size());
   exec->environ = env;
}

Config C15unfolder::add_one_run (const Replay &r)
{
   Config c (Unfolding::MAX_PROC);

   ASSERT (exec);

   // run the guest
   DEBUG ("c15u: add-1-run: this %p |replay| %zu", this, r.size());
   exec->set_replay (r);
   DEBUG ("c15u: add-1-run: running the guest ...");
   exec->run ();

   // get a stream object from the executor and transform it into events
   stid::action_streamt actions (exec->get_trace ());
   actions.print ();
   stream_to_events (c, actions);
   return c;
}

void C15unfolder::add_multiple_runs (const Replay &r)
{
   Event *e;
   Replay rep (u);
   std::vector<Event *>cex;

   Config c (add_one_run (r));
   // add_one_run executes the system up to completion, we now compute all cex
   // of the resulting configuration and iterate through them
   c.dump ();

   // compute cex
   e = nullptr;
   compute_cex (c, &e);

   // copy the cex into a vector
   for (; e; e = e->next) cex.push_back (e);

   // run the system once more for every cex
   for (Event *e : cex)
   {
      rep.clear ();
      rep.extend_from (e->cone);
      rep.push_back ({-1, -1});
      add_one_run (rep);
   }
}

std::string C15unfolder::explore_stat (const Trail &t, const Disset &d) const
{
   unsigned u, j;

   u = j = 0;
   for (auto it = d.justified.begin(), end = d.justified.end();
         it != end; ++it) j++;
   for (auto it = d.unjustified.begin(), end = d.unjustified.end();
         it != end; ++it) u++;

   // 37e  9j  8u
   return fmt ("%2ue %2uj %2uu", t.size(), j, u);
}

void C15unfolder::explore ()
{
   bool b;
   Trail t;
   Disset d;
   Config c (Unfolding::MAX_PROC);
   Cut j (Unfolding::MAX_PROC);
   Replay replay (u);
   Event *e = nullptr;
   int i = 0;
   time_t start;

   // initialize the defect report now that all settings for this verification
   // exploration are fixed
   report_init (report);
   start = time (nullptr);

   while (1)
   {
      // explore the leftmost branch starting from our current node
      DEBUG ("c15u: explore: %s: running the system...",
            explore_stat (t, d).c_str());
      exec->run ();
      stid::action_streamt s (exec->get_trace ());
      counters.runs++;
      i = s.get_rt()->trace.num_ths;
      if (counters.stid_threads < i) counters.stid_threads = i;
      DEBUG ("c15u: explore: the stream: %s:", explore_stat (t,d).c_str());
#ifdef VERB_LEVEL_DEBUG
      if (verb_debug) s.print ();
#endif
      b = stream_to_events (c, s, &t, &d);
      // b could be false because of SSBs or defects
      DEBUG ("c15u: explore: xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
#ifdef VERB_LEVEL_TRACE
      if (verb_trace)
         t.dump2 (fmt ("c15u: explore: %s: ", explore_stat(t,d).c_str()).c_str());
      if (verb_debug) pidpool.dump ();
      if (verb_debug) c.dump ();
      if (verb_debug) d.dump ();
#endif

      // add conflicting extensions
      compute_cex (c, &e);
      counters.avg_max_trail_size += t.size();

      // backtrack until we find some right subtree to explore
      DEBUG ("");
      while (t.size())
      {
         // pop last event out of the trail/config; indicate so to the disset
         e = t.pop ();
         DEBUG ("c15u: explore: %s: popping: i %2zu ncs %u %s",
               explore_stat(t,d).c_str(), t.size(), t.nr_context_switches(),
               e->str().c_str());
         c.unfire (e);
         d.trail_pop (t.size ());

         // skip searching for alternatives if we exceed the number of allowed
         // context switches
         if (t.nr_context_switches() >= max_context_switches)
            DEBUG ("c15u: explore: %s: continue", explore_stat(t,d).c_str());
         if (t.nr_context_switches() >= max_context_switches) continue;

         // check for alternatives
         counters.alt.calls++;
         if (! might_find_alternative (c, d, e)) continue;
         d.add (e, t.size());
         if (find_alternative (t, c, d, j)) break;
         d.unadd ();
      }

      // if we exhausted the time cap, we stop
      if (counters.runs % 10 == 0 and opts::timeout)
         if (time(nullptr) - start > opts::timeout)
            { counters.timeout = true; break; }

      // if the trail is now empty, we finished; otherwise we compute a replay
      // and pass it to steroids
      if (! t.size ()) break;
      replay.build_from (t, c, j);
      set_replay_and_sleepset (replay, j, d);
   }

   // statistics
   counters.ssbs = d.ssb_count;
   counters.maxconfs = counters.runs - counters.ssbs;
   counters.avg_max_trail_size /= counters.runs;
   DEBUG ("c15u: explore: done!");
   ASSERT (counters.ssbs == 0 or altalgo != Altalgo::OPTIMAL);
}

void C15unfolder::set_replay_and_sleepset (Replay &replay, const Cut &j,
      const Disset &d)
{
   unsigned tid;

   exec->set_replay (replay);

   // no need for sleepsets if we are optimal
   if (altalgo == Altalgo::OPTIMAL) return;

   // otherwise we set sleeping the thread of every unjustified event in D that
   // is still enabled at J; this assumes that J contains C
   TRACE_ ("c15u: explore: sleep set: ");
   exec->clear_sleepset();
   for (auto e : d.unjustified)
   {
      ASSERT (e->action.type == ActionType::MTXLOCK);
      if (! j.ex_is_cex (e))
      {
         tid = replay.pidmap.get(e->pid());
         TRACE_ ("r%u (#%u) %p; ", tid, e->pid(), (void*) e->action.addr);
         exec->add_sleepset (tid, (void*) e->action.addr);
      }
   }
   TRACE ("");
}

void C15unfolder::compute_cex_lock (Event *e, Event **head)
{
   // 1. let ep be the pre-proc of e
   // 2. let em be the pre-mem of e
   //
   // 3. if em <= ep, then return   // there is no cex
   // 4. em = em.pre-em             // em is now a LOCK
   // 5. if (em <= ep) return       // because when you fire [ep] the lock is acquired!
   // 6. em = em.pre-mem            // em is now an UNLOCK
   // 7. insert event (e.action, ep, em)
   // 8. goto 3

   Event *ep, *em, *ee;

   // DEBUG ("c15u: cex-lock: starting from %s", e->str().c_str());
   ASSERT (e)
   ASSERT (e->action.type == ActionType::MTXLOCK);

   // 1,2: ep/em are the predecessors in process/memory
   ep = e->pre_proc();
   em = e->pre_other();

   while (1)
   {
      // 3. we are done if we got em <= ep (em == null means em = bottom!)
      if (!em or em->is_predeq_of (ep)) return;

      // 4,5: jump back 1 predecessor in memory, if we got em <= ep, return
      ASSERT (em->action.type == ActionType::MTXUNLK);
      em = em->pre_other();
      ASSERT (em);
      ASSERT (em->action.type == ActionType::MTXLOCK);
      if (em->is_predeq_of (ep)) return;

      // 6. back 1 more predecessor
      em = em->pre_other();
      ASSERT (!em or em->action.type == ActionType::MTXUNLK);

      // 7. (action, ep, em) is a possibly new event
      ee = u.event (e->action, ep, em);
      DEBUG ("c15u: cex-lock:  new cex: %s", ee->str().c_str());

      // we add it to the linked-list
      ee->next = *head; // next is also used in cut_to_replay
      *head = ee;
   }
}

void C15unfolder::compute_cex (Config &c, Event **head)
{
   Event *e;

   //DEBUG ("c15u: cex: c %p *head %p |mm| %d", &c, *head, c.mutexmax.size());

   for (auto const & max : c.mutexmax)
   {
      // skip events that are not locks or unlocks
      e = max.second;
      ASSERT (e);
      if (e->action.type != ActionType::MTXUNLK and
            e->action.type != ActionType::MTXLOCK) continue;

      // scan the lock chain backwards
      for (; e; e = e->pre_other())
      {
         if (e->action.type == ActionType::MTXLOCK)
         {
            compute_cex_lock (e, head);
         }
      }
   }
}

bool C15unfolder::is_conflict_free (const std::vector<Event *> &sol, const Event *e) const
{
   // we return false iff e is in conflict with some event of the partial solution "sol"
   int i;
   for (i = 0; i < sol.size(); i++)
      if (e->in_cfl_with (sol[i])) return false;
   return true;
}

bool C15unfolder::enumerate_combination (unsigned i,
      std::vector<Event*> &sol)
{
   // We enumerate combinations using one event from each spike
   // - the partial solution is stored in sol
   // - if sol is conflict-free, we return true

   ASSERT (i < comb.size());
   for (auto e : comb[i])
   {
      if (! is_conflict_free (sol, e)) continue;
      sol.push_back (e);
      if (sol.size() == comb.size()) return true;
      if (enumerate_combination (i+1, sol)) return true;
      sol.pop_back ();
   }
   return false;
}

bool C15unfolder::might_find_alternative (Config &c, Disset &d, Event *e)
{
   // this method can return return false only if we are totally sure that no
   // alternative to D \cup e exists after C; this method should run fast, it
   // will be called after popping every single event event from the trail; our
   // implementation is very fast: we return false if the event type is != lock

   // e->icfls() is nonempty => e is a lock
   ASSERT (! e->icfl_count() or e->action.type == ActionType::MTXLOCK);

   return e->action.type == ActionType::MTXLOCK;
}

inline bool C15unfolder::find_alternative (const Trail &t, Config &c, const Disset &d, Cut &j)
{
   bool b;

   switch (altalgo) {
   case Altalgo::OPTIMAL :
   case Altalgo::KPARTIAL :
      b = find_alternative_kpartial (c, d, j);
      break;
   case Altalgo::ONLYLAST :
      b = find_alternative_only_last (c, d, j);
      break;
   case Altalgo::SDPOR :
      b = find_alternative_sdpor (c, d, j);
      break;
   }

   // no alternative may intersect with d
   if (b)
   {
      //if (d.intersects_with (j)) { d.dump (); j.dump (); }
      ASSERT (! d.intersects_with (j));
   }

   TRACE_ ("c15u: explore: %s: alt: [", explore_stat (t, d).c_str());
#ifdef VERB_LEVEL_TRACE
   if (verb_trace)
   {
      std::vector<Event*> v;
      for (auto e : d.unjustified)
      {
         e->icfls(v);
         TRACE_("%zu ", v.size());
         v.clear();
      }
   }
#endif
   TRACE ("\b] %s", b ? "found" : "no");
   if (b) DEBUG ("c15u: explore: %s: j: %s", explore_stat(t, d).c_str(), j.str().c_str());
   return b;
}

bool C15unfolder::find_alternative_only_last (const Config &c, const Disset &d, Cut &j)
{
   // - (complete but unoptimal)
   // - consider the last (unjustified) event added to D, call it e
   // - if you find some immediate conflict e' of e that is compatible with C (that
   //   is, e' is not in conflict with any event in proc-max(C)), then set J = [e']
   //   and return it
   // - in fact we set J = C \cup [e'], because of the way we need to compute
   //   the sleeping threads to pass them to steroids
   // - as an optimization to avoid some SSB executions, you could skip from the
   //   previous iteration those e' in D, as those will necessarily be blocked
   // - if you don't find any such e', return false

   Event * e;
   bool b;
#ifdef CONFIG_STATS_DETAILED
   unsigned count = 0;
#endif

   // D is not empty
   ASSERT (d.unjustified.begin() != d.unjustified.end());

   // statistics
   counters.alt.calls_built_comb++;
   counters.alt.calls_explore_comb++;
   counters.alt.spikes.sample (1); // number of spikes

   // last event added to D
   e = *d.unjustified.begin();
   DEBUG ("c15u: alt: only-last: c %s e %s", c.str().c_str(), e->suid().c_str());

   // scan the spike of that guy, we use 1 spike in the comb
   comb.clear();
   comb.add_spike (e);
#ifdef CONFIG_STATS_DETAILED
   counters.alt.spikesizeunfilt.sample (comb[0].size());
#endif
   b = false;
   for (Event *ee : comb[0])
   {
#ifdef CONFIG_STATS_DETAILED
      count++;
#endif
      if (!ee->flags.ind and !ee->in_cfl_with (c) and !d.intersects_with (ee))
      {
         j = c;
         j.unionn (ee);
         b = true;
         break;
      }
   }
#ifdef CONFIG_STATS_DETAILED
   counters.alt.spikesizefilt.sample (count);
#endif
   return b;
}

bool C15unfolder::find_alternative_kpartial (const Config &c, const Disset &d, Cut &j)
{
   // We do an exahustive search for alternatives to D after C, we will find one
   // iff one exists. We use a comb that has one spike per unjustified event D.
   // Each spike is made out of the immediate conflicts of that event in D. The
   // unjustified events in D are all enabled in C, none of them is in cex(C).

   unsigned i, num_unjust;
   std::vector<Event*> solution;

#ifdef VERB_LEVEL_DEBUG
   DEBUG_ ("c15u: alt: kpartial: k %u c %s d.unjust [",
         kpartial_bound, c.str().c_str());
   for (auto e : d.unjustified) DEBUG_("%p ", e);
#endif
   DEBUG ("\b]");

   ASSERT (altalgo == Altalgo::OPTIMAL or
         altalgo == Altalgo::KPARTIAL);
   ASSERT (altalgo != Altalgo::OPTIMAL or
         kpartial_bound == UINT_MAX);
   ASSERT (kpartial_bound >= 1);

   // build the spikes of the comb; there are many other ways to select the
   // interesting spikes much more interesting than this plain truncation ...
   comb.clear();
   num_unjust = 0;
   for (const auto e : d.unjustified)
   {
      if (num_unjust < kpartial_bound) comb.add_spike (e);
      num_unjust++;
   }
   ASSERT (! comb.empty());
   DEBUG ("c15u: alt: kpartial: comb: initially:\n%s", comb.str().c_str());

   // we have constructed a (non-empty) comb
   counters.alt.calls_built_comb++;
#ifdef CONFIG_STATS_DETAILED
   for (auto &spike : comb) counters.alt.spikesizeunfilt.sample (spike.size());
#endif

   // remove from each spike those events whose local configuration includes
   // some ujustified event in D, or in conflict with someone in C; the
   // (expensive) check "d.intersects_with" could be avoided if we are computing
   // optimal alternatives, as those events could never make part of a solution;
   // however, if we are computing partial alternatives, the check is
   // unavoidable
   for (auto &spike : comb)
   {
      i = 0;
      while (i < spike.size())
      {
         if (spike[i]->flags.ind or spike[i]->in_cfl_with(c) or
               (altalgo != Altalgo::OPTIMAL and d.intersects_with (spike[i])))
         {
            spike[i] = spike.back();
            spike.pop_back();
         }
         else
            i++;
      }
      // if one spike becomes empty, there is no alternative
      if (spike.empty()) return false;
   }
   DEBUG ("c15u: alt: kpartial: comb: after removing D and #(C), and bounding:\n%s",
         comb.str().c_str());

   // we have to explore the comb
   counters.alt.calls_explore_comb++;
   counters.alt.spikes.sample (num_unjust);
#ifdef CONFIG_STATS_DETAILED
   for (auto &spike : comb) counters.alt.spikesizefilt.sample (spike.size());
#endif

   // explore the comb, the combinatorial explosion could happen here
   if (enumerate_combination (0, solution))
   {
      // we include C in J, it doesn't hurt and it will make the calls to
      // j.unionn() run faster; an alternative could be to do j.clear(), but we
      // cannot do it: we need C to be in J to compute sleepsets
      j = c;

      // we build a cut as the union of of all local configurations for events
      // in the solution
      for (auto e : solution) j.unionn (e);
      return true;
   }
   return false;
}

bool C15unfolder::find_alternative_sdpor (Config &c, const Disset &d, Cut &j)
{
   Event * e;
   bool b;
   unsigned i, color;

   // find alternatives for only the last event in D
   b = find_alternative_only_last (c, d, j);
   if (! b) return b;

   // colorize all events in C
   color = u.get_fresh_color();
   c.colorize (color);

   // scan J until we find some event in J enabled at C
   //j.dump ();
   //c.dump ();
   for (i = 0; i < j.num_procs(); i++)
   {
      for (e = j[i]; e; e = e->pre_proc())
      {
         SHOW (e->str().c_str(), "s");
         // if e is in C, then this is "too low"
         if (e->color == color) break;
         // skip events where the pre-proc is not in C
         if (e->pre_proc() and e->pre_proc()->color != color) continue;
         // skip events where the pre-other is not in C
         if (e->pre_other() and e->pre_other()->color != color) continue;
         // we found the right e
         i = c.num_procs();
         break;
      }
   }

   // assert that e is enabled at C
   ASSERT (e);
   ASSERT (e->color != color);
   ASSERT (e->pre_proc() == c.proc_max (e->pid()));
   ASSERT (! e->pre_other() or e->pre_other()->color == color);
   ASSERT (! e->in_cfl_with (c));

   // and that it is not in D !!
   ASSERT (! e->flags.ind);

   // our alternative is J := C \cup {e}
   j = c;
   j.unionn (e); // for fun, comment out this line ;)
   return true;
}

void C15unfolder::report_init (Defectreport &r) const
{
   std::vector<std::string> myargv (exec->argv.begin(), exec->argv.end());
   std::vector<std::string> myenv (exec->environ.begin(), --(exec->environ.end()));

   r.dpuversion = CONFIG_VERSION;
   r.path = path;
   r.argv = myargv;
   r.environ = myenv;
   r.alt = (int) altalgo;
   r.kbound = kpartial_bound;
   r.memsize = exec->config.memsize;
   r.defaultstacksize = exec->config.defaultstacksize;
   r.tracesize = exec->config.tracesize;
   r.optlevel = exec->config.optlevel;

   r.nr_exitnz = 0;
   r.nr_abort = 0;
   r.defects.clear ();
}

} // namespace dpu
