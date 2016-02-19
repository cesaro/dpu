/*
 * pes.cc
 *
 *  Created on: Jan 12, 2016
 *      Author: tnguyen
 */

#include <cstdint>
#include <cassert>
#include <iostream>

#include "pes.hh"
#include "misc.hh"
#include "verbosity.h"

using std::vector;
using namespace ir;

namespace pes{

/*
 * Methods for class Event
 */
Event::Event()
   : pre_proc(nullptr)
   , pre_mem(nullptr)
   , val(0)
   , localvals(0)
   , trans(nullptr)
{
   pre_readers.clear();
   DEBUG ("%p: Event.ctor:", this);
}

Event::Event(const Trans & t)
   : pre_proc(nullptr)
   , pre_mem(nullptr)
   , val(0)
   , localvals(0)
   , trans(&t)

{
   pre_readers.clear();
   // DEBUG ("Event %p: Event.ctor: t %p: '%s'", this, &t, t.str().c_str());
}

bool Event::is_bottom ()
{
   return this->pre_proc == this;
}

//set up 3 attributes: pre_proc, pre_mem and pre_readers and update event's parents
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
   if (this->is_bottom() == true)
      return;

   ir::Process & p = this->trans->proc;
   std::vector<Process> & procs = c.unf.m.procs;

   /*
    * e's parent is the latest event of the process
    * for all events, initialize pre_proc of new event by the latest event of config (in its process)
    */
   pre_proc = c.latest_proc[p.id];

   switch (trans->type)
   {
      case ir::Trans::RD:
    	 pre_mem  = c.latest_op[p.id][trans->var];
         // pre_readers stays empty for RD events
         break;

      case ir::Trans::WR:
    	 pre_mem  = c.latest_wr[trans->var];
        /*
         * set pre-readers = set of latest events which use the variable copies of all processes
         * size of pre-readers is numbers of copies of the variable = number of processes
         */
         for (unsigned int i = 0; i < procs.size(); i++)
         {
        	 Event * temp = c.latest_op[i][trans->var];
        	 pre_readers.push_back(temp);
         }
         break;

      case ir::Trans::SYN:
         pre_mem  = c.latest_wr[trans->var];
         break;

      case ir::Trans::LOC:
         // nothing to do
    	 // pre_mem   = nullptr;
         break;
   }
   this->eprint_debug();
   // update the previous events (called parents) of the current
   // update_parents();
}

void Event::update_parents()
{
   if (this->is_bottom())
      return;

   Process & p  = trans->proc;
   Event * prt1, *prt2;
   prt1 = this->pre_proc;
   prt2 = this->pre_mem; // previous event accessing the same variable

   prt1->post_proc.push_back(this)  ; // parent 1 = pre_proc

   switch (prt2->trans->type)
   {
      case ir::Trans::WR:
         prt2->post_mem[p.id].push_back(this); // add a child to vector corresponding to process p
         if (trans->type == ir::Trans::WR)  //if the event itsefl is a WR, add it to parentś post_wr
            prt2->post_wr[p.id].push_back(this);
         break;

      case ir::Trans::RD:
         prt2->post_rws.push_back(this);
         break;

      case ir::Trans::SYN:
         prt2->post_rws.push_back(this);
         break;

      case ir::Trans::LOC:
         break;
   }

}

bool Event:: operator == (const Event & e) const
{
   return this == &e;
}

Event & Event:: operator  = (const Event & e)
{
   pre_proc = e.pre_proc;

   for (auto & it:post_proc)
   for (auto & i:e.post_proc)
      it = i;

   pre_mem = e.pre_mem;

   for (auto & it:post_mem)
   for (auto & i:e.post_mem)
      it = i;

   for (auto & it:pre_readers)
   for (auto & i :e.pre_readers)
      it = i;

   for (auto & it:post_wr)
   for (auto & i :e.post_wr)
      it = i;

   for (auto & it:post_rws)
   for (auto & i :e.post_rws)
      it = i;

   val = e.val;

  // for (auto & it:localvals)
  // for (auto & i :e.localvals)
     // it = i;

   trans = e.trans;

   return *this;
}

bool Event::check_cfl( const Event & e ) const
{
   printf(" Start check conflict");
   Event & parent = *(e.pre_mem);
   const ir::Trans & pa_tr = *(parent.trans);

   switch (pa_tr.type)
   {
      case ir::Trans::RD:
       for (auto const & it: parent.post_rws)
          if (*it == e)
              return true;
       break;

     case ir::Trans::WR:
        for (auto const & proc: parent.post_mem)
         for (auto const & it: proc)
              if (*it == e)
                 return true;
        break;

     case ir::Trans::SYN:
        for (auto const & i: parent.post_rws )
        if (*i == e)
           return true;
        break;

     case ir::Trans::LOC:
        Event & parent_loc = *(e.pre_proc);
        for (auto const  & i: parent_loc.post_proc)
           if (*i == e)
              return true;
        break;
   }

   return false;

}


std::string Event::str () const
{
   const char * code = trans ? trans->code.str().c_str() : "";
   if (pre_mem != nullptr)
      return fmt ("%p: trans %p code: '%s' pre_proc %p pre_mem %p",
         this, trans, code, pre_proc, pre_mem);
   else
	  return fmt ("%p: trans %p code: '%s' pre_proc %p pre_mem(null): %p",
	            this, trans, code, pre_proc, pre_mem);
}

void Event::eprint_debug() const
{
	DEBUG ("Event: %s", this->str().c_str());
	if (pre_readers.size() != 0)
	{
		DEBUG(" Pre_readers:");
		for (unsigned int i = 0; i < pre_readers.size(); i++)
			DEBUG("  Process %d: %p",i, pre_readers[i]);
	}
	else
	   DEBUG(" No pre_readers");
}
/*
 * Methods of class Config
 */

Config::Config (Unfolding & u)
   : gstate (u.m.init_state)
   , latest_proc (u.m.procs.size (), u.bottom)
   , latest_wr (u.m.memsize, u.bottom)
   , latest_op (u.m.procs.size (), std::vector<Event*> (u.m.memsize, u.bottom))
   , unf (u)
{
   DEBUG ("%p: Config.ctor", this);
   print_debug ();
   // initialize all attributes for an empty config
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
 * Add an event to a configuration
 */

void Config::add_any ()
{
   // the last event in enable set
   add (en.size () - 1);
}

void Config::add (const Event & e)
{
  DEBUG (" Event passed: %s \n", e.str().c_str());
   for (unsigned int i = 0; i < en.size (); i++)
      if (e == *en[i]) add (i);
   throw std::range_error ("Trying to add an event not in enable set by a configuration");

}

void Config::add (unsigned idx)
{
   assert(idx < en.size());
   Event & e = *en[idx];
   DEBUG("Start adding an event:");
   e.eprint_debug();
  // DEBUG (" Event to add: %s \n", e.str().c_str());
   ir::Process & p              = e.trans->proc; // process of the transition of the event
   std::vector<Process> & procs = unf.m.procs; // all processes in the machine

   // update the configuration
   e.trans->fire (gstate); //move to next state

   latest_proc[p.id] = &e; //update latest event of the process containing e.trans

   //update local variables in trans
   for (auto & i: e.trans->localvars)
       latest_wr[i] = &e;

   //update other attributes according to the type of transition.
   switch (e.trans->type)
   {
   case ir::Trans::RD:
      latest_op[p.id][e.trans->var] = &e; // update only latest_op
      break;

   case ir::Trans::WR:
      latest_wr[e.trans->var] = &e; // update latest wr event for the variable var
      for (unsigned int i = 0; i < procs.size(); i++)
         latest_op[i][e.trans->var] = &e;
      break;

   case ir::Trans::SYN:
      latest_wr[e.trans->var]=&e;
      break;

   case ir::Trans::LOC:
      //nothing to do with LOC
      break;
   }
   printf("Print new config: \n");
   this->print_debug(); // print all latests.
   //printf("\nBottom event:");
  // this->unf.bottom->eprint_debug();
   // update en and cex set with e being added to c (before removing it from en)
   __update_encex(e);
   // remove the event en[idx] from the enabled set
   en[idx] = en.back();
   en.pop_back();
}
/*
 * Update enabled set whenever an event e is added to c
 */
void Config::__update_encex (const Event & e )
{
   DEBUG ("%p: Config.__update_encex: e %p", this, &e);
   //if (en.size() > 0)
     // remove_cfl(e);

   std::vector<ir::Trans> & trans = unf.m.trans; // set of transitions in the model
   std::vector <ir::Process> & procs = unf.m.procs; // set of processes in the model
   assert(trans.size() > 0);
   assert(procs.size() > 0);

   std::vector<Trans*> enable;

   gstate.enabled (enable);

   for (auto t : enable)
   {
      DEBUG ("\n Transition %s is enabled", t->str().c_str());
      //create new event with transition t and add it to evt of the unf
      unf.evt.emplace_back(*t);
      // create an history for new event
      unf.evt.back().mk_history(*this);
      printf("Before: ");
      __print_en();
      // add new event (pointer) into the enabled set
      en.push_back(&unf.evt.back()); // this copies the event and changes its prereaders. Why????
      printf("After: ");
      __print_en();
   }

}

void Config::remove_cfl(const Event & ee)
{
   DEBUG ("%p: Config.remove_cfl: e %p", this, &ee);
   unsigned int i = 0;
   while (i < en.size())
   {
      if (ee.check_cfl(*en[i]) == true)
      {
         cex.push_back(en[i]);
         en.erase(en.begin() + i);
      }
      else   i++;
   }
   //printf("\nfinish remove cfl, en.size %zu \n", en.size());
}

/*
 * Print all the latest events of config to console
 */
void Config::print_debug () const
{
   DEBUG ("%p: latest_proc:", this);
   for (auto & e : latest_proc)
      DEBUG (" %s", e->str().c_str());
	  //printf("%p \n", e);

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
 *  Print the size of curent enable set
 */
void Config::__print_en() const
{
	DEBUG ("Enable set of config %p: size: %zu", this, en.size());
	   for (auto & e : en)
		   e->eprint_debug();
	   //DEBUG (" %s", e->str().c_str());
}

/*
 * Methods for class Unfolding
 */
Unfolding::Unfolding (ir::Machine & ma)
   : m (ma)
{
   DEBUG ("%p: Unfolding.ctor: m %p", this, &m);
   __create_bottom ();
}

void Unfolding::__create_bottom ()
{
   Event * e;
   assert (evt.size () == 0);
   // create an "bottom" event with all empty
   evt.emplace_back();
   e = & evt[0];

   e->pre_proc = e;
   e->pre_mem = e;
   e->pre_readers.clear();

   bottom = e; 
   DEBUG ("%p: Unfolding.__create_bottom: bottom %p", this, e);
   bottom->eprint_debug();
}

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
   printf ("--------Start Unfolding.explore_rnd_config----------\n");
   assert (evt.size () > 0);
   DEBUG ("Create an empty config");
   Config c(*this);
  // c.print_debug (); // whenever print c, we got segmentation fault, only for WR event
   while (c.en.empty() == false)
      c.add(0); // take the first event in enable set.

   printf("The End. No more enabled");
}

} // end of namespace

