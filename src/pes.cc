/*
 * pes.cc
 *
 *  Created on: Jan 12, 2016
 *      Author: tnguyen
 */

#include <cstdint>
#include <cstdio>
#include <cmath>
#include <cassert>
#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <sys/stat.h>

#include "pes.hh"
#include "misc.hh"
#include "verbosity.h"

using std::vector;
using std::fstream;
using std::string;
using namespace ir;

namespace pes{

/*
 * Methods for class MultiNode
 */
template<class T> void MultiNode<T> ::print_pred(int idx)
{
      int i = 1;
      for (T *n = (T*) this; n; ++i, n = n->node[idx].skip_preds[0])
      {
         printf ("%d. n %p depth %d\n", i, n, n->node[idx].depth);
      }
}

/*
 *  find predecessor at the depth of d in the tree idx
 *  - 0 for process tree
 *  - 1 for variable tree
 */
template<class T> void MultiNode<T> ::find_pred(int idx, int d, int step)
{
   T * next = this->node[idx];
   int i;
   while (next->depth > d)
   {
      i = log(next->depth - d)/log(step);
      while (next->depth % pow(step,i) != 0)
         i--;
      next = next->skip_preds[i];
   }

   //return next;
}

/*
 * Methods for class Event
 */

Event::Event (const Trans & t, Config & c)
   : idx(c.unf.count)
   , pre_proc(nullptr)
   , pre_mem(nullptr)
   , val(0)
   , localvals(0)
   , trans(&t)
   , color(0)

{
   assert(c.unf.count < c.unf.evt.capacity());
   unsigned numprocs = c.unf.m.procs.size();
   if (t.type == ir::Trans::WR)
   {
      std::vector<Event *> temp;
      for (unsigned i = 0; i < numprocs; i++)
         post_mem.push_back(temp);
   }

   // clock.reserve(numprocs);
   for (unsigned i = 0; i < numprocs; i++)
      clock.push_back(0);

   DEBUG ("  %p: Event.ctor: t %p: '%s'", this, &t, t.str().c_str());
   mk_history(c);
   /* set up vector clock */
   set_vclock();
   set_maxevt();
}

Event::Event (Unfolding & u)
   : idx(u.count)
   , pre_proc(nullptr)
   , pre_mem(nullptr)
   , val(0)
   , localvals(0)
   , trans(nullptr)
   , color(0)
{
   // this is for bottom event only
   unsigned numprocs = u.m.procs.size();
   //unsigned mem = u.m.memsize - numprocs;

   std::vector<Event *> temp;
   for (unsigned i = 0; i < numprocs; i++)
      post_mem.push_back(temp);

   // initialize vector clock
   for (unsigned i = 0; i < numprocs; i++)
      clock.push_back(0);

   DEBUG ("  %p: Event.ctor:", this);
}
/*
 * for all events. Don't use any more
 */
Event::Event (const Trans & t, Unfolding & u)
   : idx(u.count)
   , pre_proc(nullptr)
   , pre_mem(nullptr)
   , val(0)
   , localvals(0)
   , trans(&t)
   , color(0)

{
   assert(u.count < u.evt.capacity());
   unsigned numprocs = u.m.procs.size();

   // clock.reserve(numprocs);
   for (unsigned i = 0; i < numprocs; i++)
      clock.push_back(0);

   DEBUG ("  %p: Event.ctor: t %p: '%s'", this, &t, t.str().c_str());
}

// for create RD, SYN event when compute cex
Event::Event (const ir::Trans & t, Event * ep, Event * em, Unfolding & u)
   : idx(u.count)
   , pre_proc(ep)
   , pre_mem(em)
   , val(0)
   , localvals(0)
   , trans(&t)
   , color(0)

{
   assert(u.count < u.evt.capacity());
   unsigned numprocs = u.m.procs.size();

   for (unsigned i = 0; i < numprocs; i++)
      clock.push_back(0);

   DEBUG ("  %p: Event.ctor: t %p: '%s'", this, &t, t.str().c_str());
}

/*
 *  for WR event:
 *  - t: transition
 *  - ep: pre_proc
 *  - pr: pre_readers
 *  - u: unfolding to add
 */

Event::Event (const ir::Trans & t, Event * ep, std::vector<Event *> pr, Unfolding & u)
   : idx(u.count)
   , pre_proc(ep)
   , pre_mem(nullptr)
   , pre_readers(pr)
   , val(0)
   , localvals(0)
   , trans(&t)
   , color(0)

{
   assert(u.count < u.evt.capacity());
   unsigned numprocs = u.m.procs.size();

   assert(t.type == Trans::WR);

   std::vector<Event *> temp;
   for (unsigned i = 0; i < numprocs; i++)
      post_mem.push_back(temp);

   for (unsigned i = 0; i < numprocs; i++)
      clock.push_back(0);
   /* set up vector clock */
   set_vclock();
   DEBUG ("  %p: Event.ctor: t %p: '", this, &t);
}

Event::Event (const Event & e)
   : idx(e.idx)
   , pre_proc(e.pre_proc)
   , post_proc(e.post_proc)
   , pre_mem(e.pre_mem)
   , post_mem(e.post_mem)
   , pre_readers(e.pre_readers)
   , post_wr(e.post_wr)
   , post_rws(e.post_rws)
   , val(e.val)
   , localvals(e.localvals)
   , trans(e.trans)
   , color(e.color)
   , clock(e.clock)
{
}

bool Event::is_bottom () const
{
   return this->pre_mem == this;
}

/*
 * set up its history, including 3 attributes: pre_proc, pre_mem and pre_readers
 */
void Event::mk_history(const Config & c)
{
   /*
    * For all events:
    * - pre_proc    is the latest event of the same process in c
    * - post_proc, post_mem, and post_rws
    *               remain empty
    *
    * For RD
    * - pre_mem     is the lastest operation on the same process inside of c
    * - pre_readers remains empty
    *
    * For WR
    * - pre_mem     is the lastest WR in c
    * - pre_readers is the lastest OP of ALL processes in c
    *
    * For LOC
    * - pre_mem     is NULL
    * - pre_readers remains empty
    */

   if (this->is_bottom()) return;

   ir::Process & p = this->trans->proc;
   std::vector<Process> & procs = c.unf.m.procs;
   int varaddr = trans->var - procs.size();

   /*
    * e's parent is the latest event of the process
    * for all events, initialize pre_proc of new event by the latest event of config (in its process)
    */
   pre_proc = c.latest_proc[p.id];

   switch (trans->type)
   {
      case ir::Trans::RD:
         pre_mem  = c.latest_op[p.id][varaddr];
         /* pre_readers stays empty for RD events */
         break;

      case ir::Trans::WR:
         pre_mem  = c.latest_wr[varaddr];
        /*
         * set pre-readers = set of latest events which use the variable copies of all processes
         * size of pre-readers is numbers of copies of the variable = number of processes
         */

         for (unsigned int i = 0; i < procs.size(); i++)
            pre_readers.push_back(c.latest_op[i][varaddr]);

         break;

      case ir::Trans::SYN:
         pre_mem  = c.latest_op[p.id][varaddr]; // pre_mem can be latest SYN from another process (same variable)
         break;

      case ir::Trans::LOC:
         // nothing to do
    	 /*
    	  * If a LOC transition RD or WR on a local variable,
    	  * its pre_mem is previous event touching that one.
    	  * If LOC is a EXIT, no variable touched, no pre_mem
    	  * How to solve: v4 = v3 + 1; a RD for v3 but a WR for v4 (local variable)
    	  */
         pre_mem   = nullptr;
         break;
   }

   DEBUG("   Make history: %s ",this->str().c_str());
}
void Event::set_vclock()
{
   Process & p = this->trans->proc;
   clock = pre_proc->clock;
   switch (this->trans->type)
     {
        case ir::Trans::LOC:
           clock[p.id]++;
           break;

        case ir::Trans::SYN:
           for (unsigned i = 0; i < clock.size(); i++)
              clock[i] = std::max(pre_mem->clock[i], clock[i]);

           clock[p.id]++;
           break;

        case ir::Trans::RD:
           for (unsigned i = 0; i < clock.size(); i++)
              clock[i] = std::max(pre_mem->clock[i], clock[i]);

           clock[p.id]++;
           break;

        case ir::Trans::WR:
           // find out the max elements among those of all pre_readers.
           for (unsigned i = 0; i < clock.size(); i++)
           {
              /* put all elements j of pre_readers to a vector temp */
              for (unsigned j = 0; j < pre_readers.size(); j++)
                clock[i] = std::max(pre_readers[j]->clock[i], clock[i]);
           }

           clock[p.id]++;
           break;
     }

}
/*
 * Set up the vector maxevt which stores maximal events in an event's local configuration
 */
void Event::set_maxevt()
{
   maxevt = pre_proc->maxevt; // initialize maxevt by maxevt(pre_proc)
   switch (this->trans->type)
   {
        case ir::Trans::LOC:
           //maxevt = pre_proc->maxevt;
           break;

        case ir::Trans::SYN:
           for (unsigned i = 0; i < maxevt.size(); i++)
              maxevt[i] = std::max(pre_mem->maxevt[i], maxevt[i]);
           break;

        case ir::Trans::RD:
           for (unsigned i = 0; i < maxevt.size(); i++)
              maxevt[i] = std::max(pre_mem->maxevt[i], maxevt[i]);
           break;

        case ir::Trans::WR:
           // find out the max elements among those of all pre_readers.
           for (unsigned i = 0; i < maxevt.size(); i++)
           {
              for (unsigned j = 0; j < pre_readers.size(); j++)
                 if ( pre_readers[j]->maxevt[i] > maxevt[i])
                     maxevt[i] = pre_readers[j]->maxevt[i] ;
           }
           break;
   }
}
/*
 * Update all events precede current event, including pre_proc, pre_mem and all pre_readers
 */
void Event::update_parents()
{
   DEBUG("   Update_parents:  ");
   if (is_bottom())
      return;

   Process & p  = this->trans->proc;

   switch (this->trans->type)
   {
      case ir::Trans::WR:
         pre_proc->post_proc.push_back(this);
         /* update pre_mem */
         //pre_mem->post_wr.push_back(this); // need to consider its necessary
         for (unsigned i = 0; i < pre_readers.size(); i++)
         {
            if (pre_readers[i]->is_bottom() || (pre_readers[i]->trans->type == ir::Trans::WR))
            {
               // add to vector of corresponding process in post_mem
               pre_readers[i]->post_mem[i].push_back(this);
            }
            else
            {
               pre_readers[i]->post_rws.push_back(this);
            }
         }

         break;

      case ir::Trans::RD:
         pre_proc->post_proc.push_back(this);
         /* update pre_mem */
         if ( (pre_mem->is_bottom() == true) || (pre_mem->trans->type == ir::Trans::WR)   )
            pre_mem->post_mem[p.id].push_back(this);
         else
            pre_mem->post_rws.push_back(this);
         /* no pre_readers -> nothing to do with pre_readers */

         break;

      case ir::Trans::SYN:
         pre_proc->post_proc.push_back(this);
         /* update pre_mem */
         if ( (pre_mem->is_bottom() == true) || (pre_mem->trans->type == ir::Trans::WR)   )
            pre_mem->post_mem[p.id].push_back(this);
         else
            pre_mem->post_rws.push_back(this);
         /* no pre_readers -> nothing to do with pre_readers */
         break;

      case ir::Trans::LOC:
         pre_proc->post_proc.push_back(this);
         /* update pre_mem */
         /* no pre_readers -> nothing to do with pre_readers */
         break;
   }

   return ;
}
/*
 * Check if e is in this event's history or not
 * If this and e are in the same process, check their source
 * If not, check pre_mem chain of this event to see whether e is inside or not (e can only be a RD, SYN or WR)
 */
bool Event::in_history(Event * e )
{
#if 0
   for (unsigned int i = 0; i < clock.size(); i++)
   if (e->clock[i] > clock[i])
      return false;
   return true;
#endif

   if (e->clock < clock)
        return true;
     return false;
}

/*
 * Overlap == operator
 */
bool Event:: operator == (const Event & e) const
{
   return this == &e;
}

/*
 * Overlap = operator
 */
Event & Event:: operator  = (const Event & e)
{
   pre_proc = e.pre_proc;
   pre_mem = e.pre_mem;
   val = e.val;
   trans = e.trans;

   for (unsigned i = 0; i < e.pre_readers.size(); i++)
         pre_readers.push_back(e.pre_readers[i]);

   for (unsigned i = 0; i < e.post_mem.size(); i++)
   {
      for (unsigned j = 0; j < e.post_mem[i].size(); j++)
         post_mem[i].push_back(e.post_mem[i][j]);

      post_mem.push_back(post_mem[i]);
   }

   for (unsigned i = 0; i < e.post_proc.size(); i++)
      post_proc.push_back(e.post_proc[i]);

   for (unsigned i = 0; i < e.post_wr.size(); i++)
         post_wr.push_back(e.post_wr[i]);

   for (unsigned i = 0; i < e.post_rws.size(); i++)
         post_rws.push_back(e.post_rws[i]);

  return *this;
}
/*
 * Two events are in conflict if they both appear in a vector in post_mem of an event
 */

bool Event::check_cfl( const Event & e ) const
{
   if (this->is_bottom() || e.is_bottom() || (*this == e) )
      return false;

   /*
    * Check pre_proc: if they have the same pre_proc and in the same process -> conflict
    * It is true for the case where pre_proc is bottom
    */

   if ((this->pre_proc == e.pre_proc) && (this->trans->proc.id == e.trans->proc.id))
      return true;

   /*
    *  a LOC event has no conflict with any other transition.
    * "2 LOC trans sharing a localvar" doesn't matter because it depends on the PC of process.
    * Here, it means they don't have same pre_proc --> any LOC is in no conflict with others.
    */
   if ((this->trans->type == ir::Trans::LOC)  || (e.trans->type == ir::Trans::LOC))
      return false;

   // different pre_procs => check pre_mem or pre_readers
   Event * parent = pre_mem; // for RD, SYN, WR events
   std::vector<Event *>::iterator this_idx, e_idx;

   // special case when parent is bottom, bottom as a WR, but not exactly a WR
   if (parent->is_bottom())
   {
	   for (unsigned i = 0; i< parent->post_mem.size(); i++)
	   {
		  this_idx = std::find(parent->post_mem[i].begin(), parent->post_mem[i].end(),this);
		  e_idx    = std::find(parent->post_mem[i].begin(), parent->post_mem[i].end(),&e);
        if ( (this_idx != parent->post_mem[i].end()) && (e_idx != parent->post_mem[i].end()) )
           return true;
  	   }
	   return false;
   }

   switch (parent->trans->type)
   {
      case ir::Trans::RD:
         this_idx    = std::find(parent->post_rws.begin(), parent->post_rws.end(),this);
         e_idx  = std::find(parent->post_rws.begin(), parent->post_rws.end(),&e);
    	   if ( (this_idx != parent->post_rws.end()) && (e_idx != parent->post_rws.end()) )
    	      return true;
         break;

      case ir::Trans::WR:
         for (unsigned i = 0; i< parent->post_mem.size(); i++)
         {
            this_idx = std::find(parent->post_mem[i].begin(), parent->post_mem[i].end(),this);
            e_idx    = std::find(parent->post_mem[i].begin(), parent->post_mem[i].end(),&e);
            if ( (this_idx != parent->post_mem[i].end()) && (e_idx != parent->post_mem[i].end()) )
               return true;
         }
         break;

     case ir::Trans::SYN:
        this_idx = std::find(parent->post_rws.begin(), parent->post_rws.end(),this);
    	  e_idx    = std::find(parent->post_rws.begin(), parent->post_rws.end(),&e);
    	  if ( (this_idx != parent->post_rws.end()) && (e_idx != parent->post_rws.end()) )
           return true;
    	  break;

     case ir::Trans::LOC:
        // nothing to do
        break;
   }

   return false;
}

/* check if 2 events are the same or not */
bool Event:: is_same(Event & e)
{
   if (this->is_bottom() or e.is_bottom()) return false;

   if ( (trans == e.trans) && (pre_proc == e.pre_proc)
         && (pre_mem == e.pre_mem) && (pre_readers == e.pre_readers))
      return true;

   return false;
}

/* Express an event in a string */
std::string Event::str () const
{
  std::string st;

   if (pre_readers.empty())
      st = "None";
   else
   {
      for (unsigned int i = 0; i < pre_readers.size(); i++)
         if (i == pre_readers.size() -1)
            st += std::to_string(pre_readers[i]->idx);
         else
            st += std::to_string(pre_readers[i]->idx) + ", ";
   }

   const char * code = trans ? trans->code.str().c_str() : "";
   int proc = trans ? trans->proc.id : -1;

   if (pre_mem != nullptr)
      return fmt ("index: %d, %p: trans %p code: '%s' proc: %d pre_proc: %p pre_mem: %p pre_readers: %s ",
         idx, this, trans, code, proc, pre_proc, pre_mem, st.c_str());
   else
	  return fmt ("index: %d, %p: trans %p code: '%s' proc: %d pre_proc: %p pre_mem(null): %p pre_readers: %s",
	            idx, this, trans, code, proc, pre_proc, pre_mem, st.c_str());

}
/* represent event's information for dot print */
std::string Event::dotstr () const
{
   std::string st;
   for (unsigned int i = 0; i < clock.size(); i++)
      if (i == clock.size() -1)
         st += std::to_string(clock[i]);
      else
         st += std::to_string(clock[i]) + ", ";

   const char * code = (trans != nullptr) ? trans->code.str().c_str() : ""; // pay attention at c_str()
   int proc = (trans != nullptr) ? trans->proc.id : -1;
   int src = trans? trans->src : 0;
   int dst = trans? trans->dst : 0;

   return fmt ("id: %d, code: '%s' \n proc: %d , src: %d , dest: %d \n clock:(%s) ", idx, code, proc, src, dst, st.c_str());
}

/* Print all information of an event */
void Event::eprint_debug() const
{
	printf ("Event: %s", this->str().c_str());
	if (pre_readers.size() != 0)
	{
		DEBUG("\n Pre_readers: ");
		for (unsigned int i = 0; i < pre_readers.size(); i++)
			DEBUG("  Process %d: %d",i, pre_readers[i]->idx);
	}
	else
	   DEBUG("\n No pre_readers");
	//print post_mem
	if (post_mem.size() != 0)
		{
			DEBUG(" Post_mem: ");
			for (unsigned int i = 0; i < post_mem.size(); i++)
			{
			   printf("  Process %d:", i);
			   for (unsigned j = 0; j < post_mem[i].size(); j++)
			      printf(" %d  ",post_mem[i][j]->idx);
			   printf("\n");
			}
		}
	else
		   DEBUG(" No post_mem");

	// print post_proc
   if (post_proc.size() != 0)
   {
      printf(" Post_proc: ");
	   for (unsigned int i = 0; i < post_proc.size(); i++)
	      printf("%d   ", post_proc[i]->idx);
	   printf("\n");
   }
   else
      DEBUG(" No post proc");

   // print post_rws
   if (post_rws.size() != 0)
      {
          DEBUG(" Post_rws: ");
         for (unsigned int i = 0; i < post_rws.size(); i++)
            DEBUG("  Process %d: %d",i, post_rws[i]->idx);
      }
      else
         DEBUG(" No post rws");
}

/*
 *========= Methods of Config class ===========
 */

Config::Config (Unfolding & u)
   : gstate (u.m.init_state)
   , unf (u)
   , latest_proc (u.m.procs.size (), u.bottom)
   , latest_wr (u.m.memsize - u.m.procs.size(), u.bottom)
   , latest_op (u.m.procs.size (), std::vector<Event*> (u.m.memsize - u.m.procs.size(), u.bottom))
{
   /* with the unf containing only bottom event */
   DEBUG ("%p: Config.ctor", this);

   // compute enable set for a configuration with the only event "bottom"
   __update_encex (*unf.bottom);
}

#if 0
Config:: Config (const Config & c)
   : latest_proc (c.latest_proc)
   , latest_wr (c.latest_wr)
   , latest_op (c.latest_op)
   , latest_local_wr (c.latest_local_wr)
   , unf(c.unf)
{
   gstate = c.gstate;
}
#endif

/*
 * Add an event fixed by developer to a configuration
 */

void Config::add_any ()
{
   // the last event in enable set
   add (en.size () - 1);
}

/*
 * add an event identiied by its value
 */

void Config::add (const Event & e)
{

   DEBUG ("\n%p: Config.add: %p\n", this, e.str().c_str());
   for (unsigned int i = 0; i < en.size (); i++)
      if (e == *en[i]) add (i);
   throw std::range_error ("Trying to add an event not in enable set by a configuration");

}

/*
 * add an event identiied by its index idx
 */
void Config::add (unsigned idx)
{
   assert(idx < en.size());
   Event & e = *en[idx];

   DEBUG ("\n%p: Config.add: %s\n", this, e.str().c_str());

   /* move the element en[idx] out of enable set */
   en[idx] = en.back();
   en.pop_back();

   ir::Process & p              = e.trans->proc; // process of the transition of the event
   std::vector<Process> & procs = unf.m.procs; // all processes in the machine
   int varaddr = e.trans->var - procs.size();

   // update the configuration
   e.trans->fire (gstate); //move to next state

   latest_proc[p.id] = &e; //update latest event of the process containing e.trans

   //update other attributes according to the type of transition.
   switch (e.trans->type)
   {
   case ir::Trans::RD:
      latest_op[p.id][varaddr] = &e; // update only latest_op
      break;

   case ir::Trans::WR:
      latest_wr[varaddr] = &e; // update latest wr event for the variable var
      for (unsigned int i = 0; i < procs.size(); i++)
         latest_op[i][varaddr] = &e;
      break;

   case ir::Trans::SYN:
      latest_wr[varaddr]=&e;
      break;

   case ir::Trans::LOC:
      //nothing to do with LOC
      break;
   }

   //update local variables in trans
      if (e.trans->localvars.empty() != true)
         for (auto & i: e.trans->localvars)
         {
            latest_op[p.id][i - procs.size()] = &e;
         }

   /* remove conflicting events with event which has just been added */
   if (en.size() > 0)
      remove_cfl(e);

     /* update en and cex set with e being added to c (before removing it from en) */
   __update_encex(e);
}

/*
 * Update enabled set whenever an event e is added to c
 */
void Config::__update_encex (Event & e )
{
  //Event * pe;
   DEBUG ("%p: Config.__update_encex(%p)", this, &e);

   assert(unf.m.trans.size() > 0);
   assert(unf.m.procs.size() > 0);

   std::vector<Trans*> enable;

   /* get set of events enabled at the state gstate    */
   gstate.enabled (enable);

   if (enable.empty() == true )
      return;

   DEBUG(" Transitions enabled:");
   for (auto t: enable)
      DEBUG("  %s", t->str().c_str());

   DEBUG (" New events:");

   for (auto t : enable)
   {
      /*
       *  create new event with transition t and add it to evt of the unf
       *  have to check evt capacity before adding to prevent the reallocation.
       */
      unf.create_event(*t, *this);
   }
}

/*
 *  Remove all event in enable set that are in conflict with added event to the configuration
 */
void Config::remove_cfl(Event & e)
{
   printf (" %p: Config.remove_cfl(%p): ", this, &e);
   unsigned int i = 0;

   while (i < en.size())
   {
      if (e.check_cfl(*en[i]) == true)
      {
         printf ("  %p ", en[i]);
         e.dicfl.push_back(en[i]); // add en[i] to direct conflicting set of e
         en[i]->dicfl.push_back(&e); // add e to direct conflict set of en[i]
         cex.push_back(en[i]);
         en[i] = en.back();
         en.pop_back();
      }
      else   i++;
   }
   printf("\n");
}
/*
 * compute confilct extension for a maximal configuration
 */
void Config::compute_cex ()
{
   DEBUG("%p: Config.compute_cex: ",this);
   for (auto e : latest_proc)
   {
      Event * p = e;
      while (p->is_bottom() != true)
      {
         switch (p->trans->type)
         {
            case ir::Trans::RD:
               this->RD_cex(p);
               break;
            case ir::Trans::SYN:
               this->SYN_cex(p);
               break;
            case ir::Trans::WR:
               this->WR_cex(p);
               break;
            case ir::Trans::LOC:
               DEBUG(" %p, id:%d: No conflict for a LOC", p, p->idx);
               break;
         }
         p = p->pre_proc;
      }
   }

   this->__print_cex();
}
/*
 *  compute conflict extension for a RD event
 */
void Config:: RD_cex(Event * e)
{
   DEBUG(" %p, id:%d: RD conflicting extension", e, e->idx);
   Event * ep, * em, *pr_mem; //pr_mem: pre_mem
   const Trans & t = *(e->trans);
   ep = e->pre_proc;
   em = e->pre_mem;

   while (!(em->is_bottom()) and (ep->in_history(em) == false))
   {
      if (em->trans->type == ir::Trans::RD)
      {
         pr_mem   = em;
         em       = em->pre_mem;
      }
      else
      {
         pr_mem   = em->pre_readers[e->trans->proc.id];
         em       = em->pre_readers[e->trans->proc.id];
      }

      /* Need to check the event's history before adding it to the unf
       * Don't add events which are already in the unf
       */

      Event * newevt = &unf.find_or_add(t,ep,pr_mem);
      add_to_cex(newevt);

      // add event to set of direct conflict dicfl if it is new one.
      if (newevt->idx == unf.evt.back().idx)
      {
         e->dicfl.push_back(newevt);
         newevt->dicfl.push_back(e);
      }

#if 0
      bool in_unf = false, in_cex = false;
      for (auto & ee :c.unf.evt)
      {
         if ( temp->is_same(ee) == true )
         {
            in_unf = true;
            DEBUG("   Already in the unf as %s", ee.str().c_str());
            // if it is in cex -> don't add
            for (auto ce : c.cex1)
               if (ee.idx == ce->idx)
               {
                  in_cex = true;
                  DEBUG("Event already in the cex");
                  break;
               }

            if (in_cex == false)
            {
               c.cex1.push_back(&ee);
               this->dicfl.push_back(&ee);
            }

            break;
         }
      } // end for

      if (in_unf == false)
      {
            DEBUG(" Temp doesn't exist in evt");
            c.unf.evt.push_back(*temp);
            c.unf.evt.back().update_parents();
            c.unf.count++;
            c.cex1.push_back(&c.unf.evt.back());
            this->dicfl.push_back(&c.unf.evt.back());
            DEBUG("   Unf.evt.back: id: %s \n ", c.unf.evt.back().str().c_str());
      }
#endif
   } // end while
}

/*
 *  compute conflict events for a SYN event
 */
void Config:: SYN_cex(Event * e)
{
   DEBUG(" %p, id:%d: RD conflicting extension", e, e->idx);
   Event * ep, * em, *pr_mem; //pr_mem: pre_mem
   const Trans & t = *(e->trans);
   ep = e->pre_proc;
   em = e->pre_mem;

   while (!(em->is_bottom()) and (ep->in_history(em) == false))
   {
      if (em->trans->type == ir::Trans::RD)
      {
         pr_mem   = em;
         em       = em->pre_mem;
      }
      else
      {
         pr_mem   = em->pre_readers[e->trans->proc.id];
         em       = em->pre_readers[e->trans->proc.id];
      }

      /*
       *  Need to check the event's history before adding it to the unf
       * Don't add events which are already in the unf
       */

      Event * newevt = &unf.find_or_add(t,ep,pr_mem);
      add_to_cex(newevt);

      // add event to set of direct conflict dicfl if it is new one.
      if (newevt->idx == unf.evt.back().idx)
      {
         e->dicfl.push_back(newevt);
         newevt->dicfl.push_back(e);
      }
   }
}
/*
 * Compute conflicting event for a WR event
 */
void Config:: WR_cex(Event * e)
{
   DEBUG(" %p: id:%d WR cex computing", e, e->idx);
   Event * ep, * ew, * temp;
   unsigned numprocs = unf.m.procs.size();
   std::vector<std::vector<Event *> > spikes(numprocs);
   spikes.resize(numprocs);

   ep = e->pre_proc;
   ew = e;
   int count = 0;

   while ((ew->is_bottom() == false) && (ep->in_history(ew)) == false )
   {
      for (unsigned k = 0; k < numprocs; k++)
         spikes[k].clear(); // clear all spikes for storing new elements

      DEBUG("  Comb #%d (%d spikes): ", count++, numprocs);

      for (unsigned i = 0; i < ew->pre_readers.size(); i++)
      {
         temp = ew->pre_readers[i];
         while ((temp->is_bottom() == false) && (temp->trans->type != Trans::WR))
         {
            spikes[i].push_back(temp);
            temp = temp->pre_mem;
         }

         spikes[i].push_back(temp); // add the last WR or bottom to the spike
      }
      /* show the comb */
      for (unsigned i = 0; i < spikes.size(); i++)
      {
         printf("    ");
         for (unsigned j = 0; j < spikes[i].size(); j++)
            printf("%d ", spikes[i][j]->idx);
         printf("\n");
      }

      ew = spikes[0].back();
      /*
       * if ew is a event which is inside ep's history,
       * remove from the comb ew itself and all RD events that are also in the history
       * should think about bottom!!!
       */
      if (ep->in_history(ew) == true)
      {
         for (int unsigned i = 0; i < spikes.size(); i++)
            while (ep->in_history(spikes[i].back()) == true)
               spikes[i].pop_back(); // WR event at the back and its predecessors have an order backward
      }

      /*
       * Compute all possible combinations c(s1,s2,..sn) from the spikes to produce new conflicting events
       */
      std::vector<Event *> combi;
      compute_combi(0, spikes, combi,e);
   }
}
/*
 * compute all combinations of set s
 * c is vector to store a combination
 */
void Config::compute_combi(unsigned int i, const std::vector<std::vector<Event *>> & s, std::vector<Event *> combi, Event * e)
{
   Event * newevt;
   const ir::Trans & t = *(e->trans);
   for (unsigned j = 0; j < s[i].size(); j++ )
   {
      if (j < s[i].size())
      {
         combi.push_back(s[i][j]);
         if (i == s.size() - 1)
         {
            printf("   Combination:");
            for (unsigned k = 0; k < combi.size(); k++)
               printf("%d ", combi[k]->idx);

            /* if an event is already in the unf, it must have all necessary relations including causality and conflict.
             * That means it is in cex, so don't need to check if old event is in cex or not. It's surely in cex.
             */
            newevt = &unf.find_or_addWR(t, e->pre_proc, combi);
            add_to_cex(newevt);

            // update direct conflicting set for both events, but only for new added event.

            if (newevt->idx == unf.evt.back().idx) // new added event at back of evt
            {
               e->dicfl.push_back(newevt);
               newevt->dicfl.push_back(e);
            }

            printf("\n");
         }
         else
            compute_combi(i+1, s, combi, e);
      }
      combi.pop_back();
   }
}


// check if ee is in the cex or not. if not, add it to cex.
void Config:: add_to_cex(Event * ee)
{
   for (auto ce : cex)
      if (ee->idx == ce->idx)
      {
         DEBUG("Event already in the cex");
         return;
      }
   cex.push_back(ee);
   return;
}
/*
 * Print all the latest events of a configuration to console
 */
void Config::cprint_debug () const
{
   DEBUG("%p: Config.cprint_debug:", this);
   DEBUG ("%p: latest_proc:", this);
   for (auto & e : latest_proc)
      DEBUG (" %s", e->str().c_str());

   DEBUG ("%p: latest_wr:", this);
   for (auto & e : latest_wr) DEBUG (" %s", e->str().c_str());
   DEBUG ("%p: latest_op:", this);
   for (unsigned int pid = 0; pid < latest_op.size(); ++pid)
   {
      DEBUG (" Process %zu", pid);
      for (auto & e : latest_op[pid])
    	 DEBUG ("  %s", e->str().c_str());
   }
}

/*
 * cprint_dot with a fixed file
 */
void Config::cprint_dot()
{
   /* Create a folder namely output in case it hasn't existed. Work only in Linux */
   DEBUG("\n%p: Config.cprint_dot:", this);

   const char * mydir = "output";
   struct stat st;
   if ((stat(mydir, &st) == 0) && (((st.st_mode) & S_IFMT) == S_IFDIR))
      DEBUG(" Directory %s already exists", mydir);
   else
   {
      const int dir_err = mkdir("output", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
      if (-1 == dir_err)
         DEBUG("Directory \"output\" has just been created!");
   }

   printf(" The configuration is exported to dot file: \"dpu/output/conf.dot\"");
   std::ofstream fs("output/conf.dot", std::fstream::out);
   if (fs.is_open() != true)
      printf("Cannot open the file\n");

   fs << "Digraph RGraph {\n node [shape = rectangle, style = filled]";
   fs << "label =\"A random configuratiton.\"";
   fs << "edge [style=filled]";
   /*
    * Maximal events: = subset of latest_proc (latest events having no child)
    */
   Event * p;
   for (auto e : latest_proc)
   {
      if ( (e->post_proc.empty() == true) && (e->post_rws.empty() == true) ) // if e is maximal event: no child event
      {
         p = e;
         while ( (p->is_bottom() != true) && (p->color == 0) )
         {
            p->color = 1;
            switch (p->trans->type)
            {
               case ir::Trans::LOC:
                  fs << p->idx << "[id="<< p->idx << ", label=\" " << p->dotstr() << " \", fillcolor=yellow];\n";
                  fs << p->pre_proc->idx << "->"<< p->idx << "[color=brown];\n";
                  break;
               case ir::Trans::RD:
                  fs << p->idx << "[id="<< p->idx << ", label=\" " << p->dotstr() << " \", fillcolor=palegreen];\n";
                  fs << p->pre_mem->idx << "->"<< p->idx << ";\n";
                  fs << p->pre_proc->idx << "->"<< p->idx << "[fillcolor=brown];\n";
                  break;
               case ir::Trans::SYN:
                  fs << p->idx << "[id="<< p->idx << ", label=\" " << p->dotstr() << " \", fillcolor=lightblue];\n";
                  fs << p->pre_mem->idx << "->"<< p->idx << ";\n";
                  fs << p->pre_proc->idx << "->"<< p->idx << "[color=brown];\n";
                  break;

               case ir::Trans::WR:
                  fs << p->idx << "[id="<< p->idx << ", label=\" " << p->dotstr() << " \", fillcolor=red];\n";
                  fs << p->pre_proc->idx << "->"<< p->idx << "[color=brown];\n";
                  for (auto pr : p->pre_readers)
                  {
                     fs << pr->idx << "->"<< p->idx << ";\n";
                  }
                  break;
            }
            p = p->pre_proc;
         }
      }
   }

   fs << "}";
   fs.close();
   printf(" successfully\n");
}

/*
 *  Print the size of current enable set
 */
void Config::__print_en() const
{
	DEBUG ("Enable set of config %p: size: %zu", this, en.size());
	   for (auto & e : en)
		   e->eprint_debug();
}

/*
 *  Print the set of conflicting extension
 */
void Config::__print_cex() const
{
   DEBUG ("\nConflicting set of config %p: size: %zu", this, cex.size());
      for (auto & e : cex)
         e->eprint_debug();
}

/*
 * Methods for class Unfolding
 */
unsigned Unfolding::count = 0;

Unfolding::Unfolding (ir::Machine & ma)
   : m (ma)
   , colorflag(0)
{
   evt.reserve(50); // maximum number of events????
   DEBUG ("%p: Unfolding.ctor: m %p", this, &m);
   __create_bottom ();
}

void Unfolding::__create_bottom ()
{
   assert (evt.size () == 0);
   /* create an "bottom" event with all empty */
   evt.push_back(Event(*this));
   count++;
   bottom = &evt.back(); // using = operator
   bottom->pre_mem = bottom;
   bottom->pre_proc = bottom;

   DEBUG ("%p: Unfolding.__create_bottom: bottom %p", this, &evt.back());
}
/*
 * create an event with enabled transition and config.
 */
void Unfolding::create_event(ir::Trans & t, Config & c)
{
   /* Need to check the event's history before adding it to the unf
    * Only add events that are really enabled at the current state of config.
    * Don't add events already in the unf (enalbed at the previous state)
    */

   evt.emplace_back(t, c);

   for (auto ee :c.en)
      if (evt.back().is_same(*ee))
      {
         printf("   Already in the unf as %s", ee->str().c_str());
         evt.pop_back(); // remove from the evt
         return;
      }

   evt.back().update_parents();
   count++;
   c.en.push_back(&evt.back());
   DEBUG("   Unf.evt.back: id: %s \n ", evt.back().str().c_str());
}

// check if temp is already in the unfolding. If not, add it to unf.
Event & Unfolding:: find_or_add(const ir::Trans & t, Event * ep, Event * pr_mem)
{
   /* Need to check the event's history before adding it to the unf
    * Don't add events which are already in the unf
    */

   for (auto & ee: this->evt)
      if ((ee.trans == &t) and (ee.pre_proc == ep) and (ee.pre_mem == pr_mem) )
      {
         DEBUG("   already in the unf as event with idx = %d", ee.idx);
         return ee;
      }

   evt.push_back(Event(t, ep, pr_mem, *this));
   evt.back().update_parents(); // to make sure of conflict
   count++;
   DEBUG("   new event: id: %s \n ", evt.back().str().c_str());
   return evt.back();
}
/*
 * create a WR with:
 * - t : transition
 * - ep: pre_proc event
 * - combi: set of pre_readers
 */
Event & Unfolding:: find_or_addWR(const ir::Trans & t, Event * ep, std::vector<Event *> combi)
{
   /*
    * Need to check if there is some event with the same transition, pre_proc and pre_readers in the unf
    * Don't add events which are already in the unf
    */

   for (auto & ee: this->evt)
      if ((ee.trans == &t) and (ee.pre_proc == ep) and (ee.pre_readers == combi) )
      {
         DEBUG("   already in the unf as event with idx = %d", ee.idx);
         return ee;
      }

   DEBUG("Addr of t: %p", &t);
   evt.push_back(Event(t, ep, combi, *this));

   evt.back().update_parents(); // to make sure of conflict
   count++;
   DEBUG("   new event: id: %s \n ", evt.back().str().c_str());
   return evt.back();
}

/* Print all event of the unfolding */
void Unfolding:: uprint_debug()
{
   DEBUG("\n%p Unf.evt: ", this);
   for (auto & e : evt)
      e.eprint_debug();
}

/*
 * Print the unfolding into dot file.
 */
void Unfolding:: uprint_dot()
{
   /* Create a folder namely output in case it hasn't existed. Work only in Linux */
   DEBUG("\n%p: Unfolding.uprint_dot:", this);
   const char * mydir = "output";
   struct stat st;
   if ((stat(mydir, &st) == 0) && (((st.st_mode) & S_IFMT) == S_IFDIR))
      DEBUG(" Directory %s already exists", mydir);
   else
   {
      const int dir_err = mkdir("output", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
      if (-1 == dir_err)
         DEBUG("Directory output has just been created!");
   }

   std::ofstream fs("output/unf.dot", std::fstream::out);
   std::string caption = "Concur example";
   printf(" Unfolding is exported to dot file: \"dpu/output/unf.dot\"");
   fs << "Digraph RGraph {\n";
   fs << "label = \"Unfolding: " << caption <<"\"";
   fs << "node [shape=rectangle, fontsize=10, style=\"filled, solid\", align=right]";
   fs << "edge [style=filled]";

   for(auto const & e : evt)
   {
      if (e.is_bottom())
      {
         fs << e.idx << "[label=\"" << e.dotstr() <<" \"]\n";
         //fs << e.idx << "[label = bottom]\n";
         continue;
      }

      switch (e.trans->type)
      {
         case ir::Trans::LOC:
            fs << e.idx << "[id="<< e.idx << ", label=\" " << e.dotstr() << " \" fillcolor=yellow];\n";
            fs << e.pre_proc->idx << "->" << e.idx << "[color=brown]\n";
            break;
         case ir::Trans::WR:
            fs << e.idx << "[id="<< e.idx << ", label=\" " << e.dotstr() << " \" fillcolor=red];\n";
            fs << e.pre_proc->idx << "->" << e.idx << "[color=brown]\n";
            for (auto const & pre : e.pre_readers)
               fs << pre->idx << " -> " << e.idx << "\n";
            break;
         case ir::Trans::RD:
            fs << e.idx << "[id="<< e.idx << ", label=\" " << e.dotstr() << " \" fillcolor=palegreen];\n";
            fs << e.pre_proc->idx << "->" << e.idx << "[color=brown]\n";
            fs << e.pre_mem->idx << "->" << e.idx << "\n";

            break;
         case ir::Trans::SYN:
            fs << e.idx << "[id="<< e.idx << ", label=\" " << e.dotstr() << " \" color=lightblue];\n";
            fs << e.pre_proc->idx << "->" << e.idx << "[color=brown]\n";
            fs << e.pre_mem->idx << "->" << e.idx << "\n";

            break;
      }

      /* print conflicting edge */
      for (unsigned i = 0; i < e.dicfl.size(); i++)
         if (e.dicfl[i]->idx > e.idx ) // don't repeat drawing the same conflict
            fs << e.idx << "->" << e.dicfl[i]->idx << "[dir=none, color=red, style=dashed]\n";
   }

   fs << "}";
   fs.close();
   DEBUG(" successfully\n");
}

/*
 * Explore the entire unfolding
 */
void Unfolding:: explore(Config & C, std::vector<Event*> D, std::vector<Event*> A)
{
   Event * pe;
   if (C.en.empty() == true) return ;
   if (A.empty() == true)
       pe = *(C.en.begin()); // choose the first element
   else
   { //choose the mutual event in A and C.en to add
      for (auto a = A.begin(); a != A.end(); a++)
       for (auto e = C.en.begin(); e!= C.en.end(); e++)
          if (*e == *a)
             {
                pe = *e;
                A.erase(a);
                C.en.erase(e);
                break;
             }
   }
   C.add(*pe);
   // Alt(C,D.add(e))
   explore (C, D, A);
}

/*
 * Explore a random configuration
 */
void Unfolding::explore_rnd_config ()
{
   DEBUG ("%p: Unfolding.explore_rnd_config()",this);
   assert (evt.size () > 0);
   std::string cprintstr;
   std::string uprintstr;
   /* Initialize the configuration */
   Config c(*this);
   unsigned int i;

   while (c.en.empty() == false)
   {
      srand(time(NULL));
      i = rand() % c.en.size();
      c.add(i);
   }
   //c.cprint_debug();

   uprint_debug();
   c.cprint_dot();
   uprint_dot();
   c.compute_cex();

   return;
}
/*
 * Explore a configuration with a parameter driven by developer
 */
void Unfolding::explore_driven_config ()
{
   DEBUG ("%p: Unfolding.explore_driven_config()",this);
   assert (evt.size () > 0);
   std::string cprintstr;
   std::string uprintstr;
   /* Initialize the configuration */
   Config c(*this);
   unsigned int i, count;
   count = 0;

   srand(time(NULL));
   while (c.en.empty() == false)
   {
      i = rand() % c.en.size();
      while ( (c.en[i]->trans->proc.id == 1) && (count < 15)) // set up when we want to add event in proc 1 (e.g: after 21 events in proc 0)
      {
         i = rand() % c.en.size();
      }
      c.add(i);
      count++;
   }
   // c.cprint_debug();
   uprint_debug();
   c.compute_cex();
   c.cprint_dot();
   uprint_dot();
   return;
}
/*
 * Find all alternatives J after C and conflict with D
 */
void Unfolding:: alternative(Config & C, std::vector<Event *> D)
{
   std::vector<std::vector<Event *>> spikes;
   for (auto e: D)
   {
      // check if e in cex(C). If yes, remove from D
      for (unsigned i = 0; i < C.cex.size(); i++)
      {
         if (e == C.cex[i])
         {
            e = D.back();
            D.pop_back();
         }
      }
   }

   /*
    *  D now contains only events which is in en(C).
    *  D is a comb whose each spike is a list of conflict events D[i].dicfl
    */

      for (unsigned i = 0; i < D.size(); i++)
         spikes.push_back(D[i]->dicfl);

   std::vector<Event *> combi;
   compute_alt(0, spikes, combi);
}


void Unfolding:: compute_alt(unsigned int i, const std::vector<std::vector<Event *>> & s, std::vector<Event *> combi)
{
   for (unsigned j = 0; j < s[i].size(); j++ )
     {
        if (j < s[i].size())
        {
           combi.push_back(s[i][j]);
           if (i == s.size() - 1)
           {
              printf("   Combination:");
              for (unsigned k = 0; k < combi.size(); k++)
                 printf("%d ", combi[k]->idx);

              /*
               * Do something here
               */
              printf("\n");
           }
           else
              compute_alt(i+1, s, combi);
        }
        combi.pop_back();
     }
}

} // end of namespace

