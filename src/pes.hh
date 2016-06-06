
#ifndef __PES_HH_
#define __PES_HH_

#include "ir.hh"
#include <unordered_map>
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
class Event;
//class Ident;

template <class T, int SS>
class Node
{
public:
   unsigned depth;
   T * ref;
   T * pre; // immediate predecessor
   T ** skip_preds;

   Node();
   Node(int idx, Event * pr);
   void set_up(int idx, Event * pr);
   int compute_size();
   void set_skip_preds(int idx);
   void print_skip_preds();
   Event & find_pred(int d) const;


};

template <class T, int S, int SS> // S: number of trees, SS: skip step
class MultiNode
{
public:
   Node<T,SS> node[S];

   MultiNode() = default;
   MultiNode(T * pp, T * pm);

};
//----------------
class Ident
{
   ir::Trans * trans;
   Event * pre_proc;
   Event * pre_mem;
   std::vector<Event *> pre_readers;

public:
   Ident() = default;
   Ident(ir::Trans * t, Event * pp,Event * pm, std::vector<Event*> pr);
   bool          operator ==  (const Ident eid);
};

//--------class Event------------
class Event: public MultiNode<Event,2,3> // 2 trees, skip step = 3
{
public:
   unsigned int          idx;
   Ident                 evtid;
   Event *               pre_proc;    // for all events, predecessor in the same process
   Event *               pre_mem;     // parent of the event, for all events except LOCAL,

   std::vector<Event *>  post_proc;  // set of successors in the same process

   // only for WR events
   // each vector of children events for a process
   std::vector< Event * >                pre_readers; // only for WR events
   std::vector< std::vector<Event *> >   post_mem; // size = numprocs x mem
   std::vector<Event * >                 post_wr; // WR children of a WR trans

   //only for RD and SYN events
   std::vector <Event *>                 post_rws; // any operation after a RD, SYN

   uint32_t              val; //??? value for global variable?
   std::vector<uint32_t> localvals; //???
   const ir::Trans *     trans;
   int                   color;
   std::vector<Event *>  dicfl;  // set of direct conflicting events

   std::vector <int>     clock; // size = number of processes (to store clock for all its predecessors: pre_proc, pre_mem or pre_readers)
   //store maximal events in the event's local configuration for all variables and processes
   std::vector <Event *> proc_maxevt; // size of number of processes
   std::vector <Event *> var_maxevt; // size of number of variables


   bool         operator ==   (const Event &) const;
   Event & 	    operator =    (const Event &);
   std::string  str           () const;
   std::string dotstr         () const;

   Event (const ir::Trans & t, Config & c); // make it public to use emplace_back(). Consider to a new allocator if constructors are private
   Event (const Event & e);
   void mk_history (const Config & c);
   void update_parents();
   void eprint_debug() const;

   void set_vclock();
   void set_proc_maxevt();
   void set_var_maxevt();

   Event & find_latest_WR()const;
   bool check_dicfl(const Event & e); // check direct conflict
   bool check_cfl(const Event & e); // check conflict
   bool check_conflict_same_proc_tree(const Event & e);
   bool check_conflict_same_var_tree(const Event & e);
   bool check_conflict_local_config(const Event & e);
   bool is_bottom () const;
   bool is_same(Event &) const;
   bool in_history(Event * e);



   Node<Event,3> &proc () { return node[0]; }
   Node<Event,3> &var  () { return node[1]; }
   //void set_skip_preds(int idx, int step);
   void print_proc_skip_preds();
   void print_var_skip_preds();

   friend class Node<Event,3>;
   friend Unfolding;

private:
   Event() = default;
   Event (Unfolding & u);
  // Event (const ir::Trans & t, Unfolding & u);
   Event (const ir::Trans & t, Event * ep, Event * em, Unfolding & u);
   Event (const ir::Trans & t, Event * ep,  Event * ew, std::vector<Event *> pr, Unfolding & u);

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

//----------------
class Unfolding
{
public:
   static unsigned count; // count number of events.
   std::vector<Event>    evt; // events actually in the unfolding
 //  std::unordered_map <EventID, Event *> evttab;
   ir::Machine &         m;
   Event *               bottom;
   int                   colorflag;


   Unfolding (ir::Machine & ma);
   void create_event(ir::Trans & t, Config &);
   Event & find_or_add(const ir::Trans & t, Event * ep, Event * pr_mem);
   Event & find_or_addWR(const ir::Trans & t, Event * ep, Event * ew, std::vector<Event *> combi);

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


