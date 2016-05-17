
#ifndef __PES_HH_
#define __PES_HH_

#include "ir.hh"
/*
 * Event:
 * - take unf and trans as arguments
 * - constructror: private
 * -
 * Unfolding:
 * - class friend with Event
 * - call unf.addevent();
 * - add method: dot_print for config and unf
 * -
 *
 */

namespace pes
{
class Config;
class Unfolding;

template <typename T>
struct Node
{
   unsigned depth;
   T ** skip_preds;
};
#if 0
template <class T, int S>
class MultiNode
{
public:
   Node<T> node[S];

   template<int idx>
   void print_pred ()
   {
      int i = 1;
      for (T *n = (T*) this; n; ++i, n = n->node[idx].pre)
      {
         printf ("%d. n %p depth %d\n", i, n, n->node[idx].depth);
      }
   }
   template<int idx>
   void find_pred(int d);
};
#endif

template <class T>
class MultiNode
{
public:
   Node<T> node[2];

   void print_pred (int idx);

   void find_pred  (int idx, int d, int step);
};

class Event: public MultiNode<Event>
{
public:
   unsigned int          idx;
   Event *               pre_proc;    // for all events, predecessor in the same process
   std::vector<Event *>  post_proc;  // set of successors in the same process

   Event *               pre_mem;     // parent of the event, for all events except LOCAL,

   // only for WR events
   // each vector of children events for a process
   std::vector< std::vector<Event *> >   post_mem; // size = numprocs x mem
   std::vector< Event * >                pre_readers; // only for WR events

   std::vector<Event * > post_wr; // WR children of a WR trans

   //only for RD and SYN events
   std::vector <Event *>                 post_rws; // any operation after a RD, SYN

   uint32_t              val; //??? value for global variable?
   std::vector<uint32_t> localvals; //???
   const ir::Trans *     trans;
   int                   color;
   std::vector<Event *>  dicfl;  // set of direct conflicting events
   std::vector <int>     clock; // size = number of processes (to store clock for all its predecessors: pre_proc, pre_mem or pre_readers)
   std::vector <Event *> maxevt; // size of number of variables, store maximal events in the event's local configuration for all variable

   bool         operator ==   (const Event &) const;
   Event & 	    operator =    (const Event &);
   std::string  str           () const;
   std::string dotstr         () const;

   Event (const ir::Trans & t, Config & c); // make it public to use emplace_back(). Consider to a new allocator if constructors are private
   Event (const Event & e);
   void mk_history (const Config & c);
   void update_parents();
   bool check_cfl(const Event & e) const;
   void eprint_debug() const;
   bool is_bottom () const;
   bool is_same(Event &);
   bool in_history(Event * e);
   void set_vclock();
   void set_maxevt();

   Node<Event> &proc () { return node[0]; }
   Node<Event> &var  () { return node[1]; }
   void proc_print_pred () { print_pred(0); }
   void var_print_pred  () { print_pred(1); }



   friend Unfolding;

private:
   Event() = default;
   Event (Unfolding & u);
   Event (const ir::Trans & t, Unfolding & u);
   Event (const ir::Trans & t, Event * ep, Event * em, Unfolding & u);
   Event (const ir::Trans & t, Event * ep, std::vector<Event *> pr, Unfolding & u);

}; // end of class Event


/*
 *  class to represent a configuration in unfolding
 */
class Config
{
public:
   ir::State                        gstate;
   Unfolding  &                     unf;
   /*
    * latest_proc : Processes -> Events
    * initialzed by memsize but actually it uses only the last (memsize - numprocs) elements
    * latest_wr   : Variables -> Events
    * latest_op   : (Processes x Variables) -> Events
    *
    * where Variables is ALL variables
    */
   std::vector<Event*>              latest_proc;
   std::vector<Event*>              latest_wr;
   std::vector<std::vector<Event*>> latest_op;

   std::vector<Event*>              en;
   std::vector<Event*>              cex;
   //std::vector<Event*>              cex1;



   Config (Unfolding & u); // creates an empty configuration
   Config (const Config & c);
   
   void add (const Event & e); // update the cut and the new event
   void add (unsigned idx); // update the cut and the new event
   void add_any ();
   void compute_cex ();
   void add_to_cex(Event * temp);
   void RD_cex(Event * e);
   void SYN_cex(Event * e);
   void WR_cex(Event * e);
   void compute_combi(unsigned int i, const std::vector<std::vector<Event *>> & s, std::vector<Event *> combi, Event * e);

   void cprint_debug () const;
   void cprint_dot(std::string &, std::string &);
   void cprint_dot();

private:
   void __update_encex (Event & e);
   void __print_en() const;
   void __print_cex() const;
   void remove_cfl (Event & e); // modify e.dicfl
}; // end of class Config

class Unfolding
{
public:
   static unsigned count; // count number of events.
   std::vector<Event>    evt; // events actually in the unfolding
   ir::Machine &         m;
   Event *               bottom;
   int                   colorflag;

   Unfolding (ir::Machine & ma);
   void create_event(ir::Trans & t, Config &);
   Event & find_or_add(const ir::Trans & t, Event * ep, Event * pr_mem);
   Event & find_or_addWR(const ir::Trans & t, Event * ep, std::vector<Event *> combi);

   void uprint_debug();
   void uprint_dot();

   void explore(Config & C, std::vector<Event *> D, std::vector<Event *> A);
   void explore_rnd_config ();
   void explore_driven_config ();
   void alternative(Config & C, std::vector<Event *> D);
   void compute_alt(unsigned int i, const std::vector<std::vector<Event *>> & s, std::vector<Event *> combi);
   friend Event;

private :
   void __create_bottom ();

}; // end of class Unfolding

} // namespace pes


#endif


