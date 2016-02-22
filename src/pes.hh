
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


class Event
{
public:
   unsigned int          idx;
   Event *               pre_proc;    // for all events, predecessor in the same process
   std::vector<Event *>  post_proc;  // set of successors in the same process

   Event *               pre_mem;     // parent of the event, for all events except LOCAL,

   // only for WR events
   //each vector of children events for a process
   std::vector<std::vector <Event *> >   post_mem; // size = numprocs x numprocs
   std::vector< Event * >                pre_readers; // only for WR events

   //write children of a write trans
   std::vector<std::vector < Event * > > post_wr; // size = numprocs

   //only for RD and SYN events
   std::vector <Event *>                 post_rws; // for RD, WR, and SYN events, size = number of variables

   int                   val; //??? value for global variable?
   std::vector<uint32_t> localvals; //???
   const ir::Trans *     trans;

   bool         operator ==   (const Event &) const;
   Event & 	    operator =    (const Event &);
   std::string  str           () const;

   void mk_history (const Config & c);
   void update_parents();
   bool check_cfl(const Event & e) const;
   bool is_bottom () const;
   void eprint_debug() const;
   Event (const Event & e);
   friend Unfolding;

private:
   Event (Unfolding & u);
   Event (const ir::Trans & t, Unfolding & u);

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
   void add (unsigned idx, std::string &);
   void add_any ();

   void cprint_debug () const;
   void cprint_dot(std::string &, std::string &);

private:
   void __update_encex (const Event & e);
   void __print_en() const;
   void remove_cfl (const Event & e);


}; // end of class Config

class Unfolding
{
public:
   static unsigned count; // count number of events.
   std::vector<Event>    evt; // events actually in the unfolding
   ir::Machine &         m;
   Event *               bottom;

   Unfolding (ir::Machine & ma);
   void create_event(ir::Trans & t);
   void uprint_debug();
   void uprint_dot(std::string);
   void uprint_dot();
   void explore(Config & C, std::vector<Event *> D, std::vector<Event *> A);
   void explore_rnd_config ();
   friend Event;

private :
   void __create_bottom ();

}; // end of class Unfolding

} // namespace pes

#endif


