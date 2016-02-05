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

void Event::update(Config & c)
{
  ir::Trans & t   = this->getTrans();
  ir::Process & p = this->getProc();
  std::vector<Process> & procs = c.gstate->getSProcs();
  pre_proc = c.latest_proc[p.id];//e's parent is the latest event of the process
  post_proc.clear(); // e has no child

  switch (t.type)
  {
     case ir::Trans::RD:
        pre_mem  = c.latest_global_rdwr[p.id][t.addr];
        post_rdsyn[t.addr] = __null;
      	break;

     case ir::Trans::WR:
        pre_mem  = c.latest_global_wr[t.addr];
        for (auto & it: post_mem)
           for (auto & i: it)
              i = nullptr;
  	    // set pre-readers = set of latest events which use the variable copies of all processes.
  	    //size of pre-readers is numbers of copies of the variable = number of processes
      	for(auto i: procs)
            pre_readers[i.id] = c.latest_global_rdwr[i.id][t.addr]; // the latest global event of the process

        break;
     case ir::Trans::SYN:
   	    break;
     case ir::Trans::LOC:
   	    break;
  }
}

void Event::update_parent()
{
   Event & prt; //parent
   if (this->trans->type == ir::Trans::LOC)
      prt = *(this->pre_proc);
   else
	  prt = *(this->pre_mem);

   switch (prt.trans->type)
   case ir::Trans::WR:
      prt.post_proc[this->trans->proc.id] = this;
	  prt.post_wr[this->trans->addr].push_back(this);
	  break;
   case ir::Trans::LOC:
	  prt.post_proc = *this;
      break;
   case ir::Trans::RD:
      break;
   case ir::Trans::SYN:
	  break;


      prt.post_mem[this->trans->proc.id].push_back(this);
      if (this->trans->type == ir::Trans::WR)
      {

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

# if 0
void Event::compute_cfl(Event & e)
{
   Event & parent = *(e.pre_mem);
   ir::Trans & trans = parent.getTrans();
   switch (trans.type)
   {
      case ir::Trans::RD:
    	  // a read can access more than one global variable, for example l=x+y
    	  // divide into 2 different transition: l=x; l=l+y;
         for (auto const& it : parent.post_rdsyn)
        	 if ( find(parent.post_rdsyn.begin(), parent.post_rdsyn.end(), &e) != parent.post_rdsyn.end() )
        	    e.cfl.push_back(it);
         break;
      case ir::Trans::WR: // only access one variable
         for (auto const& i : parent.post_mem)
            if (find(i.begin(), i.end(), &e) != i.end())
               for (auto it : i)
                  if (i != &e)
                     e.cfl.push_back(it);
         break;
      case ir::Trans::SYN:
    	 for (auto const& it : parent.post_rdsyn)
    	   if (it != &e)
    	      e.cfl.push_back(it);
    	 break;
      case ir::Trans::LOC:
    	 Event & parent_loc = *(e.pre_proc);
    	 for (auto const& it : parent_loc.post_proc)
    	    if (it != &e)
    	       e.cfl.push_back(it);
    	 break;
   }
}
#endif

bool Event::check_cfl(Event & e)
{
   Event & parent = *(e.pre_mem);
   ir::Trans & trans = parent.getTrans();
  // std::vector<Event *>::iterator it;

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
	*gstate = u.m.init_state;
	__update_encex();
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
   ir::State & gs               = *gstate;
   ir::Trans & tran             = e.getTrans();
   ir::Process & p              = e.getProc();
   std::vector<Process> & procs = unf.m.getProcs();

   e.update(*this); //update the event
   e.update_parent(*this); // update parent of the event

   /*
    * update the configuration
    */

   gstate = tran.fire(gs); //update new global states

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
         break;
   }

   __update_encex(e);


}
#if 0
void Config::compute_en()
{
   ir::State & gs = *gstate;
   std::vector<ir::Trans> & trans = *(gs.m.getTrans());
   std::vector <ir::Process> & procs = *(gs.m.getProcs());
   assert(trans.size()==0);
   assert(procs.size()==0);

   for (auto & i: trans)
   {
      if (i.enabled(gs))
      {
         e.update(*this);
		 en.push_back(e);
      }
   }
}
#endif

void Config::__update_encex ()
{
   Event e;
   ir::State & gs = *gstate;
   std::vector<ir::Trans> & trans = gs.m.getTrans(); // all trans of the machine
   std::vector <ir::Process> & procs = gs.m.getProcs(); // all procs of the machine
   assert(trans.size()==0);
   assert(procs.size()==0);

   for (auto & t: trans)
     if (t.enabled(gs))
     {
        e = new Event(&t);
        unf.evt.push_back(e);
        for (auto ep: en)
           if (e.check_cfl(*ep) == true)
              cex.push_back(&e);
           else
              en.push_back(&e);
     }
}


/*
 * Methods for class Unfolding
 */
Unfolding::Unfolding (ir::Machine & ma): m(ma)
{
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
	// create an empty configuration conf;

	// in a loop:
	//choose randomly one event e enabled at conf;
	// conf.add (e); --> trigger construction of enabled events at the new
	//                   configuration, and addition of them to this->evt
	// reapeat until there is no event enabled

   printf ("Unfolding.explore_rnd_config\n");
   assert (evt.size () == 0);
   Config c(*this);
   Event * e;
   while (c.en.empty() == false)
   {
	   e = c.en.pop_back();
	   c.add(*e);
   }
}

} // end of namespace

