
#include <stdexcept>
#include <stdlib.h>

#include "pes/unfolding.hh"
#include "pes/process.hh"
#include "pes/event.hh"

#include "verbosity.h"

namespace dpu
{

Unfolding::Unfolding () :
   nrp (0),
   color (1)
{
   static_assert (MAX_PROC >= 2, "At lest 2 processes");
   static_assert (PROC_SIZE >= sizeof (Process), "Proccess size too small");
   static_assert (PROC_SIZE >= 4 << 10, "Process size too small"); // 4k
   static_assert (ALIGN >= 16, "Process size too small");

   DEBUG ("Unfolding.ctor: this %p max-proc %zu proc-size %zu align %zu%s size %zu%s",
         this, MAX_PROC, PROC_SIZE,
         UNITS_SIZE (ALIGN),
         UNITS_UNIT (ALIGN),
         UNITS_SIZE (MAX_PROC * PROC_SIZE),
         UNITS_UNIT (MAX_PROC * PROC_SIZE));

   // allocate suitably aligned memory for the processes
   DEBUG ("Unfolding.ctor: allocating memory and constructing bottom ...");
   procs = (char *) aligned_alloc (ALIGN, MAX_PROC * PROC_SIZE);
   if (procs == 0)
      throw std::bad_alloc ();

   // create the first process and the bottom event in it; creat is NULL
   new_proc (0);

   DEBUG ("Unfolding.ctor: done, procs %p", procs);
}

Event *Unfolding::find0 (Event *c)
{
   // if the create event (c) is null, we return bottom, already present in the
   // unfolding
   if (c == nullptr)
   {
      ASSERT (proc(0)->first_event());
      ASSERT (proc(0)->first_event()->action.type == ActionType::THSTART);
      return proc(0)->first_event();
   }

   // otherwise we are searching for the root event in a process that we might
   // or might not have already created; if creat has a causal successor in the
   // node[1] tree, then it can be the only one and it must be our event
   if (c->node[1].post.size())
   {
      ASSERT (c->node[1].post.size() == 1);
      ASSERT (c->node[1].post[0]->action.type == ActionType::THSTART);
      ASSERT (c->pid() < c->node[1].post[0]->pid());
      return c->node[1].post[0];
   }

   // otherwise the requested event is not yet in the unfolding
   return nullptr;
}

/// THSTART(), creat is the corresponding THCREAT; this will
/// create a new process
Event *Unfolding::event (Event *creat)
{
   Event *e;

   // if the event already exist, we return it
   e = find0 (creat);
   if (e) return e;

   // otherwise we need to create a new THSTART event in the process whose pid
   // is creat->action.val. If such process does not exist yet in the unfolding,
   // we need to create first the process (and return its root); otherwise we
   // need to insert a new event in that process

   ASSERT (creat); // cannot be a THSTART for pid=0
   ASSERT (creat->action.val <= nrp); // an existing process or the next fresh pid
   if (creat->action.val == nrp)
   {
      // new process, insert the 1st THSTART in the process
      Process *p = new_proc (creat);
      ASSERT (p);
      ASSERT (p->pid() == creat->action.val);
      Event *e = p->first_event();
      ASSERT (e->flags.boxfirst);
      return e;
   }

   // existing process
   ASSERT (creat->pid() != creat->action.val);
   return proc(creat->action.val)->add_event_0p (creat);
}

/// THCREAT(tid) or THEXIT(), one predecessor (in the process)
Event *Unfolding::event (Action ac, Event *p)
{
   Event *e;

   ASSERT (p);
   ASSERT (p->action.type != ActionType::THEXIT);
   ASSERT (ac.type == ActionType::THCREAT or ac.type == ActionType::THEXIT);

   // if the event already exist, we return it
   e = find1 (&ac, p);
   if (e) return e;

   // otherwise we create it
   return p->process()->add_event_1p (ac, p);
}

/// THJOIN(tid), MTXLOCK(addr), MTXUNLK(addr), two predecessors (process, memory/exit)
Event *Unfolding::event (Action ac, Event *p, Event *m)
{
   Event *e;

   // checking that this method was correctly invoked
   ASSERT (p);
   ASSERT (p->action.type != ActionType::THEXIT);
   if (ac.type == ActionType::THJOIN) ASSERT (m and m->action.type == ActionType::THEXIT);
   if (ac.type == ActionType::MTXLOCK)
   {
      ASSERT (ac.addr > nrp);
      ASSERT (!m or m->action.type == ActionType::MTXUNLK);
      ASSERT (!m or m->action.addr == ac.addr);
   }
   if (ac.type == ActionType::MTXUNLK)
   {
      ASSERT (ac.addr > nrp);
      ASSERT (m);
      ASSERT (m->action.type == ActionType::MTXLOCK);
      ASSERT (m->action.addr == ac.addr);
   }

   // if the event already exist, we return it
   e = find2 (&ac, p, m);
   if (e) return e;

   // otherwise we create it
   e = p->process()->add_event_2p (ac, p, m);

   // hack to be able to list in Event::icfl() the root events of an address
   // tree
   if (ac.type == ActionType::MTXLOCK and !m)
   {
      ASSERT (! e->node[1].skiptab);
      ASSERT (! e->node[1].depth)
      ASSERT (! e->node[1].pre)
      auto it = lockroots.find (ac.addr);
      if (it == lockroots.end())
      {
         e->node[1].skiptab = (Event **) e; // init the circular list
         lockroots[ac.addr] = e;
      }
      else
      {
         e->node[1].skiptab = it->second->node[1].skiptab; // add e to circular list
         it->second->node[1].skiptab = (Event **) e;
      }
   }

   return e;
}

Process *Unfolding::new_proc (Event *creat)
{
   void *p;
   if (nrp >= MAX_PROC)
      throw std::range_error ("Maximum number of processes reached, cannot create more");

   // construct the process object, increment nrp
   p = (void*) (procs + nrp * PROC_SIZE);
   nrp++;
   return new (p) Process (creat, this);
}

void Unfolding::dump () const
{
   PRINT ("== unfolding begin ==");
   PRINT (" this %p nrp %u", this, nrp);
   PRINT (" memory %p size %zu%s max-procs %zu",
         procs,
         UNITS_SIZE (MAX_PROC * PROC_SIZE),
         UNITS_UNIT (MAX_PROC * PROC_SIZE),
         MAX_PROC);
   PRINT (" proc-size %zu proc-align %zu%s",
         PROC_SIZE,
         UNITS_SIZE (ALIGN),
         UNITS_UNIT (ALIGN));

   for (unsigned i = 0; i < num_procs(); i++)
   {
      Process *p = proc (i);
      p->dump ();
   }
   PRINT ("== unfolding end ==");
}

void Unfolding::print_dot (std::ofstream &fs, unsigned col, std::string &&msg)
{
   std::string bcolor;
   //std::unordered_map<Event*,unsigned> id;
   unsigned i, j, count, m;
   Process *p;
   std::string var;
   std::vector<Event*> icfls;

   fs << "digraph {\n";
   fs << " node [shape=\"rectangle\", fontsize=10, style=\"filled\"];\n";
   fs << " // ranksep=0.9;\n";

   // we dump the nodes of every process first
   count = 0;
   for (i = 0; i < num_procs(); i++)
   {
      p = proc (i);
      fs << "\n subgraph cluster_p" << p->pid() << " {\n";
      fs << "  label = \"Thread " << p->pid() << "\";\n";
      fs << "  color = \"#f0f0f0\";\n";
      fs << "  fillcolor = \"#f0f0f0\";\n";
      fs << "  style = filled;\n\n";

      m = 0;
      for (Event &e : *p)
      {
         // get a color
         switch (e.action.type)
         {
            case ActionType::MTXLOCK:
               bcolor = "salmon";
               var = fmt ("%p", e.action.addr);
               break;
            case ActionType::MTXUNLK:
               bcolor = "lightblue";
               var = fmt ("%p", e.action.addr);
               break;
            case ActionType::THSTART:
               bcolor = "greenyellow";
               var = "";
               break;
            case ActionType::THCREAT:
               bcolor = "yellow2";
               var = fmt ("%u", e.action.val);
               break;
            case ActionType::THJOIN:
               bcolor = "orchid";
               var = fmt ("%u", e.action.val);
               break;
            case ActionType::THEXIT:
            default:
               var = "";
               bcolor = "grey";
               break;
         }

         // skip event if we were given a mark and it is not marked
         if (col and e.color != col) bcolor = "white";

         // print the node
         fs << "  { rank=same;\n    p" << p->pid() << "_d" << e.depth
            << " [style=\"invis\"];\n";
         fs << "    _" << &e << " [label=\"";
         fs << "" << action_type_str(e.action.type) << " " << var;
         fs << "\\ne " << fmt ("%08x", e.uid());
         fs << " d " << e.depth << "," << e.depth_proc() << "," << e.depth_other();
         //fs << " rb " << e.redbox.size();
         fs << "\" fillcolor=" << bcolor << "]; }\n";

         // compute the maximum depth
         if (m < e.depth) m = e.depth;
         count++;
      }

      // invisible linear graph for the depths
      fs << "\n  /* invisible linear graph fixing the depths */\n";
      for (j = 0; j < m; j++)
      {
         fs << "  p" << p->pid() << "_d" << j << " [style=\"invis\"];\n";
         fs << "  p" << p->pid() << "_d" << j
            << " -> p" << p->pid() << "_d" << j+1 << " [style=\"invis\"];\n";
      }

      fs << " }\n";
   }

   // invisible structure on the top of the threads to sync the beginning (depth
   // 0 in each cluster)
   fs << "\n /* sync the top rank of each cluster */ \n";
   fs << " _root [style=\"invis\"];\n";
   for (i = 0; i < num_procs(); i++)
   {
      fs << " _root -> p" << i << "_d0 [style=\"invis\"];\n";
   }

   fs << "\n /* causality edges */ \n";
   for (i = 0; i < num_procs(); i++)
   {
      p = proc (i);
      for (Event &e : *p)
      {
         if (e.pre_proc() != nullptr)
            fs << " _" << e.pre_proc() << " -> _" << &e << " [weight=1000];\n";
            // [weight=1000] could be useful here

         if (e.pre_other() != nullptr)
            fs << " _" << e.pre_other() << " -> _" << &e << " [color=blue, constraint=true];\n";
      }
   }

   fs << "\n /* conflict edges */ \n";
   for (i = 0; i < num_procs(); i++)
   {
      p = proc (i);
      for (Event &e : *p)
      {
         e.icfls (icfls);
         for (Event *ee : icfls)
         {
            // avoid redrawing the same conflict lines
            if (ee < &e) continue;
            fs << " _" << &e << " -> _" << ee
               << " [constraint=true, weight=0.1, dir=none, color=red, style=dashed];\n";
         }
         icfls.clear();
      }
   }

   fs << "\n label = \"" << msg;
   fs << "\\n\\nUnfolding " << this << "\\n";
   fs << count << " events, " << num_procs() << " threads";
   fs << "\";\n";
   fs << "}\n";
}

void Unfolding::print_dot (Cut &c, std::ofstream &fs, std::string &&msg)
{
   unsigned col;

   col = get_fresh_color();
   c.colorize (col);
   print_dot (fs, col, std::move(msg));
}

void Unfolding::print_dot (const std::string &path, unsigned col,
   std::string &&msg)
{
   std::ofstream f (path);
   print_dot (f, col, std::move(msg));
   f.close ();
}

} // namespace dpu
