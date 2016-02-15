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

   ir::Trans & t   = this->getTrans();
   ir::Process & p = this->getProc();
   std::vector<Process> & procs = c.gstate->getSProcs();

   //e's parent is the latest event of the process
   pre_proc = c.latest_proc[p.id];

   printf("it is ok here");

   switch (t.type)
   {
      case ir::Trans::RD:
         pre_mem  = c.latest_global_rdwr[p.id][t.addr];
         for(unsigned int i = 0; i< procs.size(); i++)
            pre_readers.push_back(nullptr);
         break;

      case ir::Trans::WR:
         pre_mem  = c.latest_global_wr[t.addr];
        // set pre-readers = set of latest events which use the variable copies of all processes.
          	    //size of pre-readers is numbers of copies of the variable = number of processes
         for(auto pr : procs)
            pre_readers.push_back(c.latest_global_rdwr[pr.id][t.addr]);
         break;

      case ir::Trans::SYN:
    	 pre_mem  = c.latest_global_rdwr[p.id][t.addr];
    	 break;
      case ir::Trans::LOC:
    	 pre_proc  = c.latest_proc[p.id];
    	 pre_mem   = nullptr;
    	 break;
   }
}

void Event::update_parents()
{
   Event & prt2 = *pre_mem;
   Process & p  = trans->proc;

   pre_proc->post_proc.push_back(this)  ;

   switch (prt2.trans->type)
   {
      case ir::Trans::WR:
		 prt2.post_mem[p.id].push_back(this); // add a child to vector corresponding to process p
		 if (trans->type == ir::Trans::WR)  //if the event itsefl is a WR, add it to parentś post_wr
	        prt2.post_wr[p.id].push_back(this);
	     break;
      case ir::Trans::RD:
    	 prt2.post_rws.push_back(this);
         break;
      case ir::Trans::SYN:
         prt2.post_rws.push_back(this);
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
/*
 * Methods of class Config
 */

Config::Config(Unfolding & u)
: unf(u)
{
   printf("start creating config\n");
   gstate = new State(u.m.init_state);
   // INITIALIZE ALL ATTRIBUTES FOR AN EMPTY CONFIG

   // set the latest events of all processes are bottom event: unf.evt.front
   for (unsigned int i = 0; i < unf.m.procs.size(); i++)
   	  latest_proc.push_back(& unf.evt.front());

    //???? set the latest write event on local variable is ???
      latest_local_wr.push_back(nullptr);

   for (unsigned int j = 0; j < unf.m.memsize; j++)
      latest_global_wr.push_back(& u.evt.front());

   for (unsigned int i = 0; i < unf.m.procs.size(); i++)
   {
      std::vector<Event *> v;
      // create a vector of variables for a proc
	  for (unsigned int j = 0; j < unf.m.memsize; j++)
	     v.push_back(& u.evt.front());
      // add a vector of a proc to latest_global_wr
	  latest_global_rdwr.push_back(v);
   }

   __update_encex(unf.evt.back());
}

Config:: Config(Config & c)
: latest_proc (c.latest_proc)
, latest_global_wr (c.latest_global_wr)
, latest_global_rdwr (c.latest_global_rdwr)
, latest_local_wr (c.latest_local_wr)
, unf(c.unf)
{
   gstate = c.gstate;
}

/*
 * Add an event to a configuration
 */

void Config::add(Event & e)
{
   printf("start add event e");

   ir::State & gs               = *gstate;
   ir::Trans & tran             = e.getTrans();
   ir::Process & p              = e.getProc();
   std::vector<Process> & procs = unf.m.getProcs();
   printf(" tran.proc.id %d \n", tran.proc.id);
  // e.update(*this); //dont need to update the history of e because it is set up at the time of creation.
  // e.update_parents();

   // update the configuration
   gstate = tran.fire(gs); //update new global states

   latest_proc[p.id] = &e; //update latest event of the process

   printf("latest event of the proc %d is: %p \n", p.id, latest_proc[p.id]);

   //update local variables in trans
   for (auto i: tran.localaddr)
       latest_local_wr[i] = &e;

   //update particular attributes according to type of transition.
   switch (tran.type)
   {
      case ir::Trans::RD:
    	latest_global_rdwr[p.id][tran.addr] = &e;
        break;

      case ir::Trans::WR:
    	 latest_global_wr[tran.addr] = &e; // update latest wr event for the variable s.addr
    	 for (auto p: procs)
    	    latest_global_rdwr[p.id][tran.addr] = &e;
    	 break;

      case ir::Trans::SYN:
       	 latest_global_wr[tran.addr]=&e;
       	 break;

      case ir::Trans::LOC:
    	 latest_proc[p.id] = &e;
         break;
   }
   __update_encex(e);

}

void Config::__update_encex (Event & e)
{
   printf("start update_encex\n");

   //if (en.size() > 0)
      //remove_cfl(e);
   ir::State & gs = *gstate;
   //std::vector<ir::Trans> & trans = gs.m.getTrans(); // all trans of the machine
   std::vector<ir::Trans> & trans = gs.m.trans; // all trans of the machine
   std::vector <ir::Process> & procs = gs.m.procs; // all procs of the machine
   assert(trans.size() > 0);
   assert(procs.size() > 0);

   for (auto & t: trans)
   {
	   printf("t.src: %d and t.dest: %d, t.proc.id: %d \n", t.src, t.dst, t.proc.id);
     if (t.enabled(gs) == true)
     {
    	unf.evt.emplace_back(t);
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
Unfolding::Unfolding (ir::Machine & ma): m(ma)
{
	evt.emplace_back(); // create an "bottom" event with all empty
	evt.back().pre_mem  = &evt.back(); // bottom event point to itself
	evt.back().pre_proc  = &evt.back(); // bottom event point to itself
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
   printf(" en.size %zu \n", c.en.size());
   int count = 1;
   while (c.en.empty() == false)
   {
	   e = c.en.back();
	   printf("The event number %d, with trans is %d \n", count, e->trans->proc.id);
	   count ++;
	   c.add(*e);
	   c.en.pop_back();
   }
   printf("no more enabled");

}

} // end of namespace

