
#ifndef __PES_HH_
#define __PES_HH_

#include "ir.hh"

namespace pes
{
class Config;
class Unfolding;


class Event
{
public:
   Event *               pre_proc;    // for all events, predecessor in the same process
   std::vector<Event *>  post_proc;  // set of successors in the same process

   Event *               pre_mem;     // parent of the event, for all events except LOCAL,

   // only for WR events
   //each vector of children events for a process
   std::vector<std::vector <Event *> >   post_mem; // size = numprocs x numprocs
   std::vector< Event * >                pre_readers; // only for WR events

   //write children of a write trans
   std::vector<std::vector < Event * > > post_wr; // sizze = numprocs x numprocs

   //only for RD and SYN events
   std::vector <Event *>                 post_rws; // for RD, WR, and SYN events, size = number of variables

   int                   val; //??? value for global variable?
   std::vector<uint32_t> localvals; //???
   const ir::Trans *     trans;

   Event (unsigned, unsigned);
   Event (const ir::Trans & t, unsigned numprocs, unsigned memsize);
   Event (const Event & e);

   bool         operator ==   (const Event &) const;
   Event & 	    operator =    (const Event &);
   std::string  str           () const;

   void mk_history (const Config & c);
   void update_parents();
   bool check_cfl(const Event & e) const;
   bool is_bottom () const;
   void eprint_debug() const;


}; // end of class Event


/*
 *  class to represent a configuration in unfolding
 */
class Config
{
public:
   ir::State                        gstate;

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
   Unfolding  &                     unf;


   Config (Unfolding & u); // creates an empty configuration
   Config (const Config & c);
   
   void add (const Event & e); // update the cut and the new event
   void add (unsigned idx); // update the cut and the new event
   void add_any ();

   void print_debug () const;

private:
   void __update_encex (const Event & e);
   void __print_en() const;
   void remove_cfl (const Event & e);


}; // end of class Config

class Unfolding
{
public:
   std::vector<Event>    evt; // events actually in the unfolding
   ir::Machine &         m;
   Event *               bottom;

   Unfolding (ir::Machine & ma);

   //methods
   void explore(Config & C, std::vector<Event *> D, std::vector<Event *> A);
   void explore_rnd_config ();
   void uprint_debug();

private :
   void __create_bottom ();

}; // end of class Unfolding


} // namespace pes

#endif
