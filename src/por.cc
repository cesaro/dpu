///* Get a replay of a configuration */
#include "por.hh"

#include <vector>

namespace dpu
{

void cut_to_replay (Unfolding &u, Cut &c, std::vector<int> &replay)
{
   int nrp, count;
   unsigned i, green;
   bool progress;
   Event *e, *ee;
   Cut cc (u);

   ASSERT (u.num_procs() == c.num_procs());
   nrp = c.num_procs();

   // set up the Event's next pointer
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
         //DEBUG ("Context switch to proc %d, event %p", i, e);
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

///// Compute conflicting extension for a LOCK
//void LOCK_cex (Unfolding &u, Event *e)
//{
//   DEBUG("\n %p: LOCK_cex", e);
//   Event * ep, * em, *pr_mem, *newevt;
//   ep = e->pre_proc();
//   em = e->pre_other();
//
//   if (em == nullptr)
//   {
//      DEBUG("   No conflicting event");
//      return;
//   }
//
//   ASSERT(em)
//
//  // while ((em != nullptr) and (ep->vclock < em->vclock)) //em is not a predecessor of ep
//   while (em)
//   {
////      if (em->vclock < ep->vclock)
//      if (em->is_pred_of(ep)) // excluding em = ep
//      {
//         DEBUG("em is a predecessor of ep");
//         break;
//      }
//
//      pr_mem = em->pre_other()->pre_other(); // skip 2
//
//      //DEBUG("pr_mem: %p", pr_mem);
//
//      /// The first LOCK's pre_other is nullptr
//      if (pr_mem == nullptr)
//      {
////         if (ep->vclock > em->pre_other()->vclock)
//         if (em->pre_other()->is_pred_of(ep))
//         {
//            DEBUG("   pr_mem is nil and a predecessor of ep => exit");
//            break;
//         }
//
//         newevt = u.event(e->action, ep, pr_mem);
//         DEBUG("New event created:");
//         DEBUG ("  e %-16p pid %2d pre-proc %-16p pre-other %-16p fst/lst %d/%d action %s",
//                  newevt, newevt->pid(), newevt->pre_proc(), newevt->pre_other(),
//                  newevt->flags.boxfirst ? 1 : 0,
//                  newevt->flags.boxlast ? 1 : 0,
//                  action_type_str (newevt->action.type));
//
//         // need to add newevt to cex
//
//         break;
//      }
//
//      ///Check if pr_mem < ep
//
////      if (ep->vclock > pr_mem->vclock)
//      if (em->is_pred_of(ep))
//      {
//         DEBUG("   pr_mem is predecessor of ep");
//         return;
//      }
//
//      Event * newevt = u.event(e->action, ep, pr_mem);
//      /// need to add newevt to cex
//      DEBUG("New event created:");
//      DEBUG ("  e %-16p pid %2d pre-proc %-16p pre-other %-16p fst/lst %d/%d action %s",
//               newevt, newevt->pid(), newevt->pre_proc(), newevt->pre_other(),
//               newevt->flags.boxfirst ? 1 : 0,
//               newevt->flags.boxlast ? 1 : 0,
//               action_type_str (newevt->action.type));
//
//      /// move the pointer to the next
//      em = pr_mem;
//   }
//
//   DEBUG("   Finish LOCK_cex");
//}
//
//void compute_cex (Unfolding &u, Config &c)
//{
//   unsigned i;
//   Event *e;
//
//   DEBUG("==========Compute cex====");
//
//   // FIXME -- Cesar: improve this to use only the address trees in c.mutexmax
//   for (i = 0; i < c.num_procs(); i++)
//   {
//      for (e = c[i]; e; e = e->pre_proc())
//      {
//         if (e->action.type == ActionType::MTXLOCK)
//         {
////            DEBUG("e: %p, type: %s",e, action_type_str(e->action.type));
//            LOCK_cex(u, e);
//         }
//      }
//   }
//}
#if 0
/// Check conflict between two events in the same tree depending on the idx
bool check_cfl_same_tree(int idx, const Event & e1, const Event & e2)
{
   DEBUG("Check conflict in the same tree");
   int d1, d2;
   ASSERT(e1.pid() == e2.pid() || (e1.action.addr == e2.action.addr));

   d1 = e1.node[idx].depth;
   d2 = e2.node[idx].depth;

   DEBUG("d1 = %d, d2 = %d", d1, d2);
   //one of event is the bottom
   if ((d1 == 0) or (d2 == 0))
   {
      //DEBUG("One is the bottom -> %d and %d not cfl", e1.idx, e2.idx);
      return false;
   }

   if (d1 == d2)
      return not(&e1 == &e2); // same event -> not conflict and reverse

   if (d1 > d2)
   {
      if (e2.is_pred_of(&e1)) // refer to the same event
      {
         DEBUG("2 events are in causality");
         return false;
      }
      else
      {
         DEBUG("in conflict");
         return true;
      }
   }
   else
   {
      //DEBUG("Here d2 > d1");
      if (e1.is_pred_of(&e2)) // refer to the same event
      {
         DEBUG("in causality");
         return false;
      }
      else
      {
         DEBUG("in conflict");
         return true;
      }

   }
}

//======================
bool check_2difs(Event & e1, Event & e2)
{
   return false;
}

/*
 *  check conflict between two events
 */
bool check_cfl(Event & e1, Event & e2 )
{
   if (e1.is_bottom() or e2.is_bottom())
      return false;

   if (&e1 == &e2) // same event
      return false;

   if (e1.proc() == e2.proc())
   {
      DEBUG("They are in the same process");
//      DEBUG("check cfl same tree returns %", check_cfl_same_tree<0>(e1,e2));
      return check_cfl_same_tree(0,e1,e2);
   }

   if ( ( (e1.action.type == ActionType::MTXLOCK) || (e1.action.type == ActionType::MTXLOCK) ) and ( (e2.action.type == ActionType::MTXLOCK) || (e2.action.type == ActionType::MTXLOCK) )
         and (e1.action.addr == e2.action.addr))
   {
      DEBUG("%p and %p are 2 LOCK and UNLK", &e1, &e2);
      return check_cfl_same_tree(1,e1,e2);
   }

   // other cases
   DEBUG("Other cases");
  return check_2difs(e1,e2);
}
#endif

bool is_conflict_free(std::vector<Event *> combin)
{
   for (unsigned i = 0; i < combin.size() - 1; i++)
     for (unsigned j = i; j < combin.size(); j++)
      {
        if (combin[i]->in_cfl_with(combin[j]))
           return false;
      }
   return true;
}

/*
 * compute all possible conbinations and return a set J which is a possible alternative to extend from C
 */
void compute_alt(unsigned int i, const std::vector<std::vector<Event *>> & s, std::vector<Event *> & J, std::vector<Event *> & A)
{
   DEBUG("This is compute_alt function");

   ASSERT(!s.empty());

   for (unsigned j = 0; j < s[i].size(); j++ )
   {
      if (j < s[i].size())
      {
         J.push_back(s[i][j]);

         if (i == s.size() - 1)
         {
            DEBUG_("J = {");
            for (unsigned i = 0; i < J.size(); i++)
               DEBUG_("%d, ", J[i]->idx);

            DEBUG("}");


            /*
             * Check if two or more events in J are the same
             * To maintain J, copy J to Jcopy
             */

            std::vector<Event *>  Jcopy = J;

            DEBUG("Remove duplica in Jcopy");

            for (unsigned i = 0; i < Jcopy.size(); i++)
            {
               Jcopy[i]->inside++;
               if (Jcopy[i]->inside >1)
               {
                  // remove J[j]
                  Jcopy[i]->inside--;
                  Jcopy[i] = Jcopy.back();
                  Jcopy.pop_back();
                  i--;
               }
            }

            //set all inside back to 0 for next usage
            for (unsigned i = 0; i < Jcopy.size(); i++)
               Jcopy[i]->inside = 0;

            DEBUG_("Jcopy = {");
            for (unsigned i = 0; i < Jcopy.size(); i++)
               DEBUG_("%d, ", Jcopy[i]->idx);
            DEBUG("}");

            /*
             * If J is conflict-free, then A is assigned to union of every element's local configuration.
             */
            if (is_conflict_free(Jcopy))
            {
               DEBUG(": a conflict-free combination");
               /*
                * A is the union of local configuration of all events in J
                */
               //left later
//
//               for (unsigned i = 0; i < Jcopy.size(); i++)
//               {
//                  const std::vector<Event *> & tp = Jcopy[i]->local_config();
//
//                  //DEBUG("LC.size = %d", tp.size());
//                  for (unsigned j = 0; j < tp.size(); j++)
//                     A.push_back(tp[j]);
//               }

               DEBUG_("A = {");
               for (unsigned i = 0; i < A.size(); i++)
                  DEBUG_("%d, ", A[i]->idx);
               DEBUG("}");

              /*
               * Check if two or more events in A are the same
               */

              DEBUG("Remove duplica in A");

              for (unsigned i = 0; i < A.size(); i++)
              {
                 A[i]->inside++;
                 if (A[i]->inside >1)
                 {
                    // remove A[i]
                    A[i]->inside--;
                    A[i] = Jcopy.back();
                    A.pop_back();
                    i--;
                 }
              }

              //set all inside back to 0 for next usage
              for (unsigned i = 0; i < A.size(); i++)
                 A[i]->inside = 0;

              return; // go back to
            }
            else
            {
               DEBUG(": not conflict-free");
            }
         }
         else
            compute_alt(i+1, s, J, A);
      }

      J.pop_back();
   }
}

/*
 * Try to find an alternative if it exists
 */
void find_an_alternative(Config & C, std::vector<Event *> D, std::vector<Event *> & J, std::vector<Event *> & A ) // we only want to modify D in the scope of this function
{
   std::vector<std::vector<Event *>> spikes;

   DEBUG(" Find an alternative J to D after C: ");
   DEBUG_("   Original D = {");
     for (unsigned i = 0; i < D.size(); i++)
        DEBUG_("%d  ", D[i]->idx);
   DEBUG("}");

   /// Remove from D those that are in C.cex
   for (auto c : C.cex)
      c->inside = 1;

   for (unsigned i = 0; i < D.size(); i++)
   {
      if (D[i]->inside == 1)
      {
         //remove D[i]
         D[i] = D.back();
         D.pop_back();
         i--;
      }
   }
   // set all inside back to 0
   for (auto c : C.cex)
      c->inside = 0;


   DEBUG_("   Prunned D = {");
   for (unsigned i = 0; i < D.size(); i++) DEBUG_("%d  ", D[i]->idx);
   DEBUG("}");

   DEBUG_("   After C = ");
   C.dump();

   /*
    *  D now contains only events which is in en(C).
    *  D is a comb whose each spike is a list of conflict events D[i].dicfl
    */

   //DEBUG("D.size: %d", D.size());
   for (unsigned i = 0; i < D.size(); i++)
//      spikes.push_back(D[i]->dicfl);

   /// print the comb achieved
   DEBUG("COMB: %d spikes: ", spikes.size());
     for (unsigned i = 0; i < spikes.size(); i++)
     {
        DEBUG_ ("  spike %d: (#e%d (len %d): ", i, D[i]->idx, spikes[i].size());
        for (unsigned j = 0; j < spikes[i].size(); j++)
           DEBUG_(" %d", spikes[i][j]->idx);
        DEBUG("");
     }
   DEBUG("END COMB");

   /*
    * - Remove from spikes those are already in D
    */
   for (unsigned i = 0; i < spikes.size(); i++)
   {
      DEBUG("   For spike[%d]:", i);


      unsigned j = 0;
      bool removed = false;

      DEBUG_("    Remove from spikes[%d] those which are already in D: ", i);

      while ((spikes[i].size() != 0) and (j < spikes[i].size()) )
      {
         // check if there is any event in any spike already in D. If yes, remove it from spikes
         for (unsigned k = 0; k < D.size(); k++)
         {
            if (spikes[i][j] == D[k])
            {
               DEBUG(" %d ", spikes[i][j]->idx);
               //remove spike[i][j]
               spikes[i][j] = spikes[i].back();
               spikes[i].pop_back();
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

      if (spikes[i].size() == 0)
      {
         DEBUG("\n    Spike %d (e%d) is empty: no alternative; returning.", i, D[i]->idx);
         return;
      }


      /*
       *  Remove events that are in conflict with any maximal event
       *  Let's reconsider set of maximal events
       */
      removed = false;
      j = 0;

      DEBUG("\n    Remove from spikes[%d] events cfl with maximal events", i);

      while ((spikes[i].size() != 0) and (j < spikes[i].size()) )
      {
         for (int k = 0; k < C.num_procs(); k++)
         {
            if (spikes[i][j]->in_cfl_with(C[k]))
            {
                  //remove spike[i][j]
               DEBUG_("    %p cfl with %p", spikes[i][j], C[k]);
               DEBUG("->Remove %d", spikes[i][j]->idx);
               spikes[i][j] = spikes[i].back();
               spikes[i].pop_back();
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
      if (spikes[i].size() == 0)
      {
         DEBUG("    Spike %d (e%d) is empty: no alternative; returning.", i, D[i]->idx);
         return;
      }

   } // end for spike[i]


   ASSERT (spikes.size() == D.size ());
   DEBUG("COMB: %d spikes: ", spikes.size());
   for (unsigned i = 0; i < spikes.size(); i++)
   {
      DEBUG_ ("  spike %d: (#e%d (len %d): ", i, D[i]->idx, spikes[i].size());
      for (unsigned j = 0; j < spikes[i].size(); j++)
         DEBUG_(" %d", spikes[i][j]->idx);
      DEBUG("");
   }
   DEBUG("END COMB");

   if (spikes.size() == 0)
   {
      DEBUG("Empty spikes");
      return;
   }

   else
      compute_alt(0, spikes, J, A);
}

} // end of namespace

