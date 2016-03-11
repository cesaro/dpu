/*
 * pes.cc
 *
 *  Created on: Jan 12, 2016
 *      Author: tnguyen
 */

#include <cstdint>
#include <cstdio>
#include <cassert>
#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>

#include "pes.hh"
#include "misc.hh"
#include "verbosity.h"

using std::vector;
using std::fstream;
using std::string;
using namespace ir;

namespace pes{

/*
 * Methods for class Event
 */
Event::Event (Unfolding & u)
   : idx(u.count)
   , pre_proc(nullptr)
   , pre_mem(nullptr)
   , val(0)
   , localvals(0)
   , trans(nullptr)
   , color(0)
{
   unsigned numprocs = u.m.procs.size();
   unsigned mem = u.m.memsize - numprocs;
   pre_readers.reserve(numprocs);
   post_mem.reserve(numprocs);
   post_proc.reserve(numprocs);
   post_rws.reserve(mem);
   post_wr.reserve(numprocs);
   dicfl.reserve(u.m.trans.size());

   // create numprocs vectors for storing event pointers
   post_mem.resize(numprocs);
   DEBUG ("  %p: Event.ctor:", this);
}

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
   unsigned mem = u.m.memsize - numprocs;
   pre_readers.reserve(numprocs);
   post_mem.reserve(numprocs);
   post_proc.reserve(u.m.trans.size());
   post_rws.reserve(mem);
   post_wr.reserve(numprocs);
   // create numprocs vectors for storing event pointers
   post_mem.resize(numprocs);
   dicfl.reserve(u.m.trans.size());

   DEBUG ("  Event %p: Event.ctor: t %p: '%s'", this, &t, t.str().c_str());
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
{
}
   //DEBUG ("%p: Event.ctor: other %p (copy)", this, &e);


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
         {
            Event * temp = c.latest_op[i][varaddr];
            pre_readers.push_back(temp);
         }
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
/*
 * Update all events precede current event, including pre_proc, pre_mem and all pre_readers
 */
void Event::update_parents()
{
   DEBUG("   Update_parents:  ");
   if (this->is_bottom())
      return;

   Process & p  = this->trans->proc;

   switch (this->trans->type)
   {
      case ir::Trans::WR:
         pre_proc->post_proc.push_back(this);
         /* update pre_mem */
         pre_mem->post_wr.push_back(this);
         for (unsigned i = 0; i < pre_readers.size(); i++)
         {
            if ( (pre_readers[i]->is_bottom() == true) || (pre_readers[i]->trans->type == ir::Trans::WR)  )
            {
               pre_readers[i]->post_mem[i].push_back(this); // add to vector of corresponding process in pre_reader
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

bool Event:: operator == (const Event & e) const
{
   return this == &e;
}

// Overlap = operator
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

   // a LOC event has no conflict with any other transition. Leave 2 LOC trans sharing a localvar later.
   if ((this->trans->type == ir::Trans::LOC)  || (e.trans->type == ir::Trans::LOC))
      return false;

   Event * parent = pre_mem;

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

   const ir::Trans & pa_tr = *(parent->trans);

   switch (pa_tr.type)
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
   if ( (trans == e.trans) && (pre_proc == e.pre_proc)
         && (pre_mem == e.pre_mem) && (pre_readers == e.pre_readers))
      return true;

   return false;
}

/* Express an event in a string */
std::string Event::str () const
{
   const char * code = trans ? trans->code.str().c_str() : "";
   int proc = trans ? trans->proc.id : -1;
   std::string pr;

   if (pre_mem != nullptr)
      return fmt ("index: %d, %p: trans %p code: '%s' proc: %d pre_proc: %p pre_mem: %p  ",
         idx, this, trans, code, proc, pre_proc, pre_mem);
   else
	  return fmt ("index: %d, %p: trans %p code: '%s' proc: %d pre_proc: %p pre_mem(null): %p ",
	            idx, this, trans, code, proc, pre_proc, pre_mem);
}

std::string Event::dotstr () const
{
   const char * code = trans ? trans->code.str().c_str() : "";
   int proc = trans ? trans->proc.id : -1;

      return fmt ("id: %d, code: '%s' \n proc: %d , src: %d , dest: %d",
         idx, code, proc, trans->src, trans->dst);
}

/* Print all information for an event */
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
 *========= Methods of class Config===========
 */

Config::Config (Unfolding & u)
   : gstate (u.m.init_state)
   , latest_proc (u.m.procs.size (), u.bottom)
   , latest_wr (u.m.memsize - u.m.procs.size(), u.bottom)
   , latest_op (u.m.procs.size (), std::vector<Event*> (u.m.memsize - u.m.procs.size(), u.bottom))
   , unf (u)
{
   /* with the unf containing only bottom event */
   DEBUG ("Initialize a config: \n");
   DEBUG ("%p: Config.ctor", this);
   // reserve the capacity of en and cex is square root of number of trans.
	// FIXME is this necessary ? -- Cesar
   en.reserve(u.m.trans.size()*10);
   cex.reserve(u.m.trans.size()*10);

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

   DEBUG ("\n%p: Config.add: %p\n", this, e.str().c_str());
   for (unsigned int i = 0; i < en.size (); i++)
      if (e == *en[i]) add (i);
   throw std::range_error ("Trying to add an event not in enable set by a configuration");

}

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
   DEBUG (" %p: Config.__update_encex with last event e: id = %d, mem = %p", this, e.idx, &e);

   std::vector<ir::Trans> & trans    = unf.m.trans; // set of transitions in the model
   std::vector <ir::Process> & procs = unf.m.procs; // set of processes in the model

   assert(trans.size() > 0);
   assert(procs.size() > 0);

   std::vector<Trans*> enable;

   /* get set of events enabled at the state gstate    */
   gstate.enabled (enable);

   if (enable.empty() == true )
      return;

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
 * Print all the latest events of config to console
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
   std::ofstream fs("output/conf.dot", std::fstream::out);
   if (fs.is_open() != true)
      printf("Cannot open the file\n");
   printf("\n%p: configuration exported to dot file: \"dpu/output/conf.dot\"", this);
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
                  fs << p->idx << "[id="<< p->idx << ", label=\" " << p->dotstr() << " \" color=yellow];\n";
                  fs << p->pre_proc->idx << "->"<< p->idx << "[color=brown];\n";
                  break;
               case ir::Trans::RD:
                  fs << p->idx << "[id="<< p->idx << ", label=\" " << p->dotstr() << " \" color=palegreen];\n";
                  fs << p->pre_mem->idx << "->"<< p->idx << ";\n";
                  fs << p->pre_proc->idx << "->"<< p->idx << "[color=brown];\n";
                  break;
               case ir::Trans::SYN:
                  fs << p->idx << "[id="<< p->idx << ", label=\" " << p->dotstr() << " \" color=lightblue];\n";
                  fs << p->pre_mem->idx << "->"<< p->idx << ";\n";
                  fs << p->pre_proc->idx << "->"<< p->idx << "[color=brown];\n";
                  break;

               case ir::Trans::WR:
                  fs << p->idx << "[id="<< p->idx << ", label=\" " << p->dotstr() << " \" color=red];\n";
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
 *  Print the size of curent enable set
 */
void Config::__print_en() const
{
	DEBUG ("Enable set of config %p: size: %zu", this, en.size());
	   for (auto & e : en)
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
   Event * e;
   assert (evt.size () == 0);
   /* create an "bottom" event with all empty */
   e = new Event(*this);
   evt.push_back(*e);
   count++;

   bottom = &evt.back(); // using = operator
   bottom->pre_mem = bottom;
   bottom->pre_proc = bottom;

   DEBUG ("%p: Unfolding.__create_bottom: bottom %p", this, e);
}
/*
 * create an event with enabled transition and config.
 */
void Unfolding::create_event(ir::Trans & t, Config & c)
{
   Event * e = new Event(t,*this);
   //DEBUG("  Create new event: %s ", e->str().c_str());

   // create an history for new event
   e->mk_history(c);

   /* Need to check the event's history before adding it to the unf
    * Only add events that are really enabled at the current state of config.
    * Don't add events already in the unf (enalbed at the previous state)
    */

   for (auto ee :c.en)
      if ( e->is_same(*ee) == true )
      {
         DEBUG("   No addition. It is already in the unf.");
         return;
      }

   evt.push_back(*e);
   evt.back().update_parents();
   count++;
   c.en.push_back(&evt.back());
   DEBUG("   Unf.evt.back: id: %s \n ", evt.back().str().c_str());
}


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
   std::ofstream fs("output/unf.dot", std::fstream::out);
   std::string caption = "Building multiplier";
   printf("\n%p: unfolding exported to dot file: \"dpu/output/unf.dot\"", this);
   fs << "Digraph RGraph {\n";
   fs << "label = \"Unfolding: " << caption <<"\"";
   fs << "node [shape=rectangle, fontsize=10, style=filled, align=right]";
   fs << "edge [style=filled]";
   fs << "forcelabels=true; \n ";

   for(auto const & e : evt)
   {
      if (e.is_bottom())
      {
         fs << e.idx << "[label=\"bottom\"]\n";
         continue;
      }

      switch (e.trans->type)
      {
         case ir::Trans::LOC:
            fs << e.idx << "[id="<< e.idx << ", label=\" " << e.dotstr() << " \" color=yellow];\n";
            fs << e.pre_proc->idx << "->" << e.idx << "[color=brown]\n";
            break;
         case ir::Trans::WR:
            fs << e.idx << "[id="<< e.idx << ", label=\" " << e.dotstr() << " \" color=red];\n";
            fs << e.pre_proc->idx << "->" << e.idx << "[color=brown]\n";
            for (auto const & pre : e.pre_readers)
               fs << pre->idx << " -> " << e.idx << "\n";
            break;
         case ir::Trans::RD:
            fs << e.idx << "[id="<< e.idx << ", label=\" " << e.dotstr() << " \" color=palegreen];\n";
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
         if (e.dicfl[i]->idx > e.idx ) // don't repeat the relation
            fs << e.idx << "->" << e.dicfl[i]->idx << "[dir=none, color=red, style=dashed]\n";
   }

#if 0
   /* don't use set of conflict in Event class */
   for (unsigned i = 0; i < evt.size()-1; i++) // except bottom event
      {
         for (unsigned j = i + 1; j < evt.size(); j++)
         {
            if (evt[j].check_cfl(evt[i]) == true)
               fs << evt[i].idx << "->" << evt[j].idx <<"[dir=none, color=red, style=dashed]\n";
         }
      }
#endif

   fs << "}";
   fs.close();
   printf(" successfully\n");
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
   c.cprint_debug();
   uprint_debug();
   c.cprint_dot();
   uprint_dot();
   return;
}

} // end of namespace

