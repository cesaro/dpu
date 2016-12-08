
#ifndef __PES_HH_
#define __PES_HH_

#include <unordered_map>
#include <functional>

#include "config.h"
#include "verbosity.h"
#include "vclock.hh"
#include "misc.hh"
#include "cfltree.hh"

namespace dpu
{

class Event;
class EventBox;
class EventIt;
class Process;
class Unfolding;
//class Cut;
//class Config;

typedef uint64_t Addr;

enum class ActionType
{
   // loads
   RD8,
   RD16,
   RD32,
   RD64,
   // stores
   WR8,
   WR16,
   WR32,
   WR64,
   // memory management
   MALLOC,
   FREE,
   // threads
   THCREAT,
   THSTART,
   THEXIT,
   THJOIN,
   // locks
   //MTXINIT,
   MTXLOCK,
   MTXUNLK,
};

const char *action_type_str (ActionType t);
const char *action_type_str (unsigned t);

/// Action (type, val)
struct Action
{
   ActionType type;
   Addr addr;
   uint64_t val;

   void pretty_print ();
   inline bool operator == (const Action &other) const;
};

/*
 *  class to represent a cut
 */
class Cut
{
public:
   /// FIXME - is this necessary?
   std::vector<Event *> cex;
   /// creates an empty cut for as much as u.num_procs processes
   inline Cut (const Unfolding &u);
   /// creates an empty cut for as much as n processes
   inline Cut (unsigned n);
   /// copy constructor
   inline Cut (const Cut &other);
   /// max of two cuts
   inline Cut (const Cut &c1, const Cut &c2);
   /// destructor
   inline ~Cut ();

   /// add the event to the cut
   inline void add (Event *e);

   /// empties the configuration
   inline void clear ();
   /// prints the cut in stdout
   void dump () const;
   /// returns a human-readable description
   std::string str () const;

   /// returns the maximal event of process pid in the cut
   inline Event *operator[] (unsigned pid) const;
   inline Event *&operator[] (unsigned pid);

   /// returns the number of processes of the unfolding
   inline unsigned num_procs () const;


protected:
   /// size of the map below (u.num_procs())
   unsigned nrp;
   /// map from process id (int) to maximal event in that process
   Event **max;

   void __dump_cut () const;
private :
   // class Event uses the following three constructors
   inline Cut (unsigned n, Event *e);
   inline Cut (const Cut &other, Event *e);
   inline Cut (const Cut &c1, const Cut &c2, Event *e);
   friend class Event;
};

class Event : public MultiNode<Event,3> // 2 trees, skip step = 3
{
private:
   Event *_pre_other;
public:
   int idx;
   int inside; // a flag to mark that an event is inside some set or not
   /// THSTART(), creat is the corresponding THCREAT (or null for p0)
   inline Event (Event *creat);
   /// THCREAT(tid) or THEXIT(), one predecessor (in the process)
   inline Event (Action ac, bool boxfirst);
   /// THJOIN(tid), MTXLOCK(addr), MTXUNLK(addr), two predecessors (process, memory/exit)
   inline Event (Action ac, Event *m, bool boxfirst);

   struct {
      /// True iff this event is the first one in its own EventBox
      int boxfirst : 1;
      /// True iff this event is the last one in its own EventBox
      int boxlast : 1;
      /// FIXME - True iff this event has no causal successors in the unfolding
      //int ismax : 1;
      /// True iff this event is in some configuration
      int inc : 1;
   } flags;

   /// the blue action performed by this event
   Action action;
   /// a list of red actions that the thread performed between Events pre() and this
   std::vector<Action> redbox;
   /// the vector clock
   Vclock vclock;
   /// color mark for various algorithms
   unsigned color;
   /// all the causal successors of this event
   std::vector<Event*> post;
   /// FIXME
   Event *next;

   /// predecessor in my thread, or null if THSTART
   inline Event *pre_proc ();
   /// predecessor in another thread (if any), or null
   inline Event *pre_other ();

   /// returns the pid of the process to which this event belongs
   inline unsigned pid () const;
   /// returns the process to which this event belongs
   inline Process *proc () const;

   inline unsigned depth_proc () const;

   /// tests pointer equality of two events
   inline bool operator == (const Event &) const;
   /// returns a human-readable description of the event
   std::string str () const;
   std::vector<Event *> local_config();

   /// true iff this event is the THSTART event of thread 0
   inline bool is_bottom ();

   template <int i>
   inline bool is_pred_in_the_same_tree_of(const Event *e) const;
   inline bool is_pred_of (const Event *e) const;

//   template <int idx>
//   inline bool is_cfl_in_the_same_tree_of(const Event *e) const;
   inline bool in_cfl_with (const Event *e);

   inline bool in_icfl_with (const Event *e); // Cesar

   /// the cut of the local configuration of the event
   const Cut cut;

private:
   inline void post_add (Event * const succ);
   inline Event *pre_proc (bool bf);

   inline EventBox *box_above () const;
   inline EventBox *box_below () const;

   // FIXME -- this should be a Cut instead of a std::vector, see below

#if 0
   //void mk_history (const Config & c);
   void update_parents();
   void eprint_debug();

   void set_vclock();
   void set_var_maxevt();
   void set_proc_maxevt();
   void compute_maxvarevt(std::vector<Event *> & maxevt, unsigned var) const;

   const Event & find_latest_WR_pred ()const;
   const std::vector<Event *> local_config() const;
   bool check_dicfl (const Event & e); // check direct conflict
   bool check_cfl (const Event & e); // check conflict
   bool check_cfl_WRD(const Event & e) const; // this is a WR and e is a RD
   bool check_cfl_2RD(const Event & e) const;
   bool check_2LOCs (const Event & e);
   bool is_bottom () const;
   bool is_same (const Event &) const;
   bool succeed (const Event & e) const;
   bool is_in_mutex() const;
   template <int idx>
   bool check_cfl_same_tree (const Event & e) const;
   bool found(const Event &e, Event *parent) const;

   //Node<Event,3> &proc () { return node[0]; }
   //Node<Event,3> &var  () { return node[1]; }

   //void set_skip_preds(int idx, int step);
   //void print_proc_skip_preds(){ proc().print_skip_preds();}
   //void print_var_skip_preds() { var().print_skip_preds();}

   //friend class Node<Event,3>;
#endif

   friend class EventIt;
};


class Unfolding
{
public:
   //static unsigned count;
   inline Unfolding ();
   inline ~Unfolding ();
   Unfolding (const Unfolding &other) = delete; // no copy constructor
   inline Unfolding (const Unfolding &&other);

   void dump () const;
   //void print_dot (FILE *f);
   void print_dot();

   /// returns a pointer to the process number pid
   inline Process *proc (unsigned pid) const;

   /// returns the number of processes currently present in this unfolding
   unsigned num_procs () const { return nrp; }

   /// The following methods create or retrieve existing events from the unfolding
   /// THSTART(), creat is the corresponding THCREAT (or null for p0);
   /// this will create a new process if a new event needs to be created
   inline Event *event (Event *creat);
   /// THCREAT(tid) or THEXIT(), one predecessor (in the process)
   inline Event *event (Action ac, Event *p);
   /// THJOIN(tid), MTXLOCK(addr), MTXUNLK(addr), two predecessors (process, memory/exit)
   inline Event *event (Action ac, Event *p, Event *m);

   /// returns a new fresh color
   inline unsigned get_fresh_color ();

   /// maximum number of processes in the unfolding
   static constexpr size_t MAX_PROC = CONFIG_MAX_PROCESSES;
   /// FIXME
   static constexpr size_t PROC_SIZE = alignp2 (CONFIG_MAX_EVENTS_PER_PROCCESS * sizeof (Event));
   /// FIXME
   static constexpr size_t ALIGN = PROC_SIZE * alignp2 (MAX_PROC);

   /// extracts the pid to which an object belongs (Process, EventBox, Event)
   static unsigned ptr2pid (const void *ptr)
      { return (((size_t) ptr) >> int2msb (PROC_SIZE)) & int2mask (MAX_PROC - 1); }
   static unsigned ptr2pindex (const void *ptr)
      { return int2mask (PROC_SIZE - 1) & (size_t) ptr; }
   static Process *ptr2proc (const void *ptr)
      { return (Process *) (((size_t) ptr) & ~int2mask (PROC_SIZE - 1)); }
private :
   /// memory space where Processes, EventBoxes and Events will be stored
   char *procs;
   /// number of processes created using new_proc (<= MAX_PROC)
   unsigned nrp;
   /// a fresh color, used for Events or anywhere else
   unsigned color;

   /// creates a new process in the unfolding; creat is the corrsponding THCREAT
   /// event
   inline Process *new_proc (Event *creat);

   /// returns the (only) immediate process causal successor of p with action *ac, or NULL
   inline Event *find1 (Action *ac, Event *p);
   /// returns the (only) immediate causal successor of {p,m} with action *ac, or NULL
   inline Event *find2 (Action *ac, Event *p, Event *m);
};

class EventBox
{
public:
   /// constructor
   inline EventBox (Event *pre);
   /// first event contained in the box, undefined if the box contains no event
   inline Event *event_above () const;
   /// the last event of the box below, undefined if this is the first box
   inline Event *event_below () const;
   /// process causal predecessor of all events in the box
   inline Event *pre () const;
   /// returns the id of the process containing this event box
   inline unsigned pid () const;

private:
   /// the event that causally precedes all events in this box
   Event *_pre;
};

class Process
{
public :
   inline Process (Event *creat, Unfolding &u);

   void dump ();

   /// iterator over the events in this process
   inline EventIt begin ();
   inline EventIt end ();

   /// returns the pid of this process
   inline unsigned pid () const;
   /// returns the offset of a pointer within the memory pool of this process
   inline size_t offset (void *ptr) const;

   /// returns the THSTART event of this process
   inline Event *first_event () const;

   /// THCREAT(tid), THEXIT(), one predecessor (in the process)
   inline Event *add_event_1p (Action ac, Event *p);
   /// THJOIN(tid), MTXLOCK(addr), MTXUNLK(addr), two predecessors (process, memory/exit)
   inline Event *add_event_2p (Action ac, Event *p, Event *m);

private :
   /// last event inserted in the memory pool of this process
   Event *last;

   inline EventBox *first_box () const;
};

class EventIt
{
public :
   bool operator== (const EventIt &other) const
      { return e == other.e; }
   bool operator!= (const EventIt &other) const
      { return e != other.e; }

   EventIt &operator++ ()
      { e = e->flags.boxlast ? e->box_above()->event_above() : e + 1; return *this; }
   EventIt operator++ (int x)
      { EventIt copy = *this; ++(*this); return copy; }

   Event &operator* ()
      { return *e; }
   Event *operator-> ()
      { return e; }

private:
   /// the event currently pointed by this iterator
   Event *e;

   /// constructor
   EventIt (Event *e) :
      e (e)
      { }
   friend class Process;
};

/*
 *  class to represent a configuration
 */
class Config : public Cut
{
public:
   /// creates an empty onfiguration for as much as u.num_procs processes
   inline Config (const Unfolding &u);
   /// creates an empty configuration for as much as n processes
   inline Config (unsigned n);
   /// copy constructor
   inline Config (const Config &other);
   
   /// add the event to the configuration
   inline void add (Event *e);

   /// empties the configuration
   inline void clear ();
   /// prints the cut in stdout
   void dump ();

   /// maximal event for the given pid, or nullptr
   inline Event *proc_max (unsigned pid);
   /// maximal event for the given address (or THCREAT/EXIT if pid is given)
   inline Event *mutex_max (Addr a);
   
public:
   /// map from lock addresss to events
   std::unordered_map<Addr,Event*> mutexmax;

   void __dump_mutexes ();
};

// implementation of inline methods
#include "action.hpp"
#include "event.hpp"
#include "unfolding.hpp"
#include "process.hpp"
#include "eventbox.hpp"
#include "cut.hpp"
#include "config.hpp"

} // namespace dpu

#endif

