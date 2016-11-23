
#ifndef __PES_HH_
#define __PES_HH_

#include <unordered_map>
#include <functional>

#include "config.h"
#include "verbosity.h"
#include "vclock.hh"
#include "misc.hh"

//#include "cfltree.hh"

namespace dpu
{

class Event;
class EventBox;
class EventIt;
class Process;
class Unfolding;
class BaseConfig;

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

class Event // : public MultiNode<Event,2,3> // 2 trees, skip step = 3
{
private:
   Event *_pre_other;
public:
   int idx;
   /// THSTART(), creat is the corresponding THCREAT (or null for p0)
   inline Event (Event *creat);
   /// THCREAT(tid) or THEXIT(), one predecessor (in the process)
   inline Event (Action ac);
   /// THJOIN(tid), MTXLOCK(addr), MTXUNLK(addr), two predecessors (process, memory/exit)
   inline Event (Action ac, Event *m);

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

   /// true iff this event is the THSTART event of thread 0
   inline bool is_bottom ();

   /// returns the pid of the process to which this event belongs
   inline unsigned pid () const;
   /// returns the process to which this event belongs
   inline Process *proc ();

   /// tests pointer equality of two events
   inline bool operator == (const Event &) const;
   /// returns a human-readable description of the event
   std::string str () const;

private:
   inline void post_add (Event * const succ);

   inline EventBox *box_above () const;
   inline EventBox *box_below () const;

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

   void dump ();
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
   inline Process (Event *creat);

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
 *  class to represent a configuration in unfolding
 */
class BaseConfig
{
public:
   void add (Event & e); // update the cut and the new event

   /// creates a copy of this configuration
   BaseConfig clone ();
   /// prints the configuration in stdout
   void dump ();
   
   /// creates an empty configuration
   BaseConfig (const Unfolding &u);
   /// creates a local configuration
   BaseConfig (const Unfolding &u, Event &e);
   
public:
   /// map from process id (int) to maximal event in that process (size = u.num_procs())
   Event **max;
   int size;
};

// implementation of templates
#include "action.hpp"
#include "event.hpp"
#include "unfolding.hpp"
#include "process.hpp"
#include "eventbox.hpp"

} // namespace dpu

#endif


