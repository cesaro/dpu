///* Get a replay of a configuration */
#include "por.hh"

#include <vector>

namespace dpu
{

void basic_conf_to_replay (Unfolding &u, Cut &c, std::vector<int> &replay)
{
   int nrp;
   unsigned i;
   Event * pe;

   DEBUG("==========Get replay====");
   ASSERT (u.num_procs() == c.num_procs());
   nrp = c.num_procs();

   for (i = 0; i < nrp; i++)
   {
      pe = c[i];
      // skip null pointers
      if (!pe) continue;

      // set up the pointer next in all events of one process
      pe->next = nullptr;
      while (pe->pre_proc())
      {
         pe->color = 0;
         pe->pre_proc()->next = pe; // next event in the same process
         pe = pe->pre_proc();
      }
   }

   bool unmarked = true; // there is some event in the configuration unmarked
   while (unmarked)
   {
      for (unsigned i = 0; i < nrp; i++)
      {
         pe = u.proc(i)->first_event();
//         DEBUG("%s type", action_type_str(pe->action.type));

         while (pe)
         {
            if (pe->color == 1)
               pe = pe->next;
            else
               if (((pe->pre_other() == nullptr) || (pe->pre_other()->color == 1)))
               {
//                  DEBUG("pre_other marked");
//                  DEBUG("pe->pid: %d", pe->pid());

                  if ((replay.empty()) or (replay.at(replay.size()-2) != pe->pid()))
                  {
                     replay.push_back(pe->pid());
                     replay.push_back(1);
                  }
                  else
                  {
                     replay.back()++;
                  }

                  pe->color = 1;
                  pe = pe->next;
               }
               else
                  break; // break while, then move to next process
         }
      }

      // terminate when all maximal events are marked.
      // FIXME there is a bug here - improve this using a counter in the
      // previous while loop
      unsigned int j;
      for (j = 0; j < nrp; j++)
         if (c[j]->color == 0)
            break;

      if (j == nrp)
         unmarked = false;
   }

//   DEBUG("=====End of computing replay====");

}
/// Compute conflicting extension for a LOCK
void LOCK_cex(Unfolding &u, Event *e)
{
   DEBUG("\n %p, id:%d: LOCK_cex", e);
   Event * ep, * em, *pr_mem;
   ep = e->pre_proc();
   em = e->pre_other();

   while (!(em->is_bottom()) and (em->vclock > ep->vclock)) //em is not a predecessor of ep
   {
      pr_mem = em->pre_other()->pre_other(); // skip 2

      /*
       * Check if pr_mem < ep
       */
      if (ep->vclock > pr_mem->vclock)
         return;
      /*
       *  Need to check the event's history before adding it to the unf
       * Don't add events which are already in the unf
       */

      Event * newevt = u.event(e->action, ep, pr_mem);
      DEBUG("New event:");
      DEBUG ("  e %-16p pid %2d pre-proc %-16p pre-other %-16p fst/lst %d/%d action %s",
               newevt, newevt->pid(), newevt->pre_proc(), newevt->pre_other(),
               newevt->flags.boxfirst ? 1 : 0,
               newevt->flags.boxlast ? 1 : 0,
               action_type_str (newevt->action.type));


      // move the pointer to the next
      em = em->pre_other()->pre_other();
   }
}

void compute_cex(Unfolding & u, Cut & c)
{
   DEBUG("==========Compute cex====");
   Event *e;
   int size = u.num_procs();
   for (unsigned i = 0; i < size; i++)
   {
      e = c[i];
      while (e->action.type == ActionType::THSTART)
      {
         if (e->action.type == ActionType::MTXLOCK)
            LOCK_cex(u,e);
         e = e->pre_proc();
      }
   }
}

} // end of namespace
