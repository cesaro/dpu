
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

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

#include "c15unfold.hh" // must be before verbosity.h
#include "misc.hh"
#include "verbosity.h"
#include "pes/process.hh"

namespace dpu
{

static void _ir_write_ll (const llvm::Module *m, const char *filename)
{
   int fd = open (filename, O_WRONLY | O_TRUNC | O_CREAT, 0644);
   ASSERT (fd >= 0);
   llvm::raw_fd_ostream f (fd, true);
   f << *m;
}


C15unfolder::C15unfolder (Alt_algorithm a, unsigned kbound) :
   counters {0, 0, 0, 0},
   m (nullptr),
   exec (nullptr),
   alt_algorithm (a),
   kpartial_bound (kbound)
{
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

   DEBUG ("c15u: load-bytecode: this %p path '%s'", this, filepath.c_str());

   ASSERT (filepath.size());
   ASSERT (path.size() == 0);
   ASSERT (exec == 0);
   ASSERT (m == 0);
   ASSERT (argv.size() == 0);
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

   // prepare an Executor, the constructor instruments and allocates guest
   // memory
   // FIXME - make this configurable via methods of the C15unfolder
   ExecutorConfig conf;
   conf.memsize = CONFIG_GUEST_MEMORY_SIZE;
   conf.stacksize = CONFIG_GUEST_THREAD_STACK_SIZE;
   conf.tracesize = CONFIG_GUEST_TRACE_BUFFER_SIZE;

   DEBUG ("c15u: load-bytecode: creating a bytecode executor...");
   try {
      exec = new Executor (std::move (mod), conf);
   } catch (const std::exception &e) {
      DEBUG ("c15u: load-bytecode: errors preparing the bytecode executor");
      DEBUG ("c15u: load-bytecode: %s", e.what());
      throw e;
   }
   DEBUG ("c15u: load-bytecode: executor successfully created!");

   TRACE ("c15u: load-bytecode: saving instrumented bytecode to /tmp/output.ll");
   _ir_write_ll (m, "/tmp/output.ll");

   DEBUG ("c15u: load-bytecode: done!");
}

void C15unfolder::set_args (std::vector<const char *> argv)
{
   DEBUG ("c15u: set-args: |argv| %d", argv.size());

   // FIXME - this should be moved to a proper API
   exec->envp.push_back ("HOME=/home/cesar/");
   exec->envp.push_back ("PWD=/usr/bin");
   exec->envp.push_back (nullptr);

   exec->argv = argv;
}

Config C15unfolder::add_one_run (const std::vector<int> &replay)
{
   Config c (Unfolding::MAX_PROC);

   ASSERT (exec);

   // run the guest
   DEBUG ("c15u: add-1-run: this %p |replay| %d", this, replay.size());
   exec->set_replay (replay.data(), replay.size());
   DEBUG ("c15u: add-1-run: running the guest ...");
   exec->run ();

   // get a stream object from the executor and transform it into events
   action_streamt actions (exec->get_trace ());
   actions.print ();
   stream_to_events (c, actions);
   return c;
}

void C15unfolder::add_multiple_runs (const std::vector<int> &replay)
{
   Event *e;
   std::vector<int> rep;
   std::vector<Event *>cex;
   Config c (add_one_run (replay));

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
      cut_to_replay (e->cone, rep);
      rep.push_back (-1);
      add_one_run (rep);
      rep.clear ();
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
   std::vector<int> replay {-1};
   Event *e = nullptr;
   //int i = 0;

   while (1)
   {
      // explore the leftmost branch starting from our current node
      DEBUG ("c15u: explore: %s: running the system...",
            explore_stat (t, d).c_str());
      exec->set_replay (replay.data(), replay.size());
      exec->run ();
      counters.runs++;
      action_streamt s (exec->get_trace ());
      DEBUG ("c15u: explore: the stream: %s:", explore_stat (t,d).c_str());
      if (verb_debug) s.print ();
      b = stream_to_events (c, s, &t, &d);
      if (!b) counters.ssbs++;
      DEBUG ("c15u: explore: xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
      if (verb_trace)
         t.dump2 (fmt ("c15u: explore: %s: ", explore_stat(t,d).c_str()).c_str());
      if (verb_debug) c.dump ();
      if (verb_debug) d.dump ();

      // add conflicting extensions
      compute_cex (c, &e);
      counters.avg_max_trail_size += t.size();

#if 0
      // FIXME turn this into a commandline option
      std::ofstream f (fmt ("dot/c%02d.dot", i));
      //u.print_dot (c, f, fmt ("Config %d", i));
      f.close ();
      f.open (fmt ("/tmp/dot/c%02d.dot", i));
      u.print_dot (c, f, fmt ("Config %d", i));
      f.close ();
      i++;
#endif

      // backtrack until we find some right subtree to explore
      DEBUG ("");
      while (t.size())
      {
         // pop last event out of the trail/config; indicate so to the disset
         e = t.pop ();
         DEBUG ("c15u: explore: %s: popping: i %2u %s",
               explore_stat(t,d).c_str(), t.size(), e->str().c_str());
         c.unfire (e);
         d.trail_pop (t.size ());

         // check for alternatives
         if (! might_find_alternative (c, d, e)) continue;
         d.add (e, t.size());
         if (find_alternative (t, c, d, j)) break;
         d.unadd ();
      }

      // if the trail is now empty, we finished; otherwise we compute a replay
      // and restart
      if (! t.size ()) break;
      alt_to_replay (t, c, j, replay);
   }

   // statistics
   ASSERT (counters.ssbs == d.ssb_count);
   counters.maxconfs = counters.runs - counters.ssbs;
   counters.avg_max_trail_size /= counters.runs;
   DEBUG ("c15u: explore: done!");

   ASSERT (counters.ssbs == 0 or alt_algorithm != Alt_algorithm::OPTIMAL);
}

void C15unfolder::cut_to_replay (const Cut &c, std::vector<int> &replay)
{
   static const Cut empty (Unfolding::MAX_PROC);

   cut_to_replay (c, empty, replay);
}

void C15unfolder::cut_to_replay (const Cut &c1, const Cut &c2, std::vector<int> &replay)
{
   int count, len, j;
   unsigned i, nrp;
   bool progress;
   Event *e, *ee;
   Cut cc (std::min (c1.num_procs(), u.num_procs()));

   // we need to replay every event in c1 that is not in c2

   DEBUG ("c15u: cut-to-replay: cut1 %s cut2 %s",
         c1.str().c_str(), c2.str().c_str());

   // we use an auxiliary cut cc to run forward the events in c1 \setminus c2,
   // to do so, we set up and use the Event's next pointer
   nrp = cc.num_procs();
   //SHOW (nrp, "u");
   for (i = 0; i < nrp; i++)
   {
      // compute len, the number of events that we will run in process i
      e = c1[i];
      if (! e) continue;
      DEBUG ("c15u: cut-to-replay: at least one  %d events on proc %d", len, i);
      len = e->depth_proc();
      if (c2[i]) len -= (int) c2[i]->depth_proc(); else len++;
      if (len <= 0) continue;
      DEBUG ("c15u: cut-to-replay: running %d events on proc %d", len, i);

      // set up the Event's next pointer in order to run forward
      ee = nullptr;
      for (j = 0; j < len; j++)
      {
         e->next = ee;
         ee = e;
         e = e->pre_proc();
      }

      // we set cc to be a suitable starting state for (c1 \setminus c2), which
      // is "the layer of events one above" the cut of (c1 \cap c2)
      cc[i] = ee;
   }
   DEBUG ("c15u: cut-to-replay: running forward cc %s", cc.str().c_str());

   // in a round-robbin fashion, we run each process until we cannot continue in
   // that process; this continues until we are unable to make progress
   progress = true;
   while (progress)
   {
      progress = false;
      for (i = 0; i < nrp; i++)
      {
         e = cc[i];
         DEBUG ("c15u: cut-to-replay: ctxsw to pid %d, event %p", i, e);
         count = 0;
         // we replay as many events as possible on process i
         for (; e; e = e->next)
         {
            // if event has non-visited causal predecessor, abort this process
            SHOW (e->str().c_str(), "s");
            if (e->pre_other()
                  and i != e->pre_other()->pid()
                  and cc[e->pre_other()->pid()]
                  and e->pre_other()->depth_proc() > cc[e->pre_other()->pid()]->depth_proc())
                     break;
            count++;
         }

         if (count)
         {
            cc[i] = e;
            replay.push_back (i);
            replay.push_back (count);
            DEBUG ("c15u: cut-to-replay: %u %d", i, count);
            progress = true;
         }
      }
   }
}

void C15unfolder::trail_to_replay (const Trail &t, std::vector<int> &replay) const
{
   unsigned pid;
   unsigned count;

   // if there is at least one event, then the first is for pid 0
   count = pid = 0;
   ASSERT (t.size() == 0 or t[0]->pid() == pid);

   for (Event *e : t)
   {
      if (e->pid() != pid)
      {
         DEBUG ("c15u: trail-to-replay: %u %d", pid, count);
         replay.push_back (pid);
         replay.push_back (count);
         pid = e->pid ();
         count = 0;
      }
      count++;
   }
   if (count)
   {
      DEBUG ("c15u: trail-to-replay: %u %d", pid, count);
      replay.push_back (pid);
      replay.push_back (count);
   }
}

void C15unfolder::alt_to_replay (const Trail &t, const Cut &c, const Cut &j,
      std::vector<int> &replay)
{
   // the sequence of number that we need to compute here allows steroids to
   // replay the trail (exacty in that order) followed by the replay of
   // J \setminus C (in any order).

   unsigned i;

   replay.clear();
   trail_to_replay (t, replay);
   i = replay.size();
   cut_to_replay (j, c, replay);
   replay.push_back (-1);

   TRACE ("c15u: explore: replay seq: %s", replay2str(replay,i).c_str());
}

std::string C15unfolder::replay2str (std::vector<int> &replay, unsigned altidx)
{
   std::string s;
   unsigned i;

   ASSERT (altidx % 2 == 0);
   for (i = 0; replay[i] != -1; i += 2)
   {
      if (i == altidx) s += "** ";
      s += fmt ("%d %d; ", replay[i], replay[i+1]);
   }
   s += "-1";
   return s;
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
      std::vector<std::vector<Event *>> &comb,
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
      if (enumerate_combination (i+1, comb, sol)) return true;
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
   ASSERT (! e->icfls().size() or e->action.type == ActionType::MTXLOCK);

   return e->action.type == ActionType::MTXLOCK;
}

inline bool C15unfolder::find_alternative (const Trail &t, const Config &c, const Disset &d, Cut &j)
{
   bool b;
   
   switch (alt_algorithm) {
   case Alt_algorithm::KPARTIAL :
      b = find_alternative_kpartial (c, d, j);
      break;
   case Alt_algorithm::OPTIMAL :
      b = find_alternative_optim_comb (c, d, j);
      break;
   case Alt_algorithm::ONLYLAST :
      b = find_alternative_only_last (c, d, j);
      break;
   }

   TRACE_ ("c15u: explore: %s: alt: [", explore_stat (t, d).c_str());
   for (auto e : d.unjustified)  TRACE_("%u ", e->icfls().size());
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
   // - as an optimization to avoid some SSB executions, you could skip from the
   //   previous iteration those e' in D, as those will necessarily be blocked
   // - if you don't find any such e', return false

   Event * e;

   // D is not empty
   ASSERT (d.unjustified.begin() != d.unjustified.end());

   // last event added to D
   e = *d.unjustified.begin();
   DEBUG ("c15u: alt: only-last: c %s e %s", c.str().c_str(), e->suid().c_str());

   for (Event *ee : e->icfls())
   {
      if (!ee->flags.ind and !ee->in_cfl_with (c))
      {
         j = ee->cone;
         return true;
      }
   }
   return false;
}

std::string comb2str (std::vector<std::vector<Event *>> &comb)
{
   std::string s;
   unsigned i, j;

   for (i = 0; i < comb.size(); i++)
   {
      s += fmt ("spike %u len %u [", i, comb[i].size());
      for (j = 0; j < comb[i].size(); j++)
      {
         s += fmt ("%s ", comb[i][j]->suid().c_str());
      }
      if (j) s.pop_back();
      s += fmt ("]\n");
   }

   return s;
}

bool C15unfolder::find_alternative_optim_comb (const Config &c, const Disset &d, Cut &j)
{
   // We do an exahustive search for alternatives to D after C, we will find one
   // iff one exists. We use a comb that has one spike per unjustified event D.
   // Each spike is made out of the immediate conflicts of that event in D. The
   // unjustified events in D are all enabled in C, none of them is in cex(C).

   unsigned i;
   std::vector<std::vector<Event *>> comb;
   std::vector<Event*> solution;

   DEBUG_ ("c15u: alt: optim: c %s d.unjust [", c.str().c_str());
   for (auto e : d.unjustified)  DEBUG_("%p ", e);
   DEBUG ("\b]");

   // build the spikes of the comb
   for (auto e : d.unjustified) comb.push_back (e->icfls());
   if (comb.empty()) return false;

   DEBUG ("c15u: alt: optim: comb: initially:\n%s", comb2str(comb).c_str());

   // remove from each spike those events in D or in conflict with someone in C
   for (auto &spike : comb)
   {
      i = 0;
      while (i < spike.size())
      {
         if (spike[i]->flags.ind or spike[i]->in_cfl_with(c))
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
   DEBUG ("c15u: alt: optim: comb: after removing D and #(C):\n%s", comb2str(comb).c_str());

//   // show the comb
//   DEBUG("COMB: %d spikes: ", comb.size());
//      for (unsigned i = 0; i < comb.size(); i++)
//      {
//         DEBUG_ ("  spike %d: (#%p (len %d): ", i, d[i], comb[i].size());
//         for (unsigned j = 0; j < comb[i].size(); j++)
//            DEBUG_(" %p", comb[i][j]);
//         DEBUG("");
//      }
//   DEBUG("END COMB");

   if (enumerate_combination (0, comb, solution))
   {
      j.clear();
      for (auto e : solution) j.maxhull (e);
      return true;
   }
   return false;
}

bool C15unfolder::find_alternative_kpartial (const Config &c, const Disset &d, Cut &j)
{
   // We do an exahustive search for alternatives to D after C, we will find one
   // iff one exists. We use a comb that has one spike per unjustified event D.
   // Each spike is made out of the immediate conflicts of that event in D. The
   // unjustified events in D are all enabled in C, none of them is in cex(C).

   unsigned i;
   static unsigned hack_count = 0;
   std::vector<std::vector<Event *>> comb;
   std::vector<std::vector<Event *>> comb2;
   std::vector<Event*> solution;
   std::vector<Event*> solution2;

   DEBUG_ ("c15u: alt: kpartial: k %u c %s d.unjust [",
         kpartial_bound, c.str().c_str());
   for (auto e : d.unjustified)  DEBUG_("%p ", e);
   DEBUG ("\b]");

   // build the spikes of the comb
   for (auto e : d.unjustified) comb.push_back (e->icfls());
   if (comb.empty()) return false;

   DEBUG ("c15u: alt: kpartial: comb: initially:\n%s", comb2str(comb).c_str());

   // remove from each spike those events in D or in conflict with someone in C
   for (auto &spike : comb)
   {
      i = 0;
      while (i < spike.size())
      {
         if (spike[i]->flags.ind or spike[i]->in_cfl_with(c))
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

   // FIXME - HACK to overcome the problem with the lack of sleeping processes
   // in steroids
   comb2 = comb;

   // bound the comb to k spikes; there is many other ways to select the
   // interesting spikes much more intelling than this plain truncation ...
   ASSERT (kpartial_bound >= 1);
   if (comb.size() > kpartial_bound) comb.resize (kpartial_bound);

   DEBUG ("c15u: alt: kpartial: comb: after removing D and #(C), and bounding:\n%s",
         comb2str(comb).c_str());

   if (enumerate_combination (0, comb, solution))
   {
      // HACK
      if (enumerate_combination (0, comb2, solution2))
      {
         hack_count++;
         TRACE ("c15u: alt: kpartial: warning: overriding unoptimal comb "
               "solution with optimal one (count %u)", hack_count);
         solution = solution2;
      }

      j.clear();
      for (auto e : solution) j.maxhull (e);
      return true;
   }
   return false;
}

} // namespace dpu
