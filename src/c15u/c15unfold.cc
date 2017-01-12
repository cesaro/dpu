
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

   DEBUG ("c15u: load-bytecode: saving instrumented bytecode to /tmp/output.ll");
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
      DEBUG ("c15u: explore: running the system...");
      exec->set_replay (replay.data(), replay.size());
      exec->run ();
      action_streamt s (exec->get_trace ());
      DEBUG ("c15u: explore: the stream:");
      s.print ();
      stream_to_events (c, s, &t, &d);
      DEBUG ("c15u: explore: xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
      t.dump ();
      c.dump ();
      d.dump ();

      std::ofstream f (fmt ("dot/c%02d.dot", i));
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
         DEBUG ("c15u: explore: popping: i %2u %s", t.size(), e->str().c_str());
         c.unfire (e);
         d.trail_pop (t.size ());

         // check for alternatives
         if (! might_find_alternative (c, d, e)) continue;
         d.add (e, t.size());
         if (find_alternative_only_last (c, d, j)) break;
         d.unadd ();
      }

      // if the trail is now empty, we finished; otherwise we compute a replay
      // and restart
      if (! t.size ()) break;
      DEBUG ("c15u: explore: alternative found: %s", j.str().c_str());
      alt_to_replay (c, j, replay);
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

void C15unfolder::alt_to_replay (const Cut &c, const Cut &j, std::vector<int> &replay)
{
   // the replay that we need to compute here for C \cup J is the replay of C
   // followed by the replay of J \setminus C

   replay.clear();
   cut_to_replay (c, replay);
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

bool C15unfolder::find_alternative_only_last (const Config &c, Disset &d, Cut &j)
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

bool C15unfolder::is_conflict_free(std::vector<Event *> eset)
{
   int i,j;
   for (i = 0; i < eset.size() -1; i++)
      for (j = i+1; j < eset.size(); j++)
         if (eset[i]->in_cfl_with(eset[j]))
            return false;
   return true;
}


/* enumerate combinations whose each element is in a spike of the comb.
 * - the temporary combination is stored in temp.
 * - If temp is conflict-free, assign it to J
 */

/// If there exist a config satisfied, it will be assigned to J
void C15unfolder::enumerate_combination (unsigned i, std::vector<std::vector<Event *>> comb,
      std::vector<Event*> temp, Cut &J)
{
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
            return; // go back to find_alternative
         }
         else
            temp.pop_back();
      }
      else
         enumerate_combination(i+1, comb, temp, J);
   }
   return;
}

bool C15unfolder::find_alternative (Config &c, Disset &d, Cut &J)
{
//   Event *pe;

   J.clear();

   std::vector<std::vector<Event *>> comb;

   DEBUG(" Find an alternative J to D after C: ");

   // dump D
      DEBUG_("   D = {");
         for (auto e : d.unjustified)  DEBUG_("%p  ", e);
      DEBUG("}");

   // dump c
      DEBUG_("   After C \n "); c.dump();

   /*
    *  D now contains only events from en(C) -> so, don't need to check if some events in D are in C.cex.
    *  Other functions makes sure that nothing in D is in c.cex
    */

   ///  comb is constructed by pouring all conflicting events with each event in D (computed by icfls()) into a spike
      for (auto e : d.unjustified)  comb.push_back (e->icfls());

   if (comb.empty()) return false;

   //int idx = 0;
//   Iterator it = d.unjustified.begin();
   ///show the comb
//   ASSERT (comb.size() == d.size ());
   DEBUG("COMB: %d spikes: ", comb.size());
      for (unsigned i = 0; i < comb.size(); i++)
      {
//         DEBUG_ ("  spike %d: (#%p (len %d): ", i, it++, comb[i].size());
         for (unsigned j = 0; j < comb[i].size(); j++)
            DEBUG_(" %p", comb[i][j]);
         DEBUG("");
      }
   DEBUG("END COMB");

//  // set the flag inside of all events in D
//   for (auto e : d) e->inside = 1; // use flags.ind instead of inside


   /// remove from spk those whose flag inside is set
   unsigned j = 0;

   for (auto spk : comb)
   {
      if (spk.empty()) return false;
      while (j < spk.size())
      {
         if ( (spk[j]->flags.ind) or (spk[j]->in_cfl_with(c)) ) //spk[j] is in D or in conflict with c
         {
            spk[j] = spk.back();
            spk.pop_back();
         }
         else
            j++;
      }
   }

//   ///show the comb
//   ASSERT (d);

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
   enumerate_combination(0, comb, temp, J);
   if (J.is_empty())
      return false;

   return true;
}

} // namespace dpu
