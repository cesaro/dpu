/*
f * pes.cc
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
//#include <functional>
//#include <boost/functional/hash.hpp>



#include "pes.hh"
#include "misc.hh"
#include "verbosity.h"

using std::vector;
using std::fstream;
using std::string;
using namespace ir;
using std::size_t;
using std::hash;
/*
using boost::hash_range;
using boost::hash_value;
using boost::hash_combine;
*/
namespace pes{

/*
 * Methods for class Node
 */
//-----------------------
template <class T, int SS >
Node<T,SS>::Node()
{
   depth = 0;
   pre   = nullptr;
   skip_preds = nullptr;
}

//-----------------------
template <class T, int SS >
Node<T,SS>::Node(int idx, Event * pr)
{
   pre      = pr;
   depth    = pre->nodep[idx].depth + 1;
   // initialize skip_preds here
   set_skip_preds(idx);
}
//-----------------------
template <class T, int SS >
void Node<T,SS>:: set_skip_preds(int idx)
{
   // including immediate predecessor
   int size = compute_size();
   //printf(", size: %d", size);

   // initialize the elements
   if (size == 0) return;

   assert(size > 0);

   /* mallocate the skip_preds */
   skip_preds = (Event**) malloc(sizeof(Event*) * size);

   // the first element skip_preds[0]
   T * p = pre;
   int k = 1;

   /* go back k times by pre*/
   while ((k < SS) and (p->node[idx].depth > 0))
   {
      p = p->node[idx].pre;
      k++;
   }

   skip_preds[0] = p;

   /* initialize the rest */
   if (size > 1)
   {
      for (unsigned i = 1; i < size; i++)
      {
         p = skip_preds[i - 1];
         int k = 1;

         /* go back k times by pre*/
         while ((k < SS) and (p->node[idx].depth > 0))
         {
            p = p->node[idx].skip_preds[i -1];
            k++;
         }
         skip_preds[i] = p;
      }
   }
   print_skip_preds();
}
//-----------------------
template <class T, int SS >
void Node<T,SS>:: set_up(int idx, Event * pr)
{
   pre      = pr;
   depth    = pre->node[idx].depth + 1;
   // initialize skip_preds here
   set_skip_preds(idx);
}
//---------------------
template <class T, int SS >
int Node<T,SS>::compute_size()
{
   if (depth == 0) return 0;
   int temp = 1;
   int skip = SS;
   while (depth % skip == 0)
   {
      skip = skip * SS;
      temp++;
   }
   return --temp;
}
//-------------------
template <class T, int SS >
void Node<T,SS>:: print_skip_preds()
{
   printf("Node: %p", this);
   printf(", depth: %d", depth);
   printf(", Pre: %p", pre);
   int size = compute_size();
   if (size == 0)
   {
      printf(", No skip predecessor\n");
      return;
   }
   printf(", Skip_preds: ");
   for (unsigned i = 0; i < size; i++)
   {
      if (i == size-1)
         printf ("%p", skip_preds[i]);
      else
         printf ("%p, ", skip_preds[i]);
   }

   printf("\n");
}
//----------
int max_skip(int d, int base)
{
   int i = 0;
   int pow = 1;
   while (d % pow == 0)
   {
      pow = pow * base;
      i++;
   }
   return i-1;
}
//----------
/*
 * make sure that d < this->depth to use this method
 */
template <class T, int SS >
template <int idx>
T & Node<T,SS>:: find_pred(int d) const
{
   //printf("\n This is function to find a node at a specific depth");
   T * next = nullptr;;
   int i, dis = this->depth - d;
   assert (dis != 0); // at the beginning dis != 0

   // initial next for the very first time
   DEBUG("dis = %d", dis);
   i = max_skip(dis,SS);
   DEBUG("i=%d", i);
   if (i == 0)
      next = pre;
   else
      next = skip_preds[i-1];
   dis = next->node[idx].depth - d;

// for the second loop and so on
   while (dis != 0)
   {
      i = max_skip(dis,SS);
      DEBUG("i=%d", i);
      if (i == 0)
         next = next->node[idx].pre;
      else
         next = next->node[idx].skip_preds[i-1];

      dis = next->node[idx].depth - d;
   }
   DEBUG("next = %d", next->idx);
   return *next;
}
//-----------

/*
 * Methods for class MultiNode
 */
template <class T, int S, int SS> // S: number of trees, SS: skip step
MultiNode<T,S,SS> :: MultiNode(T * pp, T * pm)
{
   node[0].set_up(0,pp);
   node[1].set_up(1,pm);
}
//---------------------
Ident:: Ident()
: trans (nullptr)
, pre_proc (nullptr)
, pre_mem (nullptr)
{
}
//----------------------
/*
Ident::Ident(const Trans * t, Event * ep, Event * em)
: trans(t)
, pre_proc(ep)
, pre_mem(em)
{
}
*/
//---------------------
Ident::Ident(const Trans * t, Event * ep, Event * em, std::vector<Event *> pr)
: trans(t)
, pre_proc(ep)
, pre_mem(em)
, pre_readers(pr)
{
}

/*
 * create an identity for events which are labelled by t and enabled at the configuration c
 */
Ident::Ident(const ir::Trans & t, const Config & c)
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

  // if (this->is_bottom()) return;

   trans = &t;
   //DEBUG("Trans: %s", trans->str().c_str());
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

  // DEBUG("   Make history: %s ",this->str().c_str());
}
//------------------
Ident::Ident(const Ident & id)
{
   trans = id.trans;
   pre_proc = id.pre_proc;
   pre_mem = id.pre_mem;
   pre_readers = id.pre_readers;
}
//------------------
std::string Ident::str () const
{
   std::string st;

      if (pre_readers.empty())
         st = "None\n";
      else
      {
         for (unsigned int i = 0; i < pre_readers.size(); i++)
            if (i == pre_readers.size() -1)
               st += std::to_string(pre_readers[i]->idx);
            else
               st += std::to_string(pre_readers[i]->idx) + ", ";
         st +="\n";
      }

      const char * code = trans ? trans->code.str().c_str() : "";
      int proc = trans ? trans->proc.id : -1;

      if (pre_mem != nullptr)
         return fmt ("trans %p, code '%s', proc %d, pre_proc %p, pre_mem %p, pre_readers: %s ",
            trans, code, proc, pre_proc, pre_mem, st.c_str());
      else
        return fmt ("trans %p, code '%s', proc %d, pre_proc %p, pre_mem(null) %p, pre_readers: %s",
            trans, code, proc, pre_proc, pre_mem, st.c_str());

}

/* Overlap == operator for Ident */
bool Ident:: operator == (const Ident & id) const
{
   return ((trans == id.trans) and (pre_proc == id.pre_proc)
         and (pre_mem == id.pre_mem) and (pre_readers == id.pre_readers));
}

/*
 * Methods for class Event
 */
//-------------------
Event:: Event (Unfolding & u, Ident & ident)
:  MultiNode()
,  idx(u.count)
,  evtid(ident)
,  val(0)
,  localvals(0)
,  color(0)
{
   assert(this->is_bottom() == false);
   unsigned numprocs = u.m.procs.size();
   if (evtid.trans->type == ir::Trans::WR)
   {
      std::vector<Event *> temp;
      for (unsigned i = 0; i < numprocs; i++)
         post_mem.push_back(temp);
   }

   /* initialize vector clock and vector of maximal events for all processes */
   for (unsigned i = 0; i < numprocs; i++)
      clock.push_back(0);

   /* initialize vector of maximal events for all variables*/
   for (unsigned i = 0; i < u.m.memsize; i++)
      var_maxevt.push_back(nullptr);

   DEBUG ("  %p: Event.ctor: t %p: '%s'", this, evtid.trans, evtid.trans->str().c_str());
   ASSERT (evtid.pre_proc != NULL);

   // initialize the corresponding nodes, after setting up the history
   node[0].set_up(0, evtid.pre_proc);
   if (evtid.trans->type != ir::Trans::LOC)
   {
      node[1].set_up(1, evtid.pre_mem);
   }
}

/* For creating bottom event */
Event::Event (Unfolding & u)
   : MultiNode()
   , idx(u.count)
   , val(0)
   , localvals(0)
   , color(0)

{
   // this is for bottom event only
   unsigned numprocs = u.m.procs.size();

   // post_mem is a vector of vectors, so it needs initialized
   std::vector<Event *> temp;
   for (unsigned i = 0; i < numprocs; i++)
      post_mem.push_back(temp);
   DEBUG ("  %p: Event.ctor:", this);
}

bool Event::is_bottom () const
{
   return this->evtid.pre_mem == this;
}

/*
 * set up its history, including 3 attributes: pre_proc, pre_mem and pre_readers
 */
#if 0
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

   ir::Process & p = this->evtid.trans->proc;
   std::vector<Process> & procs = c.unf.m.procs;
   int varaddr = evtid.trans->var - procs.size();

   /*
    * e's parent is the latest event of the process
    * for all events, initialize pre_proc of new event by the latest event of config (in its process)
    */
   evtid.pre_proc = c.latest_proc[p.id];

   switch (evtid.trans->type)
   {
      case ir::Trans::RD:
         evtid.pre_mem  = c.latest_op[p.id][varaddr];
         /* pre_readers stays empty for RD events */
         break;

      case ir::Trans::WR:
         evtid.pre_mem  = c.latest_wr[varaddr];
        /*
         * set pre-readers = set of latest events which use the variable copies of all processes
         * size of pre-readers is numbers of copies of the variable = number of processes
         */

         for (unsigned int i = 0; i < procs.size(); i++)
            evtid.pre_readers.push_back(c.latest_op[i][varaddr]);

         break;

      case ir::Trans::SYN:
         evtid.pre_mem  = c.latest_op[p.id][varaddr]; // pre_mem can be latest SYN from another process (same variable)
         break;

      case ir::Trans::LOC:
         // nothing to do
    	 /*
    	  * If a LOC transition RD or WR on a local variable,
    	  * its pre_mem is previous event touching that one.
    	  * If LOC is a EXIT, no variable touched, no pre_mem
    	  * How to solve: v4 = v3 + 1; a RD for v3 but a WR for v4 (local variable)
    	  */
         evtid.pre_mem   = nullptr;
         break;
   }

   DEBUG("   Make history: %s ",this->str().c_str());
}
#endif

void Event::set_vclock()
{
   if (this->is_bottom())
      return;

   Process & p = evtid.trans->proc;
   clock = evtid.pre_proc->clock;
   switch (evtid.trans->type)
     {
        case ir::Trans::LOC:
           clock[p.id]++;
           break;

        case ir::Trans::SYN:
           for (unsigned i = 0; i < clock.size(); i++)
              clock[i] = std::max(evtid.pre_mem->clock[i], clock[i]);

           clock[p.id]++;
           break;

        case ir::Trans::RD:
           for (unsigned i = 0; i < clock.size(); i++)
              clock[i] = std::max(evtid.pre_mem->clock[i], clock[i]);

           clock[p.id]++;
           break;

        case ir::Trans::WR:
           // find out the max elements among those of all pre_readers.
           for (unsigned i = 0; i < clock.size(); i++)
           {
              /* put all elements j of pre_readers to a vector temp */
              for (unsigned j = 0; j < evtid.pre_readers.size(); j++)
                clock[i] = std::max(evtid.pre_readers[j]->clock[i], clock[i]);
           }

           clock[p.id]++;
           break;
     }

}

/*
 * Set up the vector var_maxevt
 * - stores maximal events for all variables in an event's local configuration
 */
void Event::set_var_maxevt()
{
   if (this->is_bottom())
         return;

   var_maxevt = evtid.pre_proc->var_maxevt; // initialize maxevt by pre_proc.maxevt

   switch (evtid.trans->type)
   {
   case ir::Trans::LOC:
      return;

   case ir::Trans::SYN:
      for (unsigned i = 0; i < var_maxevt.size(); i++)
         if (evtid.pre_mem->var_maxevt[i]->succeed(*var_maxevt[i]))
            var_maxevt[i] = evtid.pre_mem->var_maxevt[i];
         // else do nothing
      break;

   case ir::Trans::RD:
      for (unsigned i = 0; i < var_maxevt.size(); i++)
         if (evtid.pre_mem->var_maxevt[i]->succeed(*var_maxevt[i]))
            var_maxevt[i] = evtid.pre_mem->var_maxevt[i];
      break;

   case ir::Trans::WR:
      // find out the max elements among those of all pre_readers.
      for (unsigned i = 0; i < var_maxevt.size(); i++)
      {
         for (unsigned j = 0; j < evtid.pre_readers.size(); j++)
            if ( evtid.pre_readers[j]->var_maxevt[i]->succeed(*var_maxevt[i]))
               var_maxevt[i] = evtid.pre_readers[j]->var_maxevt[i] ;
      }
      break;

   }

   // minus the number of processes to locate the position of variable
   var_maxevt[this->evtid.trans->var - this->clock.size()] = this; //???compute the number of variable
}

/*
 * Update all events precede current event, including pre_proc, pre_mem and all pre_readers
 */
void Event::update_parents()
{
   printf("   Update_parents:  ");
   if (is_bottom())
   {
      printf("   Its parent is bottom");
      return;
   }

   Process & p  = this->evtid.trans->proc;

   switch (this->evtid.trans->type)
   {
      case ir::Trans::WR:

         evtid.pre_proc->post_proc.push_back(this);
         /* update pre_mem */
         //pre_mem->post_wr.push_back(this); // need to consider its necessary -> No
         for (unsigned i = 0; i < evtid.pre_readers.size(); i++)
         {
            if (evtid.pre_readers[i]->is_bottom() || (evtid.pre_readers[i]->evtid.trans->type == ir::Trans::WR))
            {
               // add to vector of corresponding process in post_mem
               evtid.pre_readers[i]->post_mem[i].push_back(this);
            }
            else
            {
               evtid.pre_readers[i]->post_rws.push_back(this);
            }
         }

         break;

      case ir::Trans::RD:
         evtid.pre_proc->post_proc.push_back(this);
         /* update pre_mem */
         if ( (evtid.pre_mem->is_bottom() == true) || (evtid.pre_mem->evtid.trans->type == ir::Trans::WR)   )
            evtid.pre_mem->post_mem[p.id].push_back(this);
         else
            evtid.pre_mem->post_rws.push_back(this);
         /* no pre_readers -> nothing to do with pre_readers */

         break;

      case ir::Trans::SYN:
         evtid.pre_proc->post_proc.push_back(this);
         /* update pre_mem */
         if ( (evtid.pre_mem->is_bottom() == true) || (evtid.pre_mem->evtid.trans->type == ir::Trans::WR)   )
            evtid.pre_mem->post_mem[p.id].push_back(this);
         else
            evtid.pre_mem->post_rws.push_back(this);
         /* no pre_readers -> nothing to do with pre_readers */
         break;

      case ir::Trans::LOC:
         evtid.pre_proc->post_proc.push_back(this);
         /* update pre_mem */
         /* no pre_readers -> nothing to do with pre_readers */
         break;
   }

   printf(" Finished\n");
   return ;
}
/*
 * Check if e is in this event's history or not
 * If this and e are in the same process, check their source
 * If not, check pre_mem chain of this event to see whether e is inside or not (e can only be a RD, SYN or WR)
 */
bool Event:: succeed(const Event & e ) const
{
   if (this->is_bottom())  return false;

   if (e.is_bottom()) return true;

   if (this->evtid.trans->proc.id == e.evtid.trans->proc.id)
   {
      if (e.clock[evtid.trans->proc.id] < this->clock[evtid.trans->proc.id])
         return true;
      else
         return false;
   }

   if (e.clock < clock)
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
   idx         = e.idx;
   evtid       = e.evtid;
   val         = e.val;
   post_mem    = e.post_mem;
   post_proc   = e.post_proc;
   post_wr     = e.post_wr;
   post_rws    = e.post_rws;
   localvals   = e.localvals;
   color       = e.color;
   dicfl       = e.dicfl;
   clock       = e.clock;
   var_maxevt  = e.var_maxevt;
   return *this;
}
/*
 * Find the WR event which is the immediate predecessor of a RD
 */
const Event & Event:: find_latest_WR_pred() const
{
   assert(this->evtid.trans->type == ir::Trans::RD);
   const Event * e = this;
   while (1)
   {
      if ((e->is_bottom() or (e->evtid.trans->type == ir::Trans::WR)))
         return *e;
      e = e->evtid.pre_mem;
   }
}

/*
 * Check if two events are in immediate conflict:
 *    - Two events are in direct conflict if they both appear in a vector in post_mem of an event
 */

bool Event::check_dicfl( const Event & e )
{
   if (this->is_bottom() || e.is_bottom() || (*this == e) )
      return false;

   /*
    * Check pre_proc: if they have the same pre_proc and in the same process -> conflict
    * It is also true for the case where pre_proc is bottom
    */

   if ((this->evtid.pre_proc == e.evtid.pre_proc) && (this->evtid.trans->proc.id == e.evtid.trans->proc.id))
      return true;

   /*
    *  a LOC event has no conflict with any other transition.
    * "2 LOC trans sharing a localvar" doesn't matter because it depends on the PC of process.
    * Here, it means they don't have same pre_proc --> any LOC is in no conflict with others.
    */
   if ((this->evtid.trans->type == ir::Trans::LOC)  || (e.evtid.trans->type == ir::Trans::LOC))
      return false;

   // different pre_procs => check pre_mem or pre_readers
   Event * parent = evtid.pre_mem; // for RD, SYN, WR events
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

   switch (parent->evtid.trans->type)
   {
      case ir::Trans::RD:
         // post_rws is a vector of all operations succeeding
         //this_idx = std::find(parent->post_rws.begin(), parent->post_rws.end(),this); // don't need to find, because it is certainly there
         e_idx  = std::find(parent->post_rws.begin(), parent->post_rws.end(),&e);
    	  // if ( (this_idx != parent->post_rws.end()) && (e_idx != parent->post_rws.end()) )
         if (e_idx != parent->post_rws.end())
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
        // post_rws is a vector of all operations succeeding
       // this_idx = std::find(parent->post_rws.begin(), parent->post_rws.end(),this); // don't need to find, because it is certainly there
    	  e_idx    = std::find(parent->post_rws.begin(), parent->post_rws.end(),&e);
    	 // if ( (this_idx != parent->post_rws.end()) && (e_idx != parent->post_rws.end()) )
    	 if (e_idx != parent->post_rws.end())
           return true;
    	  break;

     case ir::Trans::LOC:
        // nothing to do
        break;
   }

   return false;
}
/*
 * Check if two events (this and e) are in conflict
 *
 */
bool Event::check_cfl(const Event & e )
{
   if (this->is_bottom() or e.is_bottom())
      return false;

   if (this == &e) // same event
      return false;

   if (evtid.trans->proc.id == e.evtid.trans->proc.id)
   {
      DEBUG("They are in the same process");
      DEBUG("check cfl same tree returns %d", this->check_cfl_same_tree<0>(e));
      return this->check_cfl_same_tree<0>(e);
   }

   if ((this->evtid.trans->type == ir::Trans::WR) and (e.evtid.trans->type == ir::Trans::WR))
   {
      DEBUG("They are 2 WRs");
      return this->check_cfl_same_tree<1>(e);
   }

   if ((this->evtid.trans->type == ir::Trans::SYN) and (e.evtid.trans->type == ir::Trans::SYN))
   {
      DEBUG("They are 2 SYNs");
      return this->check_cfl_same_tree<1>(e);
   }

   if ((this->evtid.trans->type == ir::Trans::RD) and (e.evtid.trans->type == ir::Trans::RD))
   {
      DEBUG("They are 2 RDs");
      return this->check_cfl_2RD(e);
   }

   if ((this->evtid.trans->type == ir::Trans::WR) and (e.evtid.trans->type == ir::Trans::RD))
   {
      DEBUG("They are a WR and a RD");
      return this->check_cfl_WRD(e);
   }

   if ((this->evtid.trans->type == ir::Trans::RD) and (e.evtid.trans->type == ir::Trans::WR))
   {
      DEBUG("They are a RD and a WR");
      return e.check_cfl_WRD(*this);
   }

   // other cases
   DEBUG("They touch different variables or 2 LOCs");
  return check_2LOCs(e);
}

//------same tree ---------
template <int id>
bool Event:: check_cfl_same_tree (const Event & e) const
{
   int d1, d2;
   d1 = this->node[id].depth;
   d2 = e.node[id].depth;

   DEBUG("d1 = %d, d2 = %d", d1, d2);
   //one of event is the bottom
   if ((d1 == 0) or (d2 == 0))
   {
      DEBUG("1 trong 2 cai la bottom, %d and %d not cfl", this->idx, e.idx);
      return false;
   }

   if (d1 == d2)
      return not(this == &e); // same event -> not conflict and reverse

   if (d1 > d2)
   {
      DEBUG("Here d1 > d2");
      Event & temp = this->node[id].find_pred<id>(d2);
      DEBUG("temp=%d", temp.idx);
      if (this == &temp) // refer to the same event
      {
         DEBUG("same event");
         return false;
      }
      else
      {
         DEBUG("not same event");
         return true;
      }
   }
   else
   {
      DEBUG("Here d2 > d1");
      Event & temp = e.node[id].find_pred<id>(d1);
      if (this == &temp) // refer to the same event
      {
         DEBUG("same event");
         return false;
      }
      else
      {
         DEBUG("not same event");
         return true;
      }

   }
}

/*
 * Check WR and RD where this is a WR and e is a RD
 */
bool Event:: check_cfl_WRD(const Event & e) const
{
   assert(this->evtid.trans->type == ir::Trans::WR);
   assert(e.evtid.trans->type == ir::Trans::RD);

   Event * post_wr;
   const Event * pre_wr = &e.find_latest_WR_pred();
   DEBUG("pre_wr.idx: %d", pre_wr->idx);

   if (this == pre_wr) // point to the same event
      return false;

   if (this->check_cfl_same_tree<1>(*pre_wr))
   {
      DEBUG("this and pre_wr are in cfl");
      return true;
   }
   else // else - not conflict in the same tree
   {
      if (pre_wr->succeed(*this)) // this < pre_wr (e < pre_wr < e') => no conflict
      {
         DEBUG("bla blathis < pre_wr");
         return false;
      }
      else
      {
         DEBUG(" hu hu pre_wr < this");
         if (this->node[1].depth == pre_wr->node[1].depth + 1) // this and e are in post_mem of pre_wr
            return true;

         post_wr = &node[1].find_pred<1>(pre_wr->node[1].depth + 1);

         DEBUG("post_wr.idx: %d", post_wr->idx);

         if (post_wr->evtid.pre_readers[e.evtid.trans->proc.id]->succeed(e))
            return false; //in causality
         else
            return true;
      }
   }
}
/*
 * Check conflict between 2 RDs in different processes
 */
bool Event:: check_cfl_2RD(const Event & e) const
{
   const Event * pre_wr1, * pre_wr2;
   Event * post_wr;
   pre_wr1 = &find_latest_WR_pred();
   pre_wr2 = &e.find_latest_WR_pred();

   if (pre_wr1 == pre_wr2)
      return false; // they are concurrent

   if (pre_wr1->check_cfl_same_tree<1>(*pre_wr2))
   {
         DEBUG("pre_wr1 and pre_wr2 are in cfl");
         return true;
   }

   if (pre_wr1->succeed(*pre_wr2)) // this < pre_wr (e < pre_wr < e') => no conflict
   {
      DEBUG(" hu hu pre_wr2 < pre_wr1");
      if (pre_wr1->node[1].depth == pre_wr2->node[1].depth + 1) // this and e are in post_mem of pre_wr
         return true;

      post_wr = &pre_wr1->node[1].find_pred<1>(pre_wr2->node[1].depth + 1);

      DEBUG("post_wr.idx: %d", post_wr->idx);

      if (post_wr->evtid.pre_readers[e.evtid.trans->proc.id]->succeed(e))
         return false; //in causality
      else
         return true;         }
   else //pre_wr1 < pre_wr2
   {
      DEBUG(" pre_wr1 < pre_wr2");
      if (pre_wr2->node[1].depth == pre_wr1->node[1].depth + 1) // this and e are in post_mem of pre_wr
         return true;

      post_wr = &pre_wr2->node[1].find_pred<1>(pre_wr1->node[1].depth + 1);

      DEBUG("post_wr.idx: %d", post_wr->idx);

      if (post_wr->evtid.pre_readers[this->evtid.trans->proc.id]->succeed(*this))
         return false; //in causality
      else
         return true;
   }
}

/*
 *  check conflict between two events touching different variables or 2 LOCs.
 */
bool Event:: check_2LOCs(const Event & e)
{
   DEBUG("Start checking 2 LOCs");
   for (unsigned i = 0; i < var_maxevt.size(); i++)
   {
      // if there exists a pair of events in conflict, then this \cfl e
      DEBUG("Pair %d: %d and %d", i, var_maxevt[i]->idx,e.var_maxevt[i]->idx );

      if ( var_maxevt[i]->check_cfl (*e.var_maxevt[i]) )
      {
         DEBUG("They are in cfl");
         return true;
      }
   }

   DEBUG("They are not in cfl");
   return false; // they are not in conflict
}

/* check if 2 events are the same or not */
bool Event:: is_same(const Event & e) const
{
   if (this->is_bottom() or e.is_bottom()) return false;

   if ( e.evtid == evtid)
      return true;

   return false;
}

/* Express an event in a string */
std::string Event::str () const
{
   return fmt ("%p, index: %d,  evtid: %s",
                this, idx,  evtid.str().c_str());
}

/* represent event's information for dot print */
std::string Event::dotstr () const
{
   std::string st, st1, st2;
   for (unsigned int i = 0; i < clock.size(); i++)
      if (i == clock.size() -1)
         st += std::to_string(clock[i]);
      else
         st += std::to_string(clock[i]) + ", ";

   for (unsigned int i = 0; i < var_maxevt.size(); i++)
      if (i == var_maxevt.size() -1)
         st2 += std::to_string(var_maxevt[i]->idx);
      else
         st2 += std::to_string(var_maxevt[i]->idx) + ", ";

   const char * code = (evtid.trans != nullptr) ? evtid.trans->code.str().c_str() : ""; // pay attention at c_str()
   int proc = (evtid.trans != nullptr) ? evtid.trans->proc.id : -1;
   int src = evtid.trans? evtid.trans->src : 0;
   int dst = evtid.trans? evtid.trans->dst : 0;

   return fmt ("id: %d, code: '%s' \n proc: %d , src: %d , dest: %d \n clock:(%s) \n var_max: (%s) "
            , idx, code, proc, src, dst, st.c_str(), st2.c_str());
}

/* Print all information of an event */
void Event::eprint_debug()
{

   printf ("Event: %p, id: %d, Ident: %s", this, this->idx, this->evtid.str().c_str());

	//print post_mem
	if (post_mem.size() != 0)
		{
			DEBUG("Post_mem: ");
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
   // print corresponding in the process tree
   printf("Proc node: ");
   print_proc_skip_preds();
   printf("Var node:");
   print_var_skip_preds();

   //---------------------
   printf(" Var_maxevt: ");
   for (unsigned i = 0; i < var_maxevt.size(); i++)
      printf("%d, ", var_maxevt[i]->idx);
   printf("\n");
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

Config:: Config (const Config & c)
   :  gstate(c.gstate)
   ,  unf(c.unf)
   ,  latest_proc (c.latest_proc)
   ,  latest_wr (c.latest_wr)
   ,  latest_op (c.latest_op)
{
}

/*
 * Add an event fixed by developer to a configuration
 */

void Config::add_any ()
{
   // the last event in enable set
   add (en.size () - 1);
}

/*
 * add an event identified by its value
 */

void Config::add (const Event & e)
{
   DEBUG ("\n\n%p: Config.addevent : %d\n", this, e.idx);

   for (unsigned int i = 0; i < en.size (); i++)
      if (e == *en[i])
         {
           // printf("Enable event idx = %d", i);
            add (i);
         }
  // throw std::range_error ("Trying to add an event not in enable set by a configuration");

}

/*
 * add an event identified by its index in enable set
 */
void Config::add (unsigned idx)
{
   assert(idx < en.size());
   Event & e = *en[idx];

   DEBUG ("\n%p: Config.addidx: %s\n", this, e.str().c_str());

   /* move the element en[idx] out of enable set */
   en[idx] = en.back();
   en.pop_back();

   ir::Process & p              = e.evtid.trans->proc; // process of the transition of the event
   std::vector<Process> & procs = unf.m.procs; // all processes in the machine
   int varaddr = e.evtid.trans->var - procs.size();

   // update the configuration
   e.evtid.trans->fire (gstate); //move to next state

   latest_proc[p.id] = &e; //update latest event of the process containing e.trans

   //update other attributes according to the type of transition.
   switch (e.evtid.trans->type)
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
      if (e.evtid.trans->localvars.empty() != true)
         for (auto & i: e.evtid.trans->localvars)
         {
            latest_op[p.id][i - procs.size()] = &e;
         }

   /* update en and cex set with e being added to c (before removing it from en) */
   __update_encex(e);
}

/*
 * Update enabled set whenever an event e is added to c
 */
void Config::__update_encex (Event & e )
{
   // Actually we don't use e any more!!!
   DEBUG ("%p: Config.__update_encex(%p)", this, &e);

   assert(unf.m.trans.size() > 0);
   assert(unf.m.procs.size() > 0);

   /* remove conflicting events with event which has just been added */
   if (en.size() > 0)
      remove_cfl(e);

  /* get set of events enabled at the state gstate    */
   std::vector<Trans*> enable;
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

   //update conflict extension
   compute_cex();
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
      if (e.check_dicfl(*en[i]) == true)
      {
         printf ("  %p ", en[i]);
         e.dicfl.push_back(en[i]); // add en[i] to direct conflicting set of e
         en[i]->dicfl.push_back(&e); // add e to direct conflict set of en[i]
         cex.push_back(en[i]);
         /* remove en[i] */
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
         switch (p->evtid.trans->type)
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
         p = p->evtid.pre_proc;
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
   ep = e->evtid.pre_proc;
   em = e->evtid.pre_mem;

   while (!(em->is_bottom()) and (ep->succeed(*em) == false))
   {
      if (em->evtid.trans->type == ir::Trans::RD)
      {
         pr_mem   = em;
         em       = em->evtid.pre_mem;
      }
      else
      {
         pr_mem   = em->evtid.pre_readers[e->evtid.trans->proc.id];
         em       = em->evtid.pre_readers[e->evtid.trans->proc.id];
      }

      /* Need to check the event's history before adding it to the unf
       * Don't add events which are already in the unf
       */

      Ident * id = new Ident(e->evtid.trans, ep, pr_mem, std::vector<Event *> ());
      Event * newevt = &unf.find_or_add (*id);
      add_to_cex(newevt);

      // add event to set of direct conflict dicfl if it is new one.
      if (newevt->idx == unf.evt.back().idx)
      {
         e->dicfl.push_back(newevt);
         newevt->dicfl.push_back(e);

      }
   } // end while
}

/*
 *  compute conflict events for a SYN event
 */
void Config:: SYN_cex(Event * e)
{
   DEBUG(" %p, id:%d: RD conflicting extension", e, e->idx);
   Event * ep, * em, *pr_mem; //pr_mem: pre_mem
  // const Trans & t = *e->evtid.trans;
   ep = e->evtid.pre_proc;
   em = e->evtid.pre_mem;

   while (!(em->is_bottom()) and (ep->succeed(*em) == false))
   {
      if (em->evtid.trans->type == ir::Trans::RD)
      {
         pr_mem   = em;
         em       = em->evtid.pre_mem;
      }
      else
      {
         pr_mem   = em->evtid.pre_readers[e->evtid.trans->proc.id];
         em       = em->evtid.pre_readers[e->evtid.trans->proc.id];
      }

      /*
       *  Need to check the event's history before adding it to the unf
       * Don't add events which are already in the unf
       */
      Ident id(e->evtid.trans, e->evtid.pre_proc, e->evtid.pre_mem, std::vector<Event *>());
      Event * newevt = &unf.find_or_add (id);

      // add event to set of direct conflict dicfl if it is new one.
      if (newevt->idx == unf.evt.back().idx)
      {
         e->dicfl.push_back(newevt);
         newevt->dicfl.push_back(e);
         add_to_cex(newevt);
      }
   }
}
/*
 * Compute conflicting event for a WR event e
 */
void Config:: WR_cex(Event * e)
{
   DEBUG(" %p: id:%d WR cex computing", e, e->idx);
   Event * ep, * ew, * temp;
   unsigned numprocs = unf.m.procs.size();
   std::vector<std::vector<Event *> > spikes(numprocs);
   spikes.resize(numprocs);

   ep = e->evtid.pre_proc;
   ew = e;
   int count = 0;

   while ((ew->is_bottom() == false) && (ep->succeed(*ew)) == false )
   {
      for (unsigned k = 0; k < numprocs; k++)
         spikes[k].clear(); // clear all spikes for storing new elements

      DEBUG("  Comb #%d (%d spikes): ", count++, numprocs);

      for (unsigned i = 0; i < ew->evtid.pre_readers.size(); i++)
      {
         temp = ew->evtid.pre_readers[i];
         while ((temp->is_bottom() == false) && (temp->evtid.trans->type != Trans::WR))
         {
            spikes[i].push_back(temp);
            temp = temp->evtid.pre_mem;
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
       * if ep is a successor of ew, then remove from the comb ew itself
       * and all RD events that are also in the history should think about bottom!!!
       */
      if (ep->succeed(*ew) == true)
      {
         for (int unsigned i = 0; i < spikes.size(); i++)
            while (ep->succeed(*spikes[i].back()) == true)
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
            Ident id(e->evtid.trans, e->evtid.pre_proc, e->evtid.pre_mem, combi);
            newevt = &unf.find_or_add(id);

            //only add new event in cex, if it is already in unf, don't add
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
         printf(" and already in the cex");
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
   fs << "label =\"A random configuration.\"";
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
            switch (p->evtid.trans->type)
            {
               case ir::Trans::LOC:
                  fs << p->idx << "[id="<< p->idx << ", label=\" " << p->dotstr() << " \", fillcolor=yellow];\n";
                  fs << p->evtid.pre_proc->idx << "->"<< p->idx << "[color=brown];\n";
                  break;
               case ir::Trans::RD:
                  fs << p->idx << "[id="<< p->idx << ", label=\" " << p->dotstr() << " \", fillcolor=palegreen];\n";
                  fs << p->evtid.pre_mem->idx << "->"<< p->idx << ";\n";
                  fs << p->evtid.pre_proc->idx << "->"<< p->idx << "[fillcolor=brown];\n";
                  break;
               case ir::Trans::SYN:
                  fs << p->idx << "[id="<< p->idx << ", label=\" " << p->dotstr() << " \", fillcolor=lightblue];\n";
                  fs << p->evtid.pre_mem->idx << "->"<< p->idx << ";\n";
                  fs << p->evtid.pre_proc->idx << "->"<< p->idx << "[color=brown];\n";
                  break;

               case ir::Trans::WR:
                  fs << p->idx << "[id="<< p->idx << ", label=\" " << p->dotstr() << " \", fillcolor=red];\n";
                  fs << p->evtid.pre_proc->idx << "->"<< p->idx << "[color=brown];\n";
                  for (auto pr : p->evtid.pre_readers)
                  {
                     fs << pr->idx << "->"<< p->idx << ";\n";
                  }
                  break;
            }
            p = p->evtid.pre_proc;
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
   printf ("%p Config.cex: size = %zu ", this, cex.size());
   if (cex.size() != 0)
   {
      printf(", elements: ");
      for (auto & e : cex)
         printf("%d ", e->idx);
   }
   else
      printf(", No element");
}

/*
 * Methods for EventID
 */
/*
EventID::EventID(ir::Trans * t, Event * pp,Event * pm, std::vector<Event*> pr)
: trans(t)
, pre_proc(pp)
, pre_mem(pm)
, pre_readers(pr)
{
}
*/
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
   DEBUG("Bottom is %p", bottom);
   __create_bottom ();

   DEBUG("Bottom is %p", bottom);
}

void Unfolding::__create_bottom ()
{
   assert (evt.size () == 0);
   /* create an "bottom" event with all empty */
   evt.emplace_back(*this); // need reviewing
   evt.back().evtid.pre_mem   = &evt.back();
   evt.back().evtid.pre_proc  = &evt.back();

   count++;
   bottom = &evt.back(); // using = operator

   //DEBUG("%s", bottom->evtid.str().c_str());

   /* add to the hash table evttab */
   //evttab.emplace(bottom->evtid, &evt.back());

   /*
    * set up vector clock and maxevent
    * After initialize the history Ident
    */

   for (unsigned i = 0; i < m.procs.size(); i++)
      bottom->clock.push_back(0);

   for (unsigned i = 0; i < m.memsize - m.procs.size(); i++)
      bottom->var_maxevt.push_back(bottom);

   DEBUG ("%p: Unfolding.__create_bottom: bottom %p", this, &evt.back());
   bottom->eprint_debug();
}
/*
 * create an event with enabled transition and config.
 */
void Unfolding::create_event(ir::Trans & t, Config & c)
{
   /*
    * Need to check the event's history before adding it to the unf
    * Only add events that are really enabled at the current state of config.
    * Don't add events already in the unf (enabled at the previous state)
    */

   Ident id(t,c);
   /* Check if new event exist in evt or not */
   std::unordered_map <Ident, Event *, IdHasher<Ident>>::const_iterator got = evttab.find (id);
   if ( got != evttab.end() )
      {
         DEBUG("   already in the unf as event with idx = %d", evttab[id]->idx);
         return;
      }

   evt.emplace_back(*this, id);
   printf("evt.size = %zu", evt.size());

   evt.back().update_parents();

   /* set up vector clock */
   evt.back().set_vclock();
   evt.back().set_var_maxevt();

   /* add to the hash table evttabl */
   evttab.emplace(evt.back().evtid, &evt.back());

   count++; // increase the number of objects in Event class
   c.en.push_back(&evt.back()); // do we need to check its existence???
   DEBUG("   Unf.evt.back:%s \n ", evt.back().str().c_str());


}
//------------
Event & Unfolding:: find_or_add(Ident & id)
{
   /* Need to check the event's history before adding it to the unf
    * Don't add events which are already in the unf
    */
   std::unordered_map <Ident, Event *, IdHasher<Ident>>::const_iterator got = evttab.find (id);

   if (got != evttab.end())
   {
      DEBUG("   already in the unf as event with idx = %d", evttab[id]->idx);
      return *evttab[id];
   }

   evt.push_back(Event(*this, id));
  // evt.back().update_parents(); // to make sure of conflict
   count++;
   /* set up vector clock */
   evt.back().set_vclock();
   evt.back().set_var_maxevt();

   /* add to the hash table evttabl */
   evttab.emplace(evt.back().evtid, &evt.back());

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

      switch (e.evtid.trans->type)
      {
         case ir::Trans::LOC:
            fs << e.idx << "[id="<< e.idx << ", label=\" " << e.dotstr() << " \" fillcolor=yellow];\n";
            fs << e.evtid.pre_proc->idx << "->" << e.idx << "[color=brown]\n";
            break;
         case ir::Trans::WR:
            fs << e.idx << "[id="<< e.idx << ", label=\" " << e.dotstr() << " \" fillcolor=red];\n";
            fs << e.evtid.pre_proc->idx << "->" << e.idx << "[color=brown]\n";
            for (auto const & pre : e.evtid.pre_readers)
               fs << pre->idx << " -> " << e.idx << "\n";
            break;
         case ir::Trans::RD:
            fs << e.idx << "[id="<< e.idx << ", label=\" " << e.dotstr() << " \" fillcolor=palegreen];\n";
            fs << e.evtid.pre_proc->idx << "->" << e.idx << "[color=brown]\n";
            fs << e.evtid.pre_mem->idx << "->" << e.idx << "\n";

            break;
         case ir::Trans::SYN:
            fs << e.idx << "[id="<< e.idx << ", label=\" " << e.dotstr() << " \" color=lightblue];\n";
            fs << e.evtid.pre_proc->idx << "->" << e.idx << "[color=brown]\n";
            fs << e.evtid.pre_mem->idx << "->" << e.idx << "\n";

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

   c.cprint_dot();
  // c.compute_cex();
   uprint_dot();
   uprint_debug();

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
      while ( (c.en[i]->evtid.trans->proc.id == 1) && (count < 15)) // set up when we want to add event in proc 1 (e.g: after 21 events in proc 0)
      {
         i = rand() % c.en.size();
      }
      c.add(i);
      count++;
   }
   // c.cprint_debug();
   uprint_debug();
   //c.compute_cex();
   c.cprint_dot();
   uprint_dot();
   return;
}
/*
 * Find all alternatives J after C and conflict with D
 */
void Unfolding:: alternative(Config & C, std::vector<Event *> D, std::vector<Event *> & J) // we only want to modify D in the scope of this function
{
   std::vector<std::vector<Event *>> spikes;
  // std::vector<Event *> combin;
/*
   printf("D = ");
   for (unsigned i = 0; i < D.size(); i++)
      printf("%d, ", D[i]->idx);
   DEBUG("");
*/
   DEBUG("This is alternative function");
   // Keep in D only events which are not in C.cex
   for (auto & e: C.cex)
      e->in_bit = 1;

   for (unsigned i = 0; i < D.size(); i++)
   {
      if (D[i]->in_bit == 1)
      {
         D[i] = D.back();
         D.pop_back();
      }
   }
/*
   printf("D after = ");
   for (unsigned i = 0; i < D.size(); i++)
      printf("%d, ", D[i]->idx);
   DEBUG("");
*/
 //  C.cprint_debug();
   /*
    *  D now contains only events which is in en(C).
    *  D is a comb whose each spike is a list of conflict events D[i].dicfl
    */

   for (unsigned i = 0; i < D.size(); i++)
      spikes.push_back(D[i]->dicfl);

   DEBUG("SPIKES: ");
   for (unsigned i = 0; i < spikes.size(); i++)
   {
      for (unsigned j = 0; j < spikes[i].size(); j++)
         printf(" %d", spikes[i][j]->idx);
      printf("\n");
   }
   printf("\n");

   compute_alt(0, spikes, J);

}

/*
 * check if all elements in combin are conflict-free in pairs
 */
bool is_conflict_free(std::vector<Event *> combin)
{
   for (unsigned i = 0; i < combin.size() - 1; i++)
     for (unsigned j = i; j < combin.size(); j++)
      {
        if (combin[i]->check_cfl(*combin[j]))
           return false;
      }
   printf("It is a conflict-free combination");
   return true;
}

/*
 * compute and return a set J which is a possible alternative to extend from C
 */
void Unfolding:: compute_alt(unsigned int i, const std::vector<std::vector<Event *>> & s, std::vector<Event *> & combin)
{
   DEBUG("This is compute_alt function");
   for (unsigned j = 0; j < s[i].size(); j++ )
     {
        if (j < s[i].size())
        {
           combin.push_back(s[i][j]);
           if (i == s.size() - 1)
           {
              printf("   Combination:");
              for (unsigned k = 0; k < combin.size(); k++)
                 printf("%d ", combin[k]->idx);
              printf("\n");
              /*
               * Do something here, check if the combination is conflict-free or not
               * is_conflict_free();
               */
              DEBUG("a combination");
              for (unsigned i = 0; i < combin.size(); i++)
                 printf ("%d ", combin[i]->idx);
              /*
              if (is_conflict_free(combin))
              {
                 return;
              }
              */
           }
           else
              compute_alt(i+1, s, combin);
        }
        combin.pop_back();
     }

   //combin.clear(); // make combin empty
  // return combin;
}

/*
 * Explore the entire unfolding
 */
void Unfolding:: explore(Config & C, std::vector<Event*> & D, std::vector<Event*> & A)
{
   Event * pe = nullptr;

   if (C.en.empty() == true) return ;

   if (A.empty() == true)
   {
       pe = C.en.back(); // choose the last element
   }
   else
   { //choose the mutual event in A and C.en to add
      for (auto & e: C.en)
         e->in_bit = 1;

      for (auto & a : A)
      {
         if (a->in_bit == 1)
         {
            pe = a;
            /* remove the choosen event from addable set*/
            a = A.back();
            A.pop_back();
            break;
         }
      }
   }

   Config C1(C);
   //C1.cprint_debug();
   C.add(*pe); // enable set is updated whenever pe is added to a configuration, pe is also removed from C.en in Config::add function
   //C.cprint_debug();
   explore (C, D, A);
   // When C.en is empty, choose an alternative to go

   DEBUG("Here start computing alternative");
   //Pop up the last event from C
   D.push_back(pe); // ???pe???

   std::vector<Event *> J;
   alternative(C1,D, J); //to see more carefully

   printf("J = ");
   for (unsigned i = 0; i < J.size(); i++)
      printf("%d, ", J[i]->idx);
   DEBUG("");

   if (J.empty() == false)
   {
      A.push_back(*J.begin()); // choose the first element in J to add
      explore (C1, D, A); // use C1, the state of C before adding pe
      D.pop_back();
   }
}
//----test conflict
void Unfolding:: test_conflict()
{
   DEBUG("What's the hell");
   DEBUG("Size of evt: %zu", evt.size());
   pes::Event * e1, * e2;

   for (unsigned i = 0; i < evt.size(); i++)
   {
      if (evt[i].idx == 7)    e1 = &evt[i];
      if (evt[i].idx == 5)    e2 = &evt[i];
   }

   assert(e1 !=nullptr);
   assert(e2 !=nullptr);

   e1->eprint_debug();
   e2->eprint_debug();
   DEBUG("We need to check cfl between %d and %d", e1->idx, e2->idx);

   if (e1->is_bottom() or e2->is_bottom())
   {
      DEBUG("One event is bottom");
      return;
   }

   if (e1->check_cfl(*e2))
      printf("They are in conflict");
   else
      printf("They are not in conflict");
}

} // end of namespace

