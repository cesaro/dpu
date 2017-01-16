
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


C15unfolder::C15unfolder () :
   m (nullptr),
   exec (nullptr)
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
   conf.memsize = 128 << 20;
   conf.stacksize = 16 << 20;
   conf.tracesize = 16 << 20;

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
   Trail t;
   Disset d;
   Config c (Unfolding::MAX_PROC);
   Cut j (Unfolding::MAX_PROC);
   std::vector<int> replay {-1};
   Event *e = nullptr;
   int i = 0;

   while (1)
   {
      // explore the leftmost branch starting from our current node
      DEBUG ("c15u: explore: %s: running the system...",
            explore_stat (t, d).c_str());
      exec->set_replay (replay.data(), replay.size());
      exec->run ();
      action_streamt s (exec->get_trace ());
      DEBUG ("c15u: explore: the stream: %s:", explore_stat (t,d).c_str());
      if (verb_debug) s.print ();
      stream_to_events (c, s, &t, &d);
      DEBUG ("c15u: explore: xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
      if (verb_trace)
         t.dump2 (fmt ("c15u: explore: %s: ", explore_stat(t,d).c_str()).c_str());
      if (verb_debug) c.dump ();
      if (verb_debug) d.dump ();

      // FIXME turn this into a commandline option
      std::ofstream f (fmt ("dot/c%02d.dot", i));
      u.print_dot (c, f, fmt ("Config %d", i));
      f.close ();
      f.open (fmt ("/tmp/dot/c%02d.dot", i));
      u.print_dot (c, f, fmt ("Config %d", i));
      f.close ();
      i++;

      // add conflicting extensions
      compute_cex (c, &e);

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
      DEBUG ("c15u: explore: alternative found: %s", j.str().c_str());
      alt_to_replay (t, c, j, replay);
   }
   DEBUG ("c15u: explore: done!");
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
      len = e->depth_proc();
      if (c2[i]) len -= (int) c2[i]->depth_proc(); else len++;
      if (len <= 0) continue;

      // set up the Event's next pointer in order to run forward
      ee = nullptr;
      for (j = 0; j < len; j++)
      {
         e->next = ee;
         ee = e;
         e = e->pre_proc();
      }

      // we set cc to be a suitable starting state for (c1 \setminus c2), which
      // is "the next layer of events above" the cut of (c1 \cap c2)
      cc[i] = ee;
   }

   // in a round-robbin fashion, we run each process until we cannot continue in
   // that process; this continues until we are unable to make progress
   progress = true;
   while (progress)
   {
      progress = false;
      for (i = 0; i < nrp; i++)
      {
         e = cc[i];
         //DEBUG ("c15u: cut-to-replay: ctxsw to pid %d, event %p", i, e);
         count = 0;
         // we replay as many events as possible on process i
         for (; e; e = e->next)
         {
            // if event has non-visited causal predecessor, abort this process
            //SHOW (e->str().c_str(), "s");
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

   replay.clear();
   trail_to_replay (t, replay);
   cut_to_replay (j, c, replay);
   replay.push_back (-1);
}

void C15unfolder::compute_cex_lock (Event *e, Event **head)
{
   Event *ep, *em, *ee;

   //DEBUG ("c15u: cex-lock: e %p *head %p", e, *head);
   ASSERT (e)
   ASSERT (e->action.type == ActionType::MTXLOCK);

   // ep/em are the predecessors in process/memory
   ep = e->pre_proc();
   em = e->pre_other();

   while (1)
   {
      // jump back 2 predecessors in memory; if we get null or another event in
      // the same process, then em <= ep
      if (!em or em->proc() == e->proc()) return;
      ASSERT (em->action.type == ActionType::MTXUNLK);
      em = em->pre_other();
      if (!em) return;
      ASSERT (em->action.type == ActionType::MTXLOCK);
      em = em->pre_other();

      // we are done if we got em <= ep
      if (em and em->is_predeq_of (ep)) return;

      // (action, ep, em) is a (new) event
      ee = u.event (e->action, ep, em);
      DEBUG ("c15u: cex-lock: new cex: %s", ee->str().c_str());

      // we add it to the linked-list
      ee->next = *head; // next is also used in cut_to_replay
      *head = ee;
   }
} 

void C15unfolder::compute_cex (Config &c, Event **head)
{
   Event *e;

   //DEBUG("c15u: cex: c %p *head %p |mm| %d", &c, *head, c.mutexmax.size());

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

bool C15unfolder::is_conflict_free(std::vector<Event *> eset)
{
   int i,j;
   for (i = 0; i < eset.size() -1; i++)
      for (j = i+1; j < eset.size(); j++)
         if (eset[i]->in_cfl_with(eset[j]))
            return false;
   return true;
}

void C15unfolder::enumerate_combination (unsigned i, std::vector<std::vector<Event *>> comb,
      std::vector<Event*> temp, Cut &J)
{
   // We enumerate combinations using one event from each spike
   // - the temporary combination is stored in temp
   // - if temp is conflict-free, assign it to J

   DEBUG("=======enumerate combination=======");

   ASSERT(!comb.empty());
   for (auto e: comb[i])
   {
      temp.push_back(e);
      if (temp.size() == comb.size()) // not at the last spike
      {
         if (is_conflict_free(temp))
         {
            DEBUG(": a conflict-free combination");

            for (auto e : temp)
               J[e->pid()] = e;
            return; // go back to find_alternative_optim_comb
         }
         else
            temp.pop_back();
      }
      else
         enumerate_combination(i+1, comb, temp, J);
   }
   return;
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
   b = find_alternative_only_last (c, d, j);
   // b = return find_alternative_optim_comb ();

   TRACE_ ("c15u: explore: %s: alt: [", explore_stat (t, d).c_str());
   for (auto e : d.unjustified)  TRACE_("%u ", e->icfls().size());
   TRACE ("\b] %s", b ? "found" : "no");
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

   e = *d.unjustified.begin();
   DEBUG ("c15u: alt: only-last: c %s e %p", c.str().c_str(), e);

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

bool C15unfolder::find_alternative_optim_comb (const Config &c, const Disset &d, Cut &j)
{
   // We do an exahustive search for alternatives to D after C, we will find one
   // iff one exists. We use a comb that has one spike per unjustified event D.
   // Each spike is made out of the immediate conflicts of that event in D. The
   // unjustified events in D are all enabled in C, none of them is in cex(C).

   unsigned i, k;
   std::vector<std::vector<Event *>> comb;

   DEBUG(" Find an alternative J to D after C: ");
   DEBUG_("   D = {");
   for (auto e : d.unjustified)  DEBUG_("%p  ", e);
   DEBUG("}");
   DEBUG_("   After C \n ");
   c.dump();

   // build the spikes of the comb
   for (auto e : d.unjustified) comb.push_back (e->icfls());
   if (comb.empty()) return false;

   // Iterator it = d.unjustified.begin();
   DEBUG("COMB: %d spikes: ", comb.size());
   for (i = 0; i < comb.size(); i++)
   {
      // DEBUG_ ("  spike %d: (#%p (len %d): ", i, it++, comb[i].size());
      for (k = 0; k < comb[i].size(); k++) DEBUG_(" %p", comb[i][k]);
      DEBUG("");
   }
   DEBUG("END COMB");

   // remove from each spike those events in D or in conflict with someone in C
   for (auto spike : comb)
   {
      if (spike.empty()) return false;
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
   }

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

   std::vector<Event *> temp;
   j.clear();
   enumerate_combination(0, comb, temp, j);
   if (j.is_empty()) return false;

   return true;
}

} // namespace dpu
