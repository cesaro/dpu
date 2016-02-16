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
{
   trans = nullptr;
   val = 0;
   for (auto & lval: localvals)
     lval = 0;
   pre_proc = nullptr;
   pre_mem  = nullptr;
   pre_readers.clear();
   post_mem.clear();
   post_proc.clear();
   post_rws.clear();
   post_wr.clear();
}

Event::Event(Trans & t)
{
   this->trans = &t;
   val      = 0;
   for (auto & lval: localvals)
  	 lval = 0;
   pre_proc = nullptr;
   pre_mem  = nullptr;
   pre_readers.clear();
   post_mem.clear();
   post_proc.clear();
   post_rws.clear();
   post_wr.clear();
}
//set up 3 attributes: pre_proc, pre_mem and pre_readers
void Event::mk_history(Config & c)
{
   ir::Process & p = this->trans->proc;
   std::vector<Process> & procs = c.unf.m.procs;

   printf("\nmk_his: Id of process:%d, type of trans is %s \n", p.id, trans->type_str());
   //e's parent is the latest event of the process
   pre_proc = c.latest_proc[p.id];

   switch (trans->type)
   {
      case ir::Trans::RD:
         pre_mem  = c.latest_op[p.id][trans->addr];

         for(unsigned int i = 0; i< procs.size(); i++)
            pre_readers.push_back(nullptr);
         break;

      case ir::Trans::WR:
         pre_mem  = c.latest_wr[trans->addr];
        // set pre-readers = set of latest events which use the variable copies of all processes.
          	    //size of pre-readers is numbers of copies of the variable = number of processes
         for(auto pr : procs)
            pre_readers.push_back(c.latest_op[pr.id][trans->addr]);
         break;

      case ir::Trans::SYN:
    	 pre_mem  = c.latest_op[p.id][trans->addr];
    	 break;

      case ir::Trans::LOC:
    	 pre_mem   = nullptr;
    	 for(unsigned int i = 0; i< procs.size(); i++)
    	    pre_readers.push_back(nullptr);
    	 break;
   }
   printf("pre_proc: %p \n", pre_proc);
   printf("pre_mem: %p \n", pre_mem);
   for(unsigned int i = 0; i< procs.size(); i++)
     printf("pre_readers: %p \n", pre_readers[i]);
}

void Event::update_parents()
{
	printf("Dien a\n");
	//Process & p  = trans->proc;

    pre_proc->post_proc.push_back(this)  ; // parent 1 = pre_proc

    //DEBUG ("%s \n", pre_proc->str().c_str());

   //parent 2 = pre_mem
   DEBUG("%s \n", (*pre_mem).trans->type_str());
   printf("Dien a\n");
#if 0
   switch (pre_mem->trans->type)
   {
      case ir::Trans::WR:
    	 printf("this is a WR");
		 pre_mem->post_mem[p.id].push_back(this); // add a child to vector corresponding to process p
		 if (trans->type == ir::Trans::WR)  //if the event itsefl is a WR, add it to parentś post_wr
	        pre_mem->post_wr[p.id].push_back(this);
	     break;
      case ir::Trans::RD:
    	 printf("this is a RD");
    	 pre_mem->post_rws.push_back(this);
         break;
      case ir::Trans::SYN:
         pre_mem->post_rws.push_back(this);
	     break;
      case ir::Trans::LOC:
         break;
   }
#endif

}

bool Event:: operator == (const Event & e) const
{
   return this == &e;
}

Event & Event:: operator  = (const Event & e)
{
   *pre_proc = *(e.pre_proc);

   for (auto& it:post_proc)
   for (auto& i:e.post_proc)
      it = i;

   *pre_mem = *(e.pre_mem);

   for (auto& it:post_mem)
   for (auto& i:e.post_mem)
      it = i;

   for (auto& it:pre_readers)
   for (auto& i :e.pre_readers)
      it = i;

   for (auto& it:post_wr)
   for (auto& i :e.post_wr)
      it = i;

   for (auto& it:post_rws)
   for (auto& i :e.post_rws)
      it = i;

   val = e.val;

   for (auto& it:localvals)
   for (auto& i :e.localvals)
      it = i;

   trans = e.trans;

   return *this;
}

bool Event::check_cfl(Event & e)
{
   printf("start check conflict");
   Event & parent = *(e.pre_mem);
   ir::Trans & trans = parent.getTrans();

   switch (trans.type)
   {
      case ir::Trans::RD:
    	 for (auto const it: parent.post_rws)
    	    if (*it == e)
	           return true;
    	 break;

	  case ir::Trans::WR: // only access one variable
	     for (auto const proc : parent.post_mem)
	    	for (auto const it: proc)
	           if (*it == e)
	              return true;
	     break;

	  case ir::Trans::SYN:
		  for (auto const i: post_rws )
		  if (*i == e)
		     return true;
		  break;

	  case ir::Trans::LOC:
	     Event & parent_loc = *(e.pre_proc);
	     for (auto const i: parent_loc.post_proc)
	     if (*i == e)
	     	return true;
	     break;
   }

   return false;
}


std::string Event::str () const
{
   const char * code = trans ? trans->code.str().c_str() : "";
   return fmt ("%p trans %p '%s' pre %p %p",
         this, trans, code, pre_proc, pre_mem);
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
  // DEBUG ("%p: Config.ctor: u %p", this, unf);

   print_debug ();

   // initialize all attributes for an empty config
   __update_encex(unf.evt.back());
}

void Config::print_debug ()
{
   
   DEBUG ("%p: latest_proc:", this);
   for (auto & e : latest_proc) DEBUG (" %s", e->str().c_str());
   DEBUG ("%p: latest_wr:", this);
   for (auto & e : latest_wr) DEBUG (" %s", e->str().c_str());
   DEBUG ("%p: latest_op:", this);
   for (size_t pid = 0; pid < latest_op.size(); ++pid)
   {
      DEBUG (" Process %zu", pid);
      for (auto & e : latest_op[pid]) DEBUG ("  %s", e->str().c_str());
   }
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

void Config::add(Event & e)
{
   DEBUG (" Event e: %s \n", e.str().c_str());
   ir::Process & p              = e.trans->proc;
   std::vector<Process> & procs = unf.m.procs;
   //update e's parents
   e.update_parents();
#if 0
   // update the configuration
   e.trans->fire (gstate); //update new global states

   latest_proc[p.id] = &e; //update latest event of the process

   printf("latest event of the proc %d is: %p \n", p.id, latest_proc[p.id]);

   //update local variables in trans
   for (auto i: e.trans->localaddr)
       latest_wr[i] = &e;

   //update particular attributes according to type of transition.
   switch (e.trans->type)
   {
      case ir::Trans::RD:
    	latest_op[p.id][e.trans->addr] = &e;
        break;

      case ir::Trans::WR:
    	 latest_wr[e.trans->addr] = &e; // update latest wr event for the variable s.addr
    	 for (auto & p: procs)
    	    latest_op[p.id][e.trans->addr] = &e;
    	 break;

      case ir::Trans::SYN:
       	 latest_wr[e.trans->addr]=&e;
       	 break;

      case ir::Trans::LOC:
    	 latest_proc[p.id] = &e;
         break;
   }
   en.pop_back(); // remove the event added to the config
   printf("enable set now size is: %zu \n",en.size());
   __update_encex(e);
#endif
}

void Config::__update_encex (Event & e)
{
   printf("start update_encex\n");
#if 0
   if (en.size() > 0)
      remove_cfl(e);
   else
	   printf("Oh la la \n");
#endif

   std::vector<ir::Trans> & trans = unf.m.trans;
   std::vector <ir::Process> & procs = unf.m.procs;
   assert(trans.size() > 0);
   assert(procs.size() > 0);

   for (auto & t: trans)
   {
	   printf("t.src: %d and t.dest: %d, t.proc.id: %d \n", t.src, t.dst, t.proc.id);
     if (t.enabled(gstate) == true)
     {
    	printf("t is enabled\n");
    	unf.evt.emplace_back(t);
        printf("current event: %p",&unf.evt.back());
        unf.evt.back().mk_history(*this); // create an history for new event
    	en.push_back(&unf.evt.back());
     }
   }
   printf("en.size: %zu\n", en.size());
}

void Config::remove_cfl(Event & e)
{
   printf("start remove conflict events \n");
   unsigned int i = 0;
   while (i < en.size())
   {
      printf("Lan thu i %d \n", i);
	  if (e.check_cfl(*en[i]) == true)
      {
	     cex.push_back(en[i]);
         en.erase(en.begin() + i);
      }
      else
    	 i++;
   }
   printf("\nfinish remove cfl, en.size %zu \n", en.size());
}

/*
 * Methods for class Unfolding
 */
Unfolding::Unfolding (ir::Machine & ma)
   : m (ma)
{
   DEBUG ("%p: Unfolding.ctor: m %p", this, &m);
   __create_botom ();
}

void Unfolding::__create_botom ()
{
   Event * e;

   // create an "bottom" event with all empty
	evt.emplace_back();
   e = & evt[0];

   e->pre_proc = e;
   e->pre_mem = e;

   bottom = e; 
   DEBUG ("%p: Unfolding.__create_bottom: bottom %p", this, e);
}

bool Unfolding::is_bottom (Event * e)
{
   return e->pre_proc == e;
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
   Event * e;
   printf ("--------Start Unfolding.explore_rnd_config\n");
  // assert (evt.size () == 0);
   printf("Creat an empty config:\n");
   Config c(*this);
   int count=0;
#if 0
   while (c.en.empty() == false)
   {
	   printf(" \n\nthis is the %d \n ", count++);
	   e = c.en.back();
	   c.add(*e);
   }
   printf("no more enabled");
#endif
   if (c.en.empty() == false)
      {
   	   printf(" \n\nthis is the %d \n ", ++count);
   	   e = c.en.back();
   	   c.add(*e);
      }
      printf("no more enabled");
}

} // end of namespace

