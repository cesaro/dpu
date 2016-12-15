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

   ASSERT (u.num_procs() <= c.num_procs());
   nrp = u.num_procs();

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


/// Compute conflicting extension for a LOCK
void LOCK_cex (Unfolding &u, Event *e)
{
   DEBUG("\n %p: LOCK_cex", e);
   Event * ep, * em, *pr_mem, *newevt;
   ep = e->pre_proc();
   em = e->pre_other();

   if (em == nullptr)
   {
      DEBUG("   No conflicting event");
      return;
   }

   ASSERT(em)

  // while ((em != nullptr) and (ep->vclock < em->vclock)) //em is not a predecessor of ep
   while (em)
   {
      if (em->is_pred_of(ep)) // excluding em = ep
      {
//         DEBUG("em is a predecessor of ep");
         break;
      }

      pr_mem = em->pre_other()->pre_other(); // skip 2

      //DEBUG("pr_mem: %p", pr_mem);

      /// The first LOCK's pre_other is nullptr
      if (pr_mem == nullptr)
      {
         if (em->pre_other()->is_pred_of(ep))
         {
//            DEBUG("   pr_mem is nil and a predecessor of ep => exit");
            break;
         }

         newevt = u.event(e->action, ep, pr_mem);
         DEBUG("New event created:");
         DEBUG ("  e %-16p pid %2d pre-proc %-16p pre-other %-16p fst/lst %d/%d action %s",
                  newevt, newevt->pid(), newevt->pre_proc(), newevt->pre_other(),
                  newevt->flags.boxfirst ? 1 : 0,
                  newevt->flags.boxlast ? 1 : 0,
                  action_type_str (newevt->action.type));

         // need to add newevt to cex
         em->pre_other()->dicfl.push_back(newevt);
         newevt->dicfl.push_back(em->pre_other());
         break;
      }

      ///Check if pr_mem < ep

      if (em->is_pred_of(ep))
      {
//         DEBUG("   pr_mem is predecessor of ep");
         return;
      }

      Event * newevt = u.event(e->action, ep, pr_mem);
      /// need to add newevt to cex
      DEBUG("New event created:");
      DEBUG ("  e %-16p pid %2d pre-proc %-16p pre-other %-16p fst/lst %d/%d action %s",
               newevt, newevt->pid(), newevt->pre_proc(), newevt->pre_other(),
               newevt->flags.boxfirst ? 1 : 0,
               newevt->flags.boxlast ? 1 : 0,
               action_type_str (newevt->action.type));

      // need to add newevt to cex
      em->pre_other()->dicfl.push_back(newevt);
      newevt->dicfl.push_back(em->pre_other());

      /// move the pointer to the next
      em = pr_mem;
   }

   DEBUG("   Finish LOCK_cex");
}

void compute_cex (Unfolding &u, Config &c)
{
//   unsigned nrp = c.num_procs();
   Event *e;

   DEBUG("\n==========Compute cex====");

 //   DEBUG("%d", c.mutexmax.size());
   for (auto const & max : c.mutexmax)
   {
      for (e = max.second; e; e = e->pre_other())
      {
//         DEBUG("max.second %p", max.second);
         if (e->action.type == ActionType::MTXLOCK)
         {
            DEBUG("e: %p, type: %s",e, action_type_str(e->action.type));
            LOCK_cex(u,e);
         }
      }
   }
}
/*
 * check if all elements in combin are conflict-free in pairs
 */
bool is_conflict_free(std::vector<Event *> eset)
{
   for (unsigned i = 0; i < eset.size() - 1; i++)
     for (unsigned j = i; j < eset.size(); j++)
      {
        if (eset[i]->in_cfl_with(eset[j]))
           return false;
      }
   return true;
}

/// enumerate the comb for spike i. The temporatory combination is stored in temp.
/// If there exist a config satisfied, it will be assigned to J
void enumerate_combination (unsigned i, std::vector<std::vector<Event *>> comb , std::vector<Event*> temp, Config &J)
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



            /*
             * If temp is conflict-free, then J is assigned to union of every element's local configuration.
             */
            if (is_conflict_free(temp))
            {
               DEBUG(": a conflict-free combination");

               /// just do add each element in temp to J

               for (unsigned i = 0; i < temp.size(); i++)
                  J.add(temp[i]); // BUG here

               J.dump();
               return; // go back to find_alternative

               /*
               * Check if two or more events in J are the same: by updating cut of a config, don't need to check duplica any more
               */

            }
            else
               DEBUG(": not conflict-free");
         }
         else
            enumerate_combination(i+1, comb, temp, J);
      }

      temp.pop_back();
   }

}

bool find_alternative (Config &c, std::vector<Event*> d, Config &J)
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


       DEBUG_("   Prunned  D = {");
       for (unsigned i = 0; i < d.size(); i++) DEBUG_("%d  ", d[i]->idx);
       DEBUG("}");

       DEBUG_("   After C \n ");
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

          DEBUG("\n    Remove from comb[%d] events cfl with maximal events in C", i);

          while ((comb[i].size() != 0) and (j < comb[i].size()) )
          {
             for (int i = 0; i < c.num_procs(); i++)  //FIXME HERE SEG FAULT
             {
                DEBUG("lAN THU %d", i);
                max = c.proc_max(i); // get maximal event for proc i
                DEBUG("%p", max);

                if (max == nullptr) continue;

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
                else
                   DEBUG("Nothing");
             }

             DEBUG("Done.");
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


} // end of namespace

