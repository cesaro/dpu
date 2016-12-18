
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <cstdint>
#include <cstring>
#include <stdlib.h>
#include <cstdio>
#include <string>
#include <algorithm>

//#include "llvm/Pass.h"
//#include "llvm/PassSupport.h"
#include "llvm/IR/Module.h"
//#include "llvm/IR/Function.h"
#include "llvm/IR/Verifier.h"
//#include "llvm/IR/IRBuilder.h"
//#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/LLVMContext.h"
//#include "llvm/IR/InstIterator.h"
//#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/TargetSelect.h"
//#include "llvm/ExecutionEngine/ExecutionEngine.h"
//#include "llvm/ExecutionEngine/SectionMemoryManager.h"
//#include "llvm/ExecutionEngine/MCJIT.h"

#include "c15unfold.hh"
#include "pes.hh"
#include "misc.hh"
#include "verbosity.h"

namespace dpu
{

static void _ir_write_ll (const llvm::Module *m, const char *filename)
{
   int fd = open (filename, O_WRONLY | O_TRUNC | O_CREAT, 0644);
   ASSERT (fd >= 0);
   llvm::raw_fd_ostream f (fd, true);
   f << *m;
}


Disset::Disset ()
{
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

   // related to the JIT engine
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

   DEBUG ("c15u: load-bytecode: saving instrumented code to /tmp/output.ll");
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
      cut_to_replay (e->cut, rep);
      add_one_run (rep);
      rep.clear ();
   }
}

void C15unfolder::run_to_completion (Config &c)
{

}

void C15unfolder::explore ()
{
}

void C15unfolder::stream_to_events (Config &c, const action_streamt &s)
{
   // invariant:
   // the stream is a sequence of actions starting from the state given by c

   Event *e, *ee;
   action_stream_itt it (s.begin());
   action_stream_itt end (s.end());
   std::vector<unsigned> pidmap (s.get_rt()->trace.num_ths);

   // we should have at least one action in the stream
   ASSERT (it != end);

   // skip the first context switch to pid 0, if present
   if (it.type () == RT_THCTXSW)
   {
      ASSERT (it.has_id ());
      ASSERT (it.id() == 0);
      it++;
   }

   // for the time being, we assume that the configuration is totally pristine
   ASSERT (c[0] == 0);
   e = u.event (nullptr); // bottom
   c.add (e);
   pidmap[0] = 0;

   while (it != end)
   {
      switch (it.type())
      {
      case RT_MTXLOCK :
         ee = c.mutex_max (it.addr());
         e = u.event ({.type = ActionType::MTXLOCK, .addr = it.addr()}, e, ee);
         c.add (e);
         DEBUG ("");
         break;

      case RT_MTXUNLK :
         ee = c.mutex_max (it.addr());
         e = u.event ({.type = ActionType::MTXUNLK, .addr = it.addr()}, e, ee);
         c.add (e);
         DEBUG ("");
         break;

      case RT_THCTXSW :
         ASSERT (it.id() < pidmap.size()); // pid in bounds
         ASSERT (it.id() == 0 or pidmap[it.id()] != 0); // map is defined
         e = c[pidmap[it.id()]];
         ASSERT (e); // we have at least one event there
         break;

      case RT_THCREAT :
         ASSERT (it.id() < pidmap.size()); // pid in bounds
         ASSERT (it.id() >= 1 and pidmap[it.id()] == 0); // map entry undefined
         // creat
         e = u.event ({.type = ActionType::THCREAT}, e);
         // start
         ee = u.event (e);
         e->action.val = ee->pid();
         pidmap[it.id()] = ee->pid();
         c.add (e);
         c.add (ee);
         DEBUG ("");
         break;

      case RT_THEXIT :
         e = u.event ({.type = ActionType::THEXIT}, e);
         c.add (e);
         DEBUG ("");
         break;

      case RT_THJOIN :
         ASSERT (it.id() < pidmap.size()); // pid in bounds
         ASSERT (it.id() >= 1 and pidmap[it.id()] != 0); // map is defined
         ee = c.mutex_max (pidmap[it.id()]);
         ASSERT (ee and ee->action.type == ActionType::THEXIT);
         e = u.event ({.type = ActionType::THJOIN, .val = pidmap[it.id()]}, e, ee);
         c.add (e);
         DEBUG ("");
         break;

      case RT_RD8 :
         e->redbox.push_back
               ({.type = ActionType::RD8, .addr = it.addr(), .val = *it.val()});
         break;
      case RT_RD16 :
         e->redbox.push_back
               ({.type = ActionType::RD16, .addr = it.addr(), .val = *it.val()});
         break;
      case RT_RD32 :
         e->redbox.push_back
               ({.type = ActionType::RD32, .addr = it.addr(), .val = *it.val()});
         break;
      case RT_RD64 :
         e->redbox.push_back
               ({.type = ActionType::RD64, .addr = it.addr(), .val = *it.val()});
         break;
      case RT_RD128 :
         ASSERT (it.val_size() == 2);
         e->redbox.push_back
               ({.type = ActionType::RD64, .addr = it.addr(), .val = it.val()[0]});
         e->redbox.push_back
               ({.type = ActionType::RD64, .addr = it.addr()+8, .val = it.val()[1]});
         break;

      case RT_WR8 :
         e->redbox.push_back
               ({.type = ActionType::WR8, .addr = it.addr(), .val = *it.val()});
         break;
      case RT_WR16 :
         e->redbox.push_back
               ({.type = ActionType::WR16, .addr = it.addr(), .val = *it.val()});
         break;
      case RT_WR32 :
         e->redbox.push_back
               ({.type = ActionType::WR32, .addr = it.addr(), .val = *it.val()});
         break;
      case RT_WR64 :
         e->redbox.push_back
               ({.type = ActionType::WR64, .addr = it.addr(), .val = *it.val()});
         break;
      case RT_WR128 :
         ASSERT (it.val_size() == 2);
         e->redbox.push_back
               ({.type = ActionType::WR64, .addr = it.addr(), .val = it.val()[0]});
         e->redbox.push_back
               ({.type = ActionType::WR64, .addr = it.addr()+8, .val = it.val()[1]});
         break;

      case RT_ALLOCA :
      case RT_MALLOC :
      case RT_FREE :
      case RT_CALL :
      case RT_RET :
         // not implemented, but doesn't hurt ;)
         break;

      default :
         SHOW (it.type(), "d");
         SHOW (it.addr(), "lu");
         SHOW (it.val()[0], "lu");
         SHOW (it.id(), "u");
         SHOW (it.str(), "s");
         ASSERT (0);
      }

      // get the next action
      it++;
   }
}

void C15unfolder::cut_to_replay (const Cut &c, std::vector<int> &replay)
{
   int nrp, count;
   unsigned i, green;
   bool progress;
   Event *e, *ee;
   Cut cc (u);

   DEBUG ("c15u: cut-to-replay: cut %p", &c);

   // set up the Event's next pointer
   nrp = u.num_procs();
   for (i = 0; i < nrp; i++)
   {
      ee = nullptr;
      for (e = c[i]; e; ee = e, e = e->pre_proc())
         e->next = ee;
   }

   // we use a second cut cc to run forward up to completion of cut c
   // we start by adding bottom to the cut
   cc.add (u.proc(0)->first_event());
   ASSERT (cc[0] and cc[0]->is_bottom ());

   // get a fresh color, we call it green
   green = u.get_fresh_color ();

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
         for (e = cc[i]; e; e = e->next)
         {
            // if event has non-visited causal predecessor, abort this process
            if (e->pre_other() and e->pre_other()->color != green) break;
            e->color = green;
            count++;
            if (e->action.type == ActionType::THCREAT)
            {
               // put in the cut the THSTART corresponding to a THCREAT if the
               // original cut contains at least one event from that process
               if (c[e->action.val]) cc[e->action.val] = u.event (e);
            }
         }

         if (count)
         {
            cc[i] = e;
            replay.push_back (i);
            replay.push_back (count);
            progress = true;
         }
      }
   }
   replay.push_back (-1);
}

/// Compute conflicting extension for a LOCK
void C15unfolder::compute_cex_lock (Event *e, Event **head)
{
   Event * ep, * em, *pr_mem, *newevt;

   DEBUG ("c15u: cex-lock: e %p *head %p", e, *head);

   ep = e->pre_proc();
   em = e->pre_other();
   if (em == nullptr)
   {
      DEBUG ("c15u: cex-lock: pre-other null, returning");
      return;
   }

   // while ((em != nullptr) and (ep->vclock < em->vclock)) //em is not a predecessor of ep
   while (em)
   {
//      if (em->vclock < ep->vclock)
      if (em->is_pred_of(ep)) // excluding em = ep
      {
         DEBUG("c15u: cex-lock: em < ep, returning");
         return;
      }

      // jump 2 predecessors back
      ASSERT (em->pre_other());
      pr_mem = em->pre_other()->pre_other();

      //DEBUG("pr_mem: %p", pr_mem);

      /// The first LOCK's pre_other is nullptr
      if (pr_mem == nullptr)
      {
//         if (ep->vclock > em->pre_other()->vclock)
         if (em->pre_other()->is_pred_of(ep))
         {
            DEBUG("   pr_mem is nil and a predecessor of ep => exit");
            break;
         }

         newevt = u.event(e->action, ep, pr_mem);
         DEBUG("New event created:");
         DEBUG ("  e %-16p pid %2d pre-proc %-16p pre-other %-16p fst/lst %d/%d action %s",
                  newevt, newevt->pid(), newevt->pre_proc(), newevt->pre_other(),
                  newevt->flags.boxfirst ? 1 : 0,
                  newevt->flags.boxlast ? 1 : 0,
                  action_type_str (newevt->action.type));

         // we add newevt to the list
         newevt->next = *head;
         *head = newevt;

         break;
      }

      // Check if pr_mem < ep

//      if (ep->vclock > pr_mem->vclock)
      if (em->is_pred_of(ep))
      {
         DEBUG("   pr_mem is predecessor of ep");
         return;
      }

      Event * newevt = u.event(e->action, ep, pr_mem);
      // need to add newevt to cex
      DEBUG("New event created:");
      DEBUG ("  e %-16p pid %2d pre-proc %-16p pre-other %-16p fst/lst %d/%d action %s",
               newevt, newevt->pid(), newevt->pre_proc(), newevt->pre_other(),
               newevt->flags.boxfirst ? 1 : 0,
               newevt->flags.boxlast ? 1 : 0,
               action_type_str (newevt->action.type));

      // we add newevt to the list
      newevt->next = *head;
      *head = newevt;


      // move the pointer to the next
      em = pr_mem;
   }

   DEBUG ("c15u: cex-lock: done!");
}

void C15unfolder::compute_cex (Config &c, Event **head)
{
   Event *e;

   DEBUG("c15u: cex: c %p *head %p |mm| %d", &c, *head, c.mutexmax.size());

   for (auto const & max : c.mutexmax)
   {
      // skip events that not locks or unlocks
      e = max.second;
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

#if 0
/*
 * check if all elements in combin are conflict-free in pairs
 */
bool C15unfolder::is_conflict_free(std::vector<Event *> eset)
{
   for (unsigned i = 0; i < eset.size() - 1; i++)
     for (unsigned j = i; j < eset.size(); j++)
      {
        if (eset[i]->in_cfl_with(eset[j]))
           return false;
      }
   return true;
}

///*
// * Retrieve all events in local configuration of an event
// */
//const std::vector<Event *> Event::local_config() const
//{
//   DEBUG("Event.local_config");
//   Event * next;
//   std::vector<Event *> lc;
//   //DEBUG("Proc_maxevt.size = %d", proc_maxevt.size());
//
//   for (unsigned i = 0; i < proc_maxevt.size(); i++)
//   {
//      next = proc_maxevt[i];
//      while (!next->is_bottom())
//      {
//         //DEBUG("push back to LC");
//         lc.push_back(next);
//         next = next->evtid.pre_proc;
//      }
//   }
///*
//   DEBUG("LC inside the function");
//   for (unsigned j = 0; j < lc.size(); j++)
//      DEBUG_("%d ", lc[j]->idx);
//*/
//   return lc;
//}

/// enumerate the comb for spike i. The temporatory combination is stored in temp.
/// If there exist a config satisfied, it will be assigned to J
void C15unfolder::enumerate_combination (unsigned i, std::vector<std::vector<Event *>> comb , std::vector<Event*> temp, Config &J)
{
   DEBUG("This is enum_combination function");

   ASSERT(!comb.empty());

   for (unsigned j = 0; j < comb[i].size(); j++ )
   {
      if (j < comb[i].size())
      {
         temp.push_back(comb[i][j]);

         if (i == comb.size() - 1) // get a full combination
         {
            DEBUG_("temp = {");
            for (unsigned i = 0; i < temp.size(); i++)
               DEBUG_("%d, ", temp[i]->idx);

            DEBUG("}");


//            /*
//             * Check if two or more events in J are the same -> dont need
//             * To maintain J, copy J to Jcopy
//             */
//
//            std::vector<Event *>  tempcopy = temp;
//
//            DEBUG("Remove duplica in tempcopy");
//
//            for (unsigned i = 0; i < tempcopy.size(); i++)
//            {
//               tempcopy[i]->inside++;
//               if (tempcopy[i]->inside >1)
//               {
//                  // remove J[j]
//                  tempcopy[i]->inside--;
//                  tempcopy[i] = tempcopy.back();
//                  tempcopy.pop_back();
//                  i--;
//               }
//            }
//
//            //set all inside back to 0 for next usage
//            for (unsigned i = 0; i < tempcopy.size(); i++)
//               tempcopy[i]->inside = 0;


//            DEBUG_("tempcopy = {");
//            for (unsigned i = 0; i < tempcopy.size(); i++)
//               DEBUG_("%d, ", tempcopy[i]->idx);
//            DEBUG("}");

            /*
             * If temp is conflict-free, then J is assigned to union of every element's local configuration.
             */
            if (is_conflict_free(temp))
            {
               DEBUG(": a conflict-free combination");
               /// Don't need to add local config to J because J is now a config and only store maximal events of config.
               /*
                * A is the union of local configuration of all events in J
                */

//               for (unsigned i = 0; i < tempcopy.size(); i++)
//               {
////                     const std::vector<Event *> & tp = tempcopy[i]->local_config(); // DON'T NEED ANYMORE
//
//                  //DEBUG("LC.size = %d", tp.size());
////                     for (unsigned j = 0; j < tp.size(); j++)
////                       j.push_back(tp[j]);
//                  DEBUG("%d",tempcopy[i]->pid());
////                     (j + tempcopy[i]->pid()) = tempcopy[i]; // update j.cut.max
//                  J.add(tempcopy[i]);
//               }

//                  DEBUG_("J = {");
//                  for (unsigned i = 0; i < j.size(); i++)
//                     DEBUG_("%d, ", j[i]->idx);
//                  DEBUG("}");

               /// just do add each element in tempcopy to J

               for (unsigned i = 0; i < temp.size(); i++)
                  J.add(temp[i]);

               J.dump();
               return; // go back to find_alternative

               /*
               * Check if two or more events in J are the same: by updating cut of a config, don't need to check duplica any more
               */

//                 DEBUG("Remove duplica in temp");
//
//                 for (unsigned i = 0; i < tempcopy.size(); i++)
//                 {
//                    tempcopy[i]->inside++;
//                    if (tempcopy[i]->inside >1)
//                    {
//                       // remove A[i]
//                       tempcopy[i]->inside--;
//                       tempcopy[i] = tempcopy.back();
//                       tempcopy.pop_back();
//                       i--;
//                    }
//                 }
//
//              //set all inside back to 0 for next usage
//              for (unsigned i = 0; i < tempcopy.size(); i++)
//                 tempcopy[i]->inside = 0;


            }
            else
               DEBUG(": not conflict-free");
         }
         else
            enumerate_combination(i+1, comb, temp, J);
      }

      temp.pop_back();
   }
   DEBUG ("c15u: cex: done!");
}

bool C15unfolder::find_alternative (Config &c, std::vector<Event*> d, Cut &J)
{
      ASSERT (d.size ());

      J.clear();

      // huyen here

      std::vector<std::vector<Event *>> comb;

       DEBUG(" Find an alternative J to D after C: ");
       DEBUG_("   Original D = {");
         for (unsigned i = 0; i < d.size(); i++)
            DEBUG_("%d  ", d[i]->idx);
       DEBUG("}");

       for (auto ce : c.cex)
          ce->inside = 1;

       for (unsigned i = 0; i < d.size(); i++)
       {
          if (d[i]->inside == 1)
          {
             //remove D[i]
             d[i] = d.back();
             d.pop_back();
             i--;
          }
       }

       // set all inside back to 0
       for (auto ce : c.cex)
          ce->inside = 0;


       DEBUG_("   Prunned D = {");
       for (unsigned i = 0; i < d.size(); i++) DEBUG_("%d  ", d[i]->idx);
       DEBUG("}");

       DEBUG_("   After C = ");
       c.dump();
       /*
        *  D now contains only events which is in en(C).
        *  D is a comb whose each spike is a list of conflict events D[i].dicfl
        *  pour those in dicfl of all events in D into a comb whose each spike is corresponding to an event's dicfl
        */


       //DEBUG("D.size: %d", D.size());
       for (unsigned i = 0; i < d.size(); i++)
          comb.push_back(d[i]->dicfl);

       DEBUG("COMB: %d spikes: ", comb.size());
         for (unsigned i = 0; i < comb.size(); i++)
         {
            DEBUG_ ("  spike %d: (#e%d (len %d): ", i, d[i]->idx, comb[i].size());
            for (unsigned j = 0; j < comb[i].size(); j++)
               DEBUG_(" %d", comb[i][j]->idx);
            DEBUG("");
         }
       DEBUG("END COMB");


       /// remove from the comb those are also in D
       for (unsigned i = 0; i < comb.size(); i++)
       {
          DEBUG("   For comb[%d]:", i);

          unsigned j = 0;
          bool removed = false;

          DEBUG_("    Remove from comb[%d] those which are already in D: ", i);

          while ((comb[i].size() != 0) and (j < comb[i].size()) )
          {
             // check if there is any event in any spike already in D. If yes, remove it from spikes
             for (unsigned k = 0; k < d.size(); k++)
             {
                if (comb[i][j] == d[k])
                {
                   DEBUG(" %d ", comb[i][j]->idx);
                   //remove spike[i][j]
                   comb[i][j] = comb[i].back();
                   comb[i].pop_back();
                   removed = true;
                   DEBUG("Remove done");
                   break;
                }
             }

             // if we removed some event in spikes[i] by swapping it with the last one, then repeat the task with new spike[i][j]
             if (!removed)
                j++;
             else
                removed = false;

          }

          DEBUG("Done.");

          if (comb[i].size() == 0)
          {
             DEBUG("\n    comb %d (e%d) is empty: no alternative; returning.", i, d[i]->idx);
             return false;
          }


          /*
           *  Remove from spikes those are in conflict with any maxinal event
           *  Let's reconsider set of maximal events
           */
          removed = false;
          j = 0;
          Event * max;

          DEBUG("\n    Remove from comb[%d] events cfl with maximal events", i);

          while ((comb[i].size() != 0) and (j < comb[i].size()) )
          {
             for (int i = 0; i < c.num_procs(); i++)
             {
                max = c.proc_max(i); // get maximal event for proc i
                if (comb[i][j]->in_cfl_with(max))
                {
                      //remove spike[i][j]
                   DEBUG_("    %d cfl with %d", comb[i][j]->idx, max->idx);
                   DEBUG("->Remove %d", comb[i][j]->idx);
                   comb[i][j] = comb[i].back();
                   comb[i].pop_back();
                   removed = true;
                   break;
                }
             }

             // if we removed some event in spikes[i] by swapping it with the last one,
             // then repeat the task with new spike[i][j] which means "do not increase j"
             if (!removed)
                j++;
             else
                removed = false;
          }


          DEBUG("Done.");
          if (comb[i].size() == 0)
          {
             DEBUG("    Comb %d (e%d) is empty: no alternative; returning.", i, d[i]->idx);
             return false;
          }

       } // end for


       /// show the comb
       ASSERT (comb.size() == d.size ());
       DEBUG("COMB: %d spikes: ", comb.size());
       for (unsigned i = 0; i < comb.size(); i++)
       {
          DEBUG_ ("  spike %d: (#e%d (len %d): ", i, d[i]->idx, comb[i].size());
          for (unsigned j = 0; j < comb[i].size(); j++)
             DEBUG_(" %d", comb[i][j]->idx);
          DEBUG("");
       }
       DEBUG("END COMB");


       if (comb.size() == 0)
       {
          DEBUG("Empty comb");
          return false;
       }

       else
       {
          std::vector<Event *> temp;
          enumerate_combination(0, comb, temp, J);
          if (J.is_empty())
             return false;
          else
             return true;
       }

       return false;
}
#endif

} // namespace dpu
