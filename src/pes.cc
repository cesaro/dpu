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
   unsigned mem = u.m.memsize;
   pre_readers.reserve(numprocs);
   post_mem.reserve(numprocs);
   post_proc.reserve(numprocs);
   post_rws.reserve(mem);
   post_wr.reserve(numprocs);
   dicfl.reserve(u.m.trans.size());

   // create numprocs vectors for storing event pointers
   post_mem.resize(numprocs);
   DEBUG ("%p: Event.ctor:", this);
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
   unsigned numprocs = u.m.procs.size();
   unsigned mem = u.m.memsize;
   pre_readers.reserve(numprocs);
   post_mem.reserve(numprocs);
   post_proc.reserve(10);
   post_rws.reserve(mem);
   post_wr.reserve(numprocs);
   // create numprocs vectors for storing event pointers
   post_mem.resize(numprocs);
   dicfl.reserve(u.m.trans.size());

   // DEBUG ("Event %p: Event.ctor: t %p: '%s'", this, &t, t.str().c_str());
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
    	 /*
    	  * If a LOC transition RD or WR on a local variable,
    	  * its pre_mem is previous event touching that one.
    	  * If LOC is a EXIT, no variable touched, no pre_mem
    	  * How to solve: v4 = v3 + 1; a RD for v3 but a WR for v4 (local variable)
    	  */
    	 pre_mem   = nullptr;
         break;
   }

}
/*
 * Update all events precede current event, including pre_proc, pre_mem and all pre_readers
 */
void Event::update_parents()
{
   if (this->is_bottom())
      return;

   Process & p  = trans->proc;
   // current event is one of children of its previous in the same process
   pre_proc->post_proc.push_back(this); // every event (in all types) has its pre_proc

   // LOC event has no pre_mem => exit the function
   if (this->trans->type == ir::Trans::LOC)
	   return;

   /*
    * update pre_mem for RD, WR and SYN event
    * previous event accessing the same variable, a RD, SYN or WR
    */
   // Special case for bottom event, as a WR
   if (pre_mem->is_bottom())
   {
      pre_mem->post_wr.push_back(this);
      if (this->trans->type == ir::Trans::WR)
      {
         for (unsigned i = 0; i < pre_mem->post_mem.size(); i++)
            pre_mem->post_mem[i].push_back(this); // any process has the same children

      }
      else
         pre_mem->post_mem[p.id].push_back(this);

      pre_mem->post_rws.push_back(this);
	   return;
   }

   switch (pre_mem->trans->type)
   {
      case ir::Trans::WR:
    	 // pre_meme is a WR, add this event to vector for corresponding process
          pre_mem->post_rws.push_back(this);

         if (trans->type == ir::Trans::WR)  //if the event itsefl is a WR, add it to parentś post_wr
         {
            pre_mem->post_wr.push_back(this);
            for (unsigned i = 0; i < pre_mem->post_mem.size(); i++)
               pre_mem->post_mem[i].push_back(this);
         }
         else
            pre_mem->post_mem[p.id].push_back(this);

         break;

      case ir::Trans::RD:
         pre_mem->post_rws.push_back(this);
         pre_mem->post_mem[p.id].push_back(this); // update vecotr of children for process p
         break;

      case ir::Trans::SYN:
         pre_mem->post_rws.push_back(this);
         pre_mem->post_mem[p.id].push_back(this);
         break;

      case ir::Trans::LOC:
         // nothing to do
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
 * Twon events are in conflict if they both appear in a vector of post_mem
 * for a process of its parent.
 */

bool Event::check_cfl( const Event & e ) const
{
   printf(" Start check conflict between %d and %d ", this->idx, e.idx);

   if (this->is_bottom() || e.is_bottom() || (*this == e) )
      return false;

   if (this->trans == e.trans)
      return true;

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

/* Express an event in a string */
std::string Event::str () const
{
   const char * code = trans ? trans->code.str().c_str() : "";
   int proc = trans ? trans->proc.id : -1;
   if (pre_mem != nullptr)
      return fmt ("index: %d, %p: trans %p code: '%s' proc %d pre_proc %p pre_mem %p",
         idx, this, trans, code, proc, pre_proc, pre_mem);
   else
	  return fmt ("index: %d, %p: trans %p code: '%s' proc %d pre_proc %p pre_mem(null) %p",
	            idx, this, trans, code, proc, pre_proc, pre_mem);
}

/* Print all information for an event */
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
//print post_mem
	if (post_mem.size() != 0)
		{
			printf(" Post_mem:");
			for (unsigned int i = 0; i < post_mem.size(); i++)
			{
			   printf("\n  Process %d:", i);
			   for (unsigned j = 0; j < post_mem[i].size(); j++)
			      printf(" %p   ",post_mem[i][j]);
			}
		}
	else
		   DEBUG(" No post_mem");

// print post_proc
   if (post_proc.size() != 0)
   {
      printf("\n Post_proc:");
	   for (unsigned int i = 0; i < post_proc.size(); i++)
	      printf("%p   ", post_proc[i]);
   }
   else
      DEBUG(" No post proc");

// print post_rws
   if (post_rws.size() != 0)
      {
          DEBUG("\n Post_rws:");
         for (unsigned int i = 0; i < post_rws.size(); i++)
            DEBUG("  Process %d: %p",i, post_rws[i]);
      }
      else
         DEBUG(" No post rws");
}

// store all information for dot print in a string
void Event::eprint_dot(std::string & st)
{
   st += std::to_string(idx) + "[shape=rectangle label=";
   switch (trans->type)
   {
   case ir::Trans::WR:
      st += "WR]";
      for (int unsigned i = 0; i < pre_readers.size(); i++)
         st += std::to_string(pre_readers[i]->idx) + "->" + std::to_string(idx) ;
      break;
   case ir::Trans::RD:
      st += "RD]";
         break;
   case ir::Trans::SYN:
         st += "SYN]";
         break;
   case ir::Trans::LOC:
         st += "LOC]";
         break;
   }
   st += std::to_string(pre_proc->idx) + "->" + std::to_string(idx) ;
   st += std::to_string(pre_mem->idx) + "->" + std::to_string(idx) ;

}
/*
 *========= Methods of class Config===========
 */

Config::Config (Unfolding & u)
   : gstate (u.m.init_state)
   , latest_proc (u.m.procs.size (), u.bottom)
   , latest_wr (u.m.memsize, u.bottom)
   , latest_op (u.m.procs.size (), std::vector<Event*> (u.m.memsize, u.bottom))
   , unf (u)
{
   //DEBUG ("%p: Config.ctor", this);
   // reserve the capacity of en and cex is square root of number of trans.
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
  //DEBUG (" Event passed: %s \n", e.str().c_str());
   for (unsigned int i = 0; i < en.size (); i++)
      if (e == *en[i]) add (i);
   throw std::range_error ("Trying to add an event not in enable set by a configuration");

}

void Config::add (unsigned idx)
{
   assert(idx < en.size());
   Event & e = *en[idx];
   // e.eprint_debug();

   ir::Process & p              = e.trans->proc; // process of the transition of the event
   std::vector<Process> & procs = unf.m.procs; // all processes in the machine

   // update the configuration
   e.trans->fire (gstate); //move to next state

   latest_proc[p.id] = &e; //update latest event of the process containing e.trans

   //update local variables in trans
   for (auto & i: e.trans->localvars)
   {
       latest_wr[i] = &e;
       latest_op[p.id][i] = &e;
   }

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

   en[idx] = en.back();
   en.pop_back();

   /* update en and cex set with e being added to c (before removing it from en) */
   __update_encex(e);
   /* remove the event en[idx] from the enabled set */
}

/*
 * add an event to config, store the event info to dot print in string st
 */
void Config::add (unsigned idx, std::string & st)
{
   assert(idx < en.size());
   Event & e = *en[idx];

   ir::Process & p              = e.trans->proc; // process of the transition of the event
   std::vector<Process> & procs = unf.m.procs; // all processes in the machine

   /*
    * Update the configuration:
    * - fire the transition to next state
    * - update latest_proc, latest_wr, latest_op to e (regarding e's process and variable).
    */
   e.trans->fire (gstate);

   /* stor print_dot infos in st
    * Mount new event to its pre_mem???
    */
   if (e.trans->type == ir::Trans::LOC)
   {
      //st += std::to_string(e.idx) + "[label=" + e.trans->code.str() + "]\n";
      st += std::to_string(e.pre_proc->idx) + "->"+ std::to_string(e.idx) + "\n"; // for the edge with previous event in process
   }

   else
   {
      // st += std::to_string(e.idx) + "[label=\" " + e.trans->code.str() + "\" ]\n";
      st += std::to_string(e.pre_mem->idx) + "->"+ std::to_string(e.idx) + "\n"; // for the edge with previous event
   }

   latest_proc[p.id] = &e; //update latest event of the process containing e.trans

   //update local variables in trans
   for (auto & i: e.trans->localvars)
   {
       latest_wr[i] = &e;
       latest_op[p.id][i] = &e;
   }

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

   en[idx] = en.back();
   en.pop_back();

   /* update en and cex set with e being added to c (before removing it from en)*/
   __update_encex(e);
   // __print_en();
}


/*
 * Update enabled set whenever an event e is added to c
 */
void Config::__update_encex (Event & e )
{
   //DEBUG ("%p: Config.__update_encex with new event e: id= %d, mem=%p", this, e.idx, &e);
   //printf("En.size = %zu \n", en.size());

   if (en.size() > 0)
      /* remove all events in EN conflicting with e*/
         remove_cfl(e);

   std::vector<ir::Trans> & trans    = unf.m.trans; // set of transitions in the model
   std::vector <ir::Process> & procs = unf.m.procs; // set of processes in the model

   assert(trans.size() > 0);
   assert(procs.size() > 0);

   std::vector<Trans*> enable;

   /* get set of events enabled at the state gstate    */
   gstate.enabled (enable);

   if (enable.empty() == true )
      return;

   for (auto t : enable)
   {
      //DEBUG ("\n Transition %s is enabled", t->str().c_str());
      /*
       *  create new event with transition t and add it to evt of the unf
       *  have to check evt capacity before adding to prevent the reallocation.
       */
      if (unf.evt.size () == unf.evt.capacity ())
      	 throw std::logic_error (
      	    "Tried to allocate more events than the maximum permitted");

      unf.create_event(*t, *this);
      en.push_back(&unf.evt.back()); // this copies the event and changes its prereaders. Why????
   }
}

#if 0
/*
 * string st for dot printing
 */
void Config::__update_encex (Event & e, std::string & st )
{
   //DEBUG ("%p: Config.__update_encex with

   if (en.size() > 0)
      /* remove all events in EN conflicting with e*/
         remove_cfl(e,st);

   std::vector<ir::Trans> & trans    = unf.m.trans; // set of transitions in the model
   std::vector <ir::Process> & procs = unf.m.procs; // set of processes in the model

   assert(trans.size() > 0);
   assert(procs.size() > 0);

   std::vector<Trans*> enable;

   /* get set of events enabled at the state gstate    */
   gstate.enabled (enable);

   if (enable.empty() == true )
      return;

   for (auto t : enable)
   {
      //DEBUG ("\n Transition %s is enabled", t->str().c_str());
      /*
       *  create new event with transition t and add it to evt of the unf
       *  have to check evt capacity before adding to prevent the reallocation.
       */
      if (unf.evt.size () == unf.evt.capacity ())
          throw std::logic_error (
             "Tried to allocate more events than the maximum permitted");

      unf.create_event(*t, *this);
      en.push_back(&unf.evt.back()); // this copies the event and changes its prereaders. Why????
      /*
       * add to string for dot printing
       */
      Event & e = unf.evt.back();
      switch (e.trans->type)
           {
              case ir::Trans::LOC:
                 st += std::to_string(e.idx) + "[fillcolor=green]";
                 st += std::to_string(e.pre_proc->idx) + "->" + std::to_string(e.idx) + "\n";
                 break;
              case ir::Trans::WR:
                 st += std::to_string(e.idx) + "[fillcolor=red]";
                 for (auto const & pre : e.pre_readers)
                    st += std::to_string(pre->idx) + "->" + std::to_string(e.idx) + "\n";
                 break;
              default:
                 st += std::to_string(e.idx) + "[fillcolor=blue]";
                 st += std::to_string(e.pre_mem->idx) + "->" + std::to_string(e.idx) + "\n";
           }
      st += std::to_string(e.pre_mem->idx) + "->" + std::to_string(e.idx);
   }
}
#endif

void Config::remove_cfl(Event & e)
{
   // DEBUG ("%p: Config.remove_cfl: e %p", this, &e);
   unsigned int i = 0;

   while (i < en.size())
   {
      if (e.check_cfl(*en[i]) == true)
      {
         e.dicfl.push_back(en[i]); // add en[i] to direct conflicting set of e
         cex.push_back(en[i]);
         //en.erase(en.begin() + i); // bad allocation
         en[i] = en.back();
         en.pop_back();
      }
      else   i++;
   }
}

void Config::remove_cfl(Event & e, std::string & st)
{
   // DEBUG ("%p: Config.remove_cfl: e %p", this, &e);
   unsigned int i = 0;

   while (i < en.size())
   {
      if (e.check_cfl(*en[i]) == true)
      {
         st += "edge[color=red]" + std::to_string(e.idx) + "-" + std::to_string(en[i]->idx) + "\n";
         cex.push_back(en[i]);
         //en.erase(en.begin() + i);
         en[i] = en.back();
         en.pop_back();
      }
      else   i++;
   }
}

/*
 * Print all the latest events of config to console
 */
void Config::cprint_debug () const
{
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
 * All dot script stored in st, accumulated by adding an event to the config
 */
void Config::cprint_dot(std::string &, std::string & st)
{
   //std::ofstream fs(file, std::fstream::out);
   //boost::filesystem::path dir ("../output");

  /*
   if(boost::filesystem::create_directory(dir))
   {
      printf ("Directory Created: ");
   }
   */
   std::ofstream fs("output/conf.dot", std::fstream::out);
   if (fs.is_open() != true)
      printf("Cannot open the file\n");
   fs << "Digraph RGraph {\n node [shape=circle]";

   fs << st;
   fs << "}";
   fs.close();
   printf("Print_dot done\n");
}

/*
 * cprint_dot with a fixed file
 */
void Config::cprint_dot()
{
   std::ofstream fs("output/conf.dot", std::fstream::out);
   if (fs.is_open() != true)
      printf("Cannot open the file\n");
   fs << "Digraph RGraph {\n node [shape=rectangle, style=filled]";

   for (auto e : latest_proc)
   {
     // while ((e->is_bottom() != true) && (e->color != 1))
      {
         fs << e->pre_mem->idx << "->" << e->idx;
         e->color = 1;
         e = e->pre_mem;
      }
   }

   fs << "}";
   fs.close();
   printf("Print_dot done\n");
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
   evt.reserve(1000); // maximum number of events????
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

   // DEBUG ("%p: Unfolding.__create_bottom: bottom %p", this, e);
}
/*
 * create an event with enabled transition and config.
 */
void Unfolding::create_event(ir::Trans & t, Config & c)
{
   Event * e = new Event(t,*this);
   // create an history for new event
   e->mk_history(c);
   /*
   for (auto & ee : evt)
      if ( e->is_same(ee) == true )
         return;
   */
   evt.push_back(*e);
   evt.back().update_parents();
   count++;
}

void Unfolding:: uprint_debug()
{
   DEBUG("===========Start of Unfolding===========\n");
   for (auto & e : evt)
      e.eprint_debug();
   DEBUG("===========End of unfolding=============");
}
/*
 * Print the unfolding into dot file with a string as input.
 */
void Unfolding:: uprint_dot(std::string ofile, std::string & st)
{
   std::ofstream fs(ofile, std::fstream::out);
   if (fs.is_open() != true)
      printf("Cannot open file for unf");

   fs << "Digraph RGraph {\n";
   fs << st;
   fs<< "}";
   fs.close();
}

#if 0
void Unfolding:: uprint_dot()
{
   std::ofstream fs("output/unf.dot", std::fstream::out);
   fs << "Digraph RGraph {\n";
  // fs << "node[shape = rectangle]\n";

   for(unsigned i = 0; i < evt.size(); i++)
   {
      if (evt[i].is_bottom())
      {
         fs << evt[i].idx << "\n";
         continue;
      }

      fs << evt[i].idx <<"[label =\" " << evt[i].idx << "\"] \n";
      fs << "edge [color=black]";
      switch (evt[i].trans->type)
      {
         case ir::Trans::LOC:
            fs << evt[i].idx << "[fillcolor=green]";
            fs << evt[i].pre_proc->idx << "->" << evt[i].idx << "\n";
            break;
         case ir::Trans::WR:
            fs << evt[i].idx << "[fillcolor=red]";
            for (auto const & pre : evt[i].pre_readers)
               fs << pre->idx << "->" << evt[i].idx << "\n";
            break;
         default:
            fs << evt[i].idx << "[fillcolor=blue]";
            fs << evt[i].pre_mem->idx << "->" << evt[i].idx << "\n";
      }

      /* draw conflicting edge*/
      fs << "edge [dir=none, color=red] \n";
      for (unsigned j = i; j < evt.size(); j++)
         if (evt[j].check_cfl(evt[i]) == true)
            fs << evt[i].idx << "->" << evt[j].idx <<"\n";
      /*
       * Cannot scope with indirect conflict.
       */
   }

   fs << "}";
   fs.close();
}
#endif

void Unfolding:: uprint_dot()
{
   std::ofstream fs("output/unf.dot", std::fstream::out);
   fs << "Digraph RGraph {\n";
   fs << "node [shape=rectangle, fontsize=10, style=filled, align=right]";
   fs << "forcelabels=true; \n ";

   for(auto const & e : evt)
   {
      if (e.is_bottom())
      {
         fs << e.idx << "[label=\"bottom\"]\n";
         continue;
      }

      // fs << e.idx <<" [xlabel=\" " << e.trans->code.str() << " \"]\n";

      switch (e.trans->type)
      {
         case ir::Trans::LOC:
            fs << e.idx << " [label= \"ID:  " << e.idx <<",  Proc: " << e.trans->proc.id <<"\nsrc: " <<e.trans->src << ",  dest: " << e.trans->dst <<"\ncode: "<< e.trans->code.str() << " \" color=yellow] \n";
            fs << e.pre_proc->idx << "->" << e.idx << "\n";
            break;
         case ir::Trans::WR:
            fs << e.idx << " [label= \"ID:  " << e.idx <<",  Proc: " << e.trans->proc.id <<"\nsrc: " <<e.trans->src << ",  dest: " << e.trans->dst <<"\ncode: "<< e.trans->code.str() << " \" color=red] \n";
            for (auto const & pre : e.pre_readers)
               fs << pre->idx << " -> " << e.idx << "\n";
            break;
         default:
            fs << e.idx << " [label= \"ID:  " << e.idx <<",  Proc: " << e.trans->proc.id <<"\nsrc: " <<e.trans->src << ",  dest: " << e.trans->dst <<"\ncode: "<< e.trans->code.str() << " \" color=lightblue] \n";
            fs << e.pre_mem->idx << "->" << e.idx << "\n";
            break;
      }
      /* print conflicting edge */
      for (unsigned i = 0; i < e.dicfl.size(); i++)
         fs << e.idx << "->" << e.dicfl[i]->idx << "[dir=none, color=red, style=dashed]\n";
   }

#if 0
   /* use post_rws to print but there are problems with LOC events */
   for (auto & e : evt)
   {
      fs << e.idx << " -> { ";
      for (auto child: e.post_rws)
         fs << child->idx << " ";
      fs << " };\n";

      if (e.trans->type == ir::Trans::LOC)
         fs << e.pre_proc->idx << "->" << e.idx << ";\n";

   }
#endif


   fs << "}";
   fs.close();
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
   std::string cprintstr;
   std::string uprintstr;
   Config c(*this);

   while (c.en.empty() == false)
   {
      c.add(1, cprintstr);
   }

   c.cprint_dot();

   //uprint_dot("output/unf.dot", uprintstr); // problems: nothing modifies uprintstr
   this->uprint_debug();
   uprint_dot();
   return;
}

} // end of namespace

