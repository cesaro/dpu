///* Get a replay of a configuration */
#include "por.hh"

#include <vector>

namespace dpu
{

void basic_conf_to_replay (Unfolding &u, BaseConfig &c, std::vector<int> &replay)
{
   int size;
   unsigned i;
   Event * pe;

   DEBUG("==========basic_conf_to_replay====");
   ASSERT (u.num_procs() == c.size);
   size = c.size;

   // set up the pointer next in all events of one process

   for (i = 0; i < c.size; i++)
   {
      pe = c.max[i];
      // skip null pointers
      if (!pe) continue;

      pe->next = nullptr;
      while (pe->pre_proc())
      {
         pe->color = 0;
         pe->pre_proc()->next = pe; // next event in the same process
         pe = pe->pre_proc();
      }
   }

   bool unmarked = true; // there is some event in the configuration unmarked
   int counter = 0;
   while (unmarked)
   {
      for (unsigned i = 0; i < size; i++)
      {
         DEBUG("Proc %d", i);
         if (!c.max[i])
         {
            counter++;
            continue;
         }
         pe = u.proc(i)->first_event();
         ASSERT(pe);
//         FIXME: proc 1 has no event but the first_event() still returns a THSTART
         DEBUG("%s type", action_type_str(pe->action.type));

         while (pe)
         {
            if (pe->color == 1)
               pe = pe->next;
            else
               if (((pe->pre_other() == nullptr) || (pe->pre_other()->color == 1)))
               {
//                  DEBUG("pre_other marked or nil-> add");
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

                  if (pe == nullptr)
                     counter++;
               }
               else
                  break; // break while, then move to next process
         }
      }

      // terminate when all maximal events are marked.
      // FIXME there is a bug here - improve this using a counter in the
      // previous while loop
      if (counter == size)
         unmarked = false;
   }

//   DEBUG("=====End of computing replay====");

}
/// Compute conflicting extension for a LOCK
void LOCK_cex(Unfolding &u, Event *e)
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
      if (em->vclock < ep->vclock)
      {
         DEBUG("em is a predecessor of ep");
         break;
      }

      pr_mem = em->pre_other()->pre_other(); // skip 2

      //DEBUG("pr_mem: %p", pr_mem);

      /// The first LOCK's pre_other is nullptr
      if (pr_mem == nullptr)
      {
         if (ep->vclock > em->pre_other()->vclock)
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
         break;
      }

      ///Check if pr_mem < ep

      if (ep->vclock > pr_mem->vclock)
      {
         DEBUG("   pr_mem is predecessor of ep");
         return;
      }

      Event * newevt = u.event(e->action, ep, pr_mem);
      DEBUG("New event created:");
      DEBUG ("  e %-16p pid %2d pre-proc %-16p pre-other %-16p fst/lst %d/%d action %s",
               newevt, newevt->pid(), newevt->pre_proc(), newevt->pre_other(),
               newevt->flags.boxfirst ? 1 : 0,
               newevt->flags.boxlast ? 1 : 0,
               action_type_str (newevt->action.type));

      /// move the pointer to the next
      em = pr_mem;
   }

   DEBUG("   Finish LOCK_cex");
}

void compute_cex(Unfolding & u, BaseConfig & c)
{
   DEBUG("==========Compute cex====");
   Event *e;
   int size = u.num_procs();
   for (unsigned i = 0; i < size; i++)
   {
      e = c.max[i];
      if (!e) continue;

      while (e->action.type != ActionType::THSTART)
      {
         if (e->action.type == ActionType::MTXLOCK)
         {
//            DEBUG("e: %p, type: %s",e, action_type_str(e->action.type));
            LOCK_cex(u,e);
         }


         e = e->pre_proc();
      }
   }
}

} // end of namespace
