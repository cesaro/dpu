/*
 * pes.cc
 *
 *  Created on: Jan 12, 2016
 *      Author: tnguyen
 */

#include <cstdint>
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
  ir::Trans & s   = this->getTrans();
  ir::Process & p = this->getProc();
  int numproc     = c.gstate->getNumProcs();
  pre_proc = c.latest_proc[p.id];//e's parent is the latest event of the process
  post_proc.clear(); // e has no child

  switch (s.type)
  {
     case ir::Trans::RD:
        pre_mem  = c.latest_global_rdwr[p.id][s.addr];
      	post_mem[val] = this;
      	break;

     case ir::Trans::WR:
        pre_mem  = c.latest_global_wr[s.addr];
        for (auto & it: post_mem)
           it = *val;
  	    // set pre-readers = set of latest events which use the variable copies of all processes.
  	    //size of pre-readers is numbers of copies of the variable
      	 for (auto & it : pre_readers)
      	    for(int i = 0; i < numproc; i++)
     		   it = c.latest_global_rdwr[i][s.addr]; // the latest global event of the process
        break;
     case ir::Trans::SYN:
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

   for (auto& it:post_rdsyn)
   for (auto& i :e.post_rdsyn)
      it = i;

   val = e.val;

   for (auto& it:localvals)
   for (auto& i :e.localvals)
      it = i;

   *trans = *(e.trans);

   return *this;
}

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
            if (it != e) // need to overlap ==, != operator in Event
        	   e.cfl.push_back(it);
         break;
      case ir::Trans::WR: // only access one variable
         for (auto const& i : parent.post_mem)
            if (find(i.begin(), i.end(), e) != i.end())
               for (auto it : i)
                  if (i != e)
                     e.cfl.push_back(it);
         break;
      case ir::Trans::SYN:
    	 for (auto const& it : parent.post_rdsyn)
    	   if (it != e)
    	      e.cfl.push_back(it);
    	 break;
      case ir::Trans::LOC:
    	 Event & parent_loc = e.pre_proc;
    	 for (auto const& it : parent_loc.post_proc)
    	    if (it != e)
    	       e.cfl.push_back(it);
    	 break;
   }
}

bool Event::check_cfl(Event & e)
{
   Event & parent = *(e.pre_mem);
   ir::Trans & trans = parent.getTrans();
   switch (trans.type)
   {
      case ir::Trans::RD:
	  // a read can access more than one global variable, for example l=x+y
	  // divide into 2 different transition: l=x; l=l+y;
	     if (std::find(std::begin(parent.post_rdsyn), std::end(parent.post_rdsyn), e) != std::end(parent.post_rdsyn))
	    	 return true;
    	 break;

	  case ir::Trans::WR: // only access one variable
	     for (auto const& i : parent.post_mem)
	        if (find(i.begin(), i.end(), e) != i.end())
	           return true;
	     break;

	  case ir::Trans::SYN:
		  if (std::find(std::begin(parent.post_rdsyn), std::end(parent.post_rdsyn), e) != std::end(parent.post_rdsyn))
		     return true;
		  break;

	  case ir::Trans::LOC:
	     Event & parent_loc = e.pre_proc;
	     if (std::find(std::begin(parent.post_proc), std::end(parent.post_proc), e) != std::end(parent.post_proc))
	     	return true;
	     break;

	  default: return false;
   }
   return false;
}

/*
 * Methods of class Config
 */
Config:: Config()
{
	//gstate = NULL;
}

Config:: Config(Config & c)
: latest_global_wr (c.latest_global_wr)
, latest_proc (c.latest_proc)
, latest_global_rdwr (c.latest_global_rdwr)
, latest_local_wr (c.latest_local_wr)
{
   gstate = c.gstate;
}

/*
 * Add an event to a configuration
 */

void Config::add(Event & e)
{
   ir::State & gs               = *gstate;
   std::vector<Process> & procs = gs.getSProcs();
   int numofproc                = procs.size();
   ir::Trans & s                = e.getTrans();
   ir::Process & p              = e.getProc();
   /*
    * update the configuration
    */

   latest_proc[s.proc] = &e; //update latest event of the process
   gstate = s.fire(gs); //update new global states
   latest_global_wr  = e;

   //update local variables in trans
   for (auto i = s.localaddr.begin(); i != s.localaddr.end(); i++)
   latest_local_wr[*i] = &e;

   //update particular attributes according to type of transition.
   switch (s.type)
   {
      case ir::Trans::RD:
         latest_global_rdwr[s.proc][s.addr] = e;
         break;

      case ir::Trans::WR:
    	 latest_global_wr[s.addr] = e;
    	 for (int i = 0; i < numofproc; i++)
       	    latest_global_rdwr[i][s.addr] = e;

         break;

      case ir::Trans::SYN:
       	 latest_global_wr[s.addr]=&e;
       	 break;
   }

}
void Config::compute_en()
{
	ir::State & gs = *gstate;
	   std::vector<ir::Trans> & trans = gs.m.getTrans();
	   std::vector <ir::Process> & procs = gs.m.getProcs();
	   for (auto const& it : procs)
	   {
	      for (auto & i: trans)
		    {
			   if ( i.src == gs[it.id] )
			   {
				   Event e(&i);
	               e.update(*this);
				  // evt.push_back(e);
				   //Event * p = &evt.back();
				   en.push_back(&e);
	               //en.emplace_back(i);
				   //U.push_back(&e);
			    }
		     }
	   }
}

/*
 * Methods for class Unfolding
 */

/*
 * function to compute a set of events enabled at a configuration c
 */
void Unfolding::compute_en(Config & c)
{
   ir::State & gs = *(c.gstate);
   std::vector<ir::Trans> & trans = gs.m.getTrans();
   std::vector <ir::Process> & procs = gs.m.getProcs();
   for (auto const& it : procs)
   {
      for (auto & i : trans)
	  {
		  if ( i.src == gs[it.id] )
		  {
		     Event e(&i);
             e.update(c);
			 evt.push_back(e);
			 Event * p = &evt.back();
			 c.en.push_back(p);
			 U.push_back(p);
		  }
	  }
   }
}

/*
 * function to compute a set of conflicting extension to a configuration
 */

void Unfolding::compute_cex(Config & c)
{

}

/*
 * Compute set of events in conflict with event e
 */

void Unfolding:: explore(Config & C, std::vector<Event *> D, std::vector<Event *> A)
{
   Event * e;
   compute_en(C);
   compute_cex(C);
   if (C.en.empty() == true) return ;
   if (A.empty() == true)
       e = C.en.back();
   else
   {
      for (auto & it : A)
         if (std::find(C.en.begin(), C.en.end(), *it) != C.en.end()) //found an element of A in C
         {
            e = it;
            A.erase(&it);
         }
   }
   C.add(*e);
   explore (C, D, A);
}

void Unfolding::explore_rnd_config ()
{
   assert (evt.size () == 0);
   Config c;
   Event e;
   c.compute_en();
   while (c.en.empty() == false)
   {
	   e = *(c.en.begin());
	   c.add(e);
	   c.compute_en();
   }

   // create an empty configuration conf;

   // in a loop:
      // choose randomly one event e enabled at conf;
      // conf.add (e); --> trigger construction of enabled events at the new
      //                   configuration, and addition of them to this->evt
      // reapeat until there is no event enabled

}

} // end of namespace

