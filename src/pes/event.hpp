
/// THSTART(), creat is the corresponding THCREAT (or null for p0)
Event::Event (Event *creat, bool bf) :
   MultiNode(pre_proc(bf), creat),
   flags ({.boxfirst = bf, .boxlast = 0, .crb = 0, .ind = 0}),
   action ({.type = ActionType::THSTART}),
   color (0),
   dat (nullptr),
   // pre_proc(bf) is null or a causal predecessor of creat->cone[pid()], so
   // it's safe to construct our cone like this:
   cone (creat ? Primecon (creat->cone, this) : Primecon (pid() + 1, this)),
   depth (creat ? creat->depth + 1 : 0)
{
   ASSERT (action.type == ActionType::THSTART);
   ASSERT (pre_other() == creat);
   ASSERT (!creat or creat->action.type == ActionType::THCREAT);
   // the first THSTART needs to have a null process predecessor
   ASSERT (pre_proc() == (creat ? creat->cone[pid()] : nullptr));

   //DEBUG ("Event.ctor: %s", str().c_str());
   //DEBUG ("Event.ctor: e %-16p %s", this, cone.str().c_str());
}

/// THCREAT(tid) or THEXIT(), one predecessor (in the process)
Event::Event (Action ac, bool bf) :
   MultiNode(pre_proc(bf), nullptr),
   flags ({.boxfirst = bf, .boxlast = 0, .crb = 0, .ind = 0}),
   action (ac),
   color (0),
   dat (nullptr),
   cone (pre_proc()->cone, this),
   depth (pre_proc(bf)->depth + 1)
{
   ASSERT (pre_proc() != 0);
   ASSERT (pre_other() == 0);

   //DEBUG ("Event.ctor: %s", str().c_str());
   //DEBUG ("Event.ctor: e %-16p %s", this, cone.str().c_str());
}

/// THJOIN(tid), MTXLOCK(addr), MTXUNLK(addr), two predecessors (process, memory/exit)
Event::Event (Action ac, Event *m, bool bf) :
   MultiNode(pre_proc(bf), m),
   flags ({.boxfirst = bf, .boxlast = 0, .crb = 0, .ind = 0}),
   action (ac),
   color (0),
   dat (nullptr),
   cone (m ?
         Primecon (pre_proc()->cone, m->cone, this) :
         Primecon (pre_proc()->cone, this)),
   depth (1 + std::max (pre_proc(bf)->depth, m ? m->depth : 0))
{
   // m could be null (eg, first lock of an execution)
   ASSERT (pre_proc() != 0);
   ASSERT (pre_other() == m);

   //DEBUG ("Event.ctor: %s", str().c_str());
   //DEBUG ("Event.ctor: e %-16p %s", this, cone.str().c_str());
}

#if 1
template<typename T>
T &Event::data ()
{
   ASSERT (dat);
   return * (T *) dat;
}

template<typename T>
const T &Event::data () const
{
   ASSERT (dat);
   return * (const T *) dat;
}
#endif

const Event *Event::pre_proc () const
{
   return pre_proc (flags.boxfirst);
}
Event *Event::pre_proc ()
{
   return const_cast<Event*> (static_cast<const Event*>(this)->pre_proc ());
}

const Event *Event::pre_proc (bool bf) const
{

   // if this is the first in the box, then we make magic with the addresses ...
   // if the event is the THSTART (root of the tree), the Eventbox::pre already
   // stores a null pointer ;)
   if (bf) return box_below()->pre();
   
   // otherwise it's the precessor in the box
   return this - 1;
}
Event *Event::pre_proc (bool bf)
{
   return const_cast<Event*> (static_cast<const Event*>(this)->pre_proc (bf));
}

const Event *Event::pre_other () const
{
   return node[1].pre;
}
Event *Event::pre_other ()
{
   return const_cast<Event*> (static_cast<const Event*>(this)->pre_other ());
}

bool Event::is_bottom () const
{
   return action.type == ActionType::THSTART and pid () == 0;
}

// Process *Event::process () const in pes/event.cc

unsigned Event::depth_proc () const
{
   return node[0].depth;
}

unsigned Event::depth_other () const
{
   return node[1].depth;
}

Eventbox *Event::box_above () const
{
   return (Eventbox *) (this + 1);
}

// Eventbox *Event::box_below () const in pes/event.cc

bool Event::operator == (const Event &other) const
{
   return this == &other;
}


bool Event::is_predeq_of (const Event *e) const
{
   Event *ee;

   // find the maximal event in [e] for process pid()
   ee = e->cone[pid()];
   //DEBUG ("Event.is_pred_of: this %p pid %u e %p pid %u ee %p",
   //      this, pid(), e, e->pid(), ee);

#ifdef CONFIG_STATS_DETAILED
   Event::counters.causality.calls++;
#endif

   // if there is no event, or it's depth is superior to this.depth, then false
   if (! ee) {
#ifdef CONFIG_STATS_DETAILED
      Event::counters.causality.trivial_null++;
#endif
      return false;
   }

   if (ee == this) {
#ifdef CONFIG_STATS_DETAILED
   Event::counters.causality.trivial_eq++;
#endif
      return true;
   }

   if (node[0].depth >= ee->node[0].depth) {
#ifdef CONFIG_STATS_DETAILED
   Event::counters.causality.trivial_invdep++;
#endif
      return false;
   }

#ifdef CONFIG_STATS_DETAILED
   Event::counters.causality.depth.sample (ee->node[0].depth);
   Event::counters.causality.diff.sample (ee->node[0].depth - node[0].depth);
#endif

   // otherwise, we need to scan the process tree
   return this == ee->node[0].find_pred<0> (node[0].depth);
}

bool Event::is_pred_of (const Event *e) const
{
#ifdef CONFIG_STATS_DETAILED
   Event::counters.causality.calls++;
   Event::counters.causality.trivial_eq++;
#endif
   if (e == this) return false;
#ifdef CONFIG_STATS_DETAILED
   Event::counters.causality.trivial_eq--;
   Event::counters.causality.calls--; // incremented again in is_predeq_of
#endif
   return is_predeq_of (e);
}

bool Event::in_cfl_with (const Event *e) const
{
   bool b;
#ifdef CONFIG_STATS_DETAILED
   Event::counters.conflict.calls_event++;
#endif
   if (this == e) {
      b = false;
#ifdef CONFIG_STATS_DETAILED
      Event::counters.conflict.trivial_eq++;
#endif
   } else {
      b = cone.in_cfl_with (&e->cone);
   }
   //DEBUG ("Event.in_cfl_with: this %p pid %u e %p pid %u ret %d", this, pid(), e, e->pid(), b);
   return b;
}

bool Event::in_cfl_with (const Config &c) const
{
   bool b;

#ifdef CONFIG_STATS_DETAILED
      Event::counters.conflict.calls_conf++;
#endif
   b = cone.in_cfl_with (c);
   //DEBUG ("Event.in_cfl_with: this %p pid %u c %p ret %d", this, pid(), &c, b);
   return b;
}

bool Event::in_icfl_with (const Event *e) const
{
   // two events are in immediate conflict iff both of them are MTXLOCK and
   // their pre_other pointer is equal and their addresses are equal (the reason
   // why we need to check also the addresses is that the pre_other() could be
   // null, and in that case checking the address is necessary, if the
   // pre_other() is not null, then the addresses will be equal)
   return action.type == ActionType::MTXLOCK and
         e->action.type == ActionType::MTXLOCK and
         pre_other() == e->pre_other() and
         action.addr == e->action.addr;
}

bool Event::in_con_with (const Event *e) const
{
   return !is_predeq_of(e) and !e->is_predeq_of(this) and !in_cfl_with(e);
}

void Event::icfls (std::vector<Event*> &v) const
{
   Event *e;

   // only locks have immediate conflicts
   if (action.type != ActionType::MTXLOCK) return;

   // if the event has depth 0, we have to use the circular-list hack to get to
   // the 'other roots' of the forest
   if (node[1].depth == 0)
   {
      for (e = (Event*) node[1].skiptab; e != this;
            e = (Event*) e->node[1].skiptab)
      {
         if (e->process() != process()) v.push_back (e);
      }
      return;
   }

   // for non-root events, the parent's post contains a superset of my immediate
   // conflicts
   for (Event *e : node[1].pre->node[1].post)
   {
      if (e->process() != process()) v.push_back (e);
   }
   return;
}

unsigned Event::icfl_count () const
{
   std::vector<Event*> v;
   icfls (v);
   return v.size();
}

size_t Event::pointed_memory_size () const
{
   return
      MultiNode::pointed_memory_size() +
      (dat ? dat->pointed_memory_size() : 0) +
      cone.pointed_memory_size ();
}
