
#ifndef __PES_HH_
#define __PES_HH_

#include "ir.hh"
#include <unordered_map>
#include <functional>

/*
 * Event:
 * - take unf and trans as arguments
 * - constructror: private
 * -
 * Unfolding:
 * - class friend with Event
 * - call unf.add_event();
 * - add method: dot_print for config and unf
 * -
 *
 */

namespace pes
{
class Config;
class Unfolding;
class Event;

//-------template class Node ---------
template <class T, int SS>
class Node
{
public:
   unsigned depth;
   T * pre; // immediate predecessor
   T ** skip_preds;

   Node();
   Node(int idx, Event * pr);
   void set_up(int idx, Event * pr);
   int compute_size();
   void set_skip_preds(int idx);
   void print_skip_preds();

   template <int idx>
   T & find_pred(int d) const;
};
//-------template class MultiNode------
template <class T, int S, int SS> // S: number of trees, SS: skip step
class MultiNode
{
public:
   Node<T,SS> node[S];

   MultiNode() = default;
   MultiNode(T * pp, T * pm);
};

//-----Ident class-------------
class Ident
{
public:
   const ir::Trans * trans;
   Event * pre_proc;
   Event * pre_mem;
   std::vector<Event *> pre_readers;

public:
   Ident();
   Ident(const ir::Trans * t, Event * pp, Event * pm, std::vector<Event*> pr);
   Ident(const ir::Trans & t, const Config & c);
   Ident(const Ident & id);
   std::string str() const;
   bool operator == (const Ident & id) const;
};
//-------template hash function for pointer----
template<typename Tval>
struct MyTemplatePointerHash {
    size_t operator()(const Tval* val) const
    {
       static const size_t shift = (size_t)log2(1 + sizeof(Tval));
       return (size_t)(val) >> shift;
    }
};
template <typename V>
struct MyVectorHash
{
   size_t operator() (const std::vector<V *> v) const
   {
      if (v.empty())
         return (size_t) 0;

      size_t it = (size_t) v[0] >> 3;
      for (int i = 1; i < v.size(); i++)
         it = it ^ ((size_t) v[i] >> 3);// shift a bit to the right, 3 for 64 bit, 1 for 32 bi
      return it;
    }
};
//--------struct IdHasher--------
template <class T> struct IdHasher;
template<> struct IdHasher<Ident>
{
   std::size_t operator() (const Ident& k) const
   {
      std::size_t h1 = MyTemplatePointerHash<ir::Trans>() (k.trans);
      std::size_t h2 = MyTemplatePointerHash<Event>() (k.pre_proc);
      std::size_t h3 = MyTemplatePointerHash<Event>() (k.pre_mem);
      std::size_t h4 = MyVectorHash<Event>() (k.pre_readers);

      return h1^h2^h3^h4;

   }
};

//--------class Event------------
class Event: public MultiNode<Event,2,3> // 2 trees, skip step = 3
{
public:
   unsigned int          idx;
   Ident                 evtid;

   std::vector<Event *>  post_proc;  // set of successors in the same process

   /*
    *  Only for WR events
    *  Each vector of children events for a process
    */
   std::vector< std::vector<Event *> >   post_mem; // size = numprocs x mem
   std::vector<Event * >                 post_wr; // WR children of a WR trans

   //only for RD and SYN events
   std::vector <Event *>                 post_rws; // any operation after a RD, SYN

   uint32_t              val; //??? value for global variable?
   std::vector<uint32_t> localvals; //???

   int                   color; // to avoid re-read an event in printing functions.
   int                   in_bit; // to mark that an event is in cex or not: 1 - in, 0: not in

   std::vector<Event *>  dicfl;  // set of direct conflicting events
   std::vector <int>     clock; // size = number of processes (to store clock for all its predecessors: pre_proc, pre_mem or pre_readers)
   //store maximal events in the event's local configuration for all variables
   std::vector <Event *> var_maxevt; // size of number of variables


   bool         operator ==   (const Event &) const;
   Event & 	    operator =    (const Event &);
   std::string  str           () const;
   std::string dotstr         () const;


   // Constructors
   Event() = default;
   Event (Unfolding & u);
   Event (Unfolding & u, Ident & ident);

   //void mk_history (const Config & c);
   void update_parents();
   void eprint_debug();

   void set_vclock();
   void set_var_maxevt();

   const Event & find_latest_WR_pred ()const;
   bool check_dicfl (const Event & e); // check direct conflict
   bool check_cfl (const Event & e); // check conflict
   bool check_cfl_WRD(const Event & e) const; // this is a WR and e is a RD
   bool check_cfl_2RD(const Event & e) const;
   bool check_2LOCs (const Event & e);
   bool is_bottom () const;
   bool is_same (const Event &) const;
   bool succeed (const Event & e) const;

   template <int idx>
   bool check_cfl_same_tree (const Event & e) const;


   Node<Event,3> &proc () { return node[0]; }
   Node<Event,3> &var  () { return node[1]; }

   //void set_skip_preds(int idx, int step);
   void print_proc_skip_preds(){ proc().print_skip_preds();}
   void print_var_skip_preds() { var().print_skip_preds();}

   friend class Node<Event,3>;
   friend Unfolding;


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
   void cprint_event() const ;
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
   std::unordered_map <Ident, Event *, IdHasher<Ident>>    evttab;
   ir::Machine &         m;
   Event *               bottom;
   int                   colorflag;


   Unfolding (ir::Machine & ma);
   void create_event(ir::Trans & t, Config &);
   Event & find_or_add(Ident & id);


   void uprint_debug();
   void uprint_dot();

   void explore(Config & C, std::vector<Event *> & D, std::vector<Event *> & A);
   void explore_rnd_config ();
   void explore_driven_config ();
   void find_an_alternative(Config & C, std::vector<Event *> D, std::vector<Event *> & J, std::vector<Event *> & A );
   //std::vector<Event *> compute_alt(unsigned int i, const std::vector<std::vector<Event *>> & s, std::vector<Event *> & combi);
   void compute_alt(unsigned int i, const std::vector<std::vector<Event *>> & s, std::vector<Event *> & combi, std::vector<Event *> & A);
   friend Event;
   void test_conflict();

private :
   void __create_bottom ();

}; // end of class Unfolding

} // namespace pes

#endif


