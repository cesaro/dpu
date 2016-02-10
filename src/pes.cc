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
	val =0;
	for (auto i: localvals)
	   i = 0;
	pre_proc = nullptr;
	pre_mem  = nullptr;
	pre_readers.clear();
	post_mem.clear();
	post_proc.clear();
	post_rdsyn.clear();
	post_wr.clear();
}

Event::Event(Trans & t)
{
  this->trans = &t;
  val      = 0;
  for (auto i: localvals)
  	 i = 0;
  pre_proc = nullptr;
  pre_mem  = nullptr;
  pre_readers.clear();
  post_mem.clear();
  post_proc.clear();
  post_rdsyn.clear();
  post_wr.clear();
}

void Event::update(Config & c)
{
  ir::Trans & t   = this->getTrans();
  ir::Process & p = this->getProc();
  std::vector<Process> & procs = c.gstate->getSProcs();
  pre_proc = c.latest_proc[p.id];//e's parent is the latest event of the process
  printf("it is ok here");
  switch (t.type)
  {
     case ir::Trans::RD:
        pre_mem  = c.latest_global_rdwr[p.id][t.addr];
        break;

     case ir::Trans::WR:
        pre_mem  = c.latest_global_wr[t.addr];
        for(auto i: procs)
           pre_readers[i.id] = c.latest_global_rdwr[i.id][t.addr]; // the latest global event of the process
        //for (auto & it: post_mem)
        //   for (auto & i: it)
        //      i = nullptr;
  	    // set pre-readers = set of latest events which use the variable copies of all processes.
  	    //size of pre-readers is numbers of copies of the variable = number of processes


        break;
     case ir::Trans::SYN:
    	pre_mem  = c.latest_global_rdwr[p.id][t.addr];
    	break;
     case ir::Trans::LOC:
    	pre_proc  = c.latest_proc[p.id];
    	//post_proc = nullptr;
   	    break;
  }
  update_parents(); // update parent of the event
}

void Event::update_parents()
{
   Event * prt; //parent
   Process & p = this->trans->proc;
   Trans & tr  = *(this->trans);
   prt = this->pre_proc;
   prt->post_proc[p.id] = this;

   if (tr.type != ir::Trans::LOC)
   {
      prt = this->pre_mem;
      switch (prt->trans->type)
      {
      case ir::Trans::WR:
		 prt->post_mem[p.id].push_back(this);
		 if (tr.type == ir::Trans::WR)
	        prt->post_wr[p.id].push_back(this);
	     break;
      case ir::Trans::RD:
    	 prt->post_rdsyn.push_back(this);
         break;
      case ir::Trans::SYN:
         prt->post_rdsyn[p.id] = this;
	     break;
      default: break;
      }
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

   for (auto& it:post_rdsyn)
   for (auto& i :e.post_rdsyn)
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
   Event & parent = *(e.pre_mem);
   ir::Trans & trans = parent.getTrans();

   switch (trans.type)
   {
      case ir::Trans::RD:
    	 for (auto const it: parent.post_rdsyn)
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
		  for (auto const i: post_rdsyn )
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
  // e.update(*this); //update the event

   // update the configuration
   gstate = tran.fire(gs); //update new global states
#if 0
   latest_proc[p.id] = &e; //update latest event of the process

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
#endif
   __update_encex(e);

}

void Config::__update_encex (Event & e)
{
   printf("start update_encex\n");

   //if (en.size() > 0)
     // remove_cfl(e);
   ir::State & gs = *gstate;
   //std::vector<ir::Trans> & trans = gs.m.getTrans(); // all trans of the machine
   std::vector<ir::Trans> & trans = gs.m.trans; // all trans of the machine

   std::vector <ir::Process> & procs = gs.m.getProcs(); // all procs of the machine
   assert(trans.size() > 0);
   assert(procs.size() > 0);

   for (auto & t: trans)
   {
	   printf("t.src: %d and t.dest: %d, t.proc.id: %d \n", t.src, t.dst, t.proc.id);
     if (t.enabled(gs) == true)
     {
    	printf("size of evt: %zu \n", unf.evt.size());
    	unf.evt.emplace_back(t);
    	printf("size of evt: %zu \n", unf.evt.size());

    	en.push_back(&unf.evt.back());
     }
   }
}

void Config::remove_cfl(Event & e)
{
   printf("start remove cfl\n");
   for (auto ep = en.begin(); ep != en.end(); ep++)
      if (e.check_cfl(**ep) == true)
      {
	     cex.push_back(*ep);
         en.erase(ep);
      }
   printf("finish remove cfl, en.size %zu \n", en.size());

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
   printf ("=======Start Unfolding.explore_rnd_config\n");
  // assert (evt.size () == 0);
   printf("Creat an empty config:\n");
   Config c(*this);
   printf(" en.size %zu \n", c.en.size());
   int dem = 1;
   while (c.en.empty() == false)
   {

	   e = c.en.back();
	   printf("The event number %d, with trans is %d \n", dem, e->trans->proc.id);
	   dem ++;
	   c.add(*e);
	   c.en.pop_back();
   }
   printf("no more enabled");

}

} // end of namespace

