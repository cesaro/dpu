/// THSTART(), creat is the corresponding THCREAT (or null for p0)
inline Event::Event (Event *creat) :
   MultiNode(nullptr),
   _pre_other (creat),
   flags ({.boxfirst = 1, .boxlast = 0, .inc = 0}),
   action ({.type = ActionType::THSTART}),
   redbox (),
//   vclock(0,0),
   color (0),
   post (),
   cut (creat ? Cut (creat->cut, this) : Cut (pid() + 1, this))
{
   ASSERT (action.type == ActionType::THSTART);
   ASSERT (pre_proc() == 0);
   ASSERT (pre_other() == creat);
   ASSERT (!creat or creat->action.type == ActionType::THCREAT);

   DEBUG ("Event.ctor: this %p pid %u action %s pre-proc %p pre-other %p "
         "bf 1 cut %s",
         this, pid(), action_type_str (action.type), pre_proc(), creat,
         cut.str().c_str());

   if (creat) creat->post_add (this);

//   if (creat == nullptr)
//      vclock.add_clock(0,0);
//   else
//   {
//      vclock = creat->vclock;
//      vclock.add_clock(pid(),0);
//   }
}

/// THCREAT(tid) or THEXIT(), one predecessor (in the process)
inline Event::Event (Action ac, bool bf) :
   MultiNode(pre_proc(bf)),
   _pre_other (0),
   flags ({.boxfirst = bf, .boxlast = 0, .inc = 0}),
   action (ac),
   redbox (),
//   vclock(pre_proc()->vclock),
   color (0),
   post (),
   cut(pre_proc()->cut, this)
{
   ASSERT (pre_proc() != 0);
   ASSERT (pre_other() == 0);

   DEBUG ("Event.ctor: this %p pid %u action %s pre-proc %p pre-other %p "
         "bf %d cut %s",
         this, pid(), action_type_str (action.type), pre_proc(), pre_other(),
         bf, cut.str().c_str());
   
   pre_proc()->post_add (this);
//   vclock.inc_clock(pid()); // wrong because pid() is not the index of proc in the vector
}

/// THJOIN(tid), MTXLOCK(addr), MTXUNLK(addr), two predecessors (process, memory/exit)
inline Event::Event (Action ac, Event *m, bool bf) :
   MultiNode(pre_proc(bf), m),
   _pre_other (m),
   flags ({.boxfirst = bf, .boxlast = 0, .inc = 0}),
   action (ac),
   redbox (),
   // FIXME -- ???
   // here, pre_proc is different from the one passed to unf.event(ac, p, m)
//   vclock (m ? pre_proc()->vclock + m->vclock : pre_proc()->vclock),
   color (0),
   post (),
   cut(m ? Cut(pre_proc()->cut, m->cut, this) : Cut (pre_proc()->cut, this))
{
   // m could be null (eg, first lock of an execution)
   ASSERT (pre_proc() != 0);
   ASSERT (pre_other() == m);

   DEBUG ("Event.ctor: this %p pid %u action %s pre-proc %p pre-other %p "
         "bf %d cut %s",
         this, pid(), action_type_str (action.type), pre_proc(), pre_other(),
         bf, cut.str().c_str());

   // update postsets and clocks
   pre_proc()->post_add (this);
   if (m)
   {
      if (pre_proc() != m) m->post_add (this);
      // FIXME - I commented out next line, as the clock should be correctly
      // constructed by now. Cesar
      //vclock = vclock + m->vclock;
   }
//   vclock.inc_clock (pid());
}

inline void Event::post_add (Event * succ)
{
   post.push_back (succ);
}

inline unsigned Event::pid () const
{
   return Unfolding::ptr2pid (this);
}
inline const Event *Event::pre_proc () const
{
   ASSERT (action.type != ActionType::THSTART or flags.boxfirst);
   return pre_proc (flags.boxfirst);
}
inline Event *Event::pre_proc ()
{
   return const_cast<Event*> (static_cast<const Event*>(this)->pre_proc ());
}

inline const Event *Event::pre_proc (bool bf) const
{

   // if this is the first in the box, then make magic with the addresses ...
   // if the vent is the THSTART (root of the tree), the EventBox::pre already
   // stores a null pointer ;)
   if (bf) return box_below()->pre();
   
   // otherwise it's the precessor in the box
   return this - 1;
}
inline Event *Event::pre_proc (bool bf)
{
   return const_cast<Event*> (static_cast<const Event*>(this)->pre_proc (bf));
}

inline const Event *Event::pre_other () const
{
   return _pre_other;
}
inline Event *Event::pre_other ()
{
   return const_cast<Event*> (static_cast<const Event*>(this)->pre_other ());
}

inline bool Event::is_bottom () const
{
   return action.type == ActionType::THSTART and pid () == 0;
}

inline Process *Event::proc () const
{
   return Unfolding::ptr2proc (this);
}

inline unsigned Event::depth_proc () const
{
   return node[0].depth;
}

inline EventBox *Event::box_above () const
{
   return (EventBox *) (this + 1);
}
inline EventBox *Event::box_below () const
{
   return ((EventBox *) this) - 1;
}

inline bool Event::operator == (const Event &other) const
{
   return this == &other;
}
/*
 *
 */
template <int i>
inline bool Event:: is_pred_in_the_same_tree_of (const Event *e) const
{
   Event *ee;
   if (this == e) return true;

   if (node[i].depth > e->node[i].depth)
   {
//      DEBUG("node[i] cfl or is a successor of e");
      return false;
   }

   if (node[i].depth == e->node[i].depth)
   {
      DEBUG("Two nodes have the same depth");
      if (this == e)
         return true;
      else
         return false;
   }

   ASSERT(node[i].depth < e->node[i].depth);

   ee = &e->node[i].find_pred<i>(node[i].depth);
   DEBUG("%d", ee->idx);
   DEBUG("%d", this->idx);

   if (ee == this)
   {
      DEBUG("not cfl here");
      return true;
   }

   return false;
}

inline bool Event::is_pred_of (const Event *e) const
{
//   DEBUG("%p cut.nrp: %d",this, cut.num_procs());
//   DEBUG("%p cut.nrp: %d",e, e->cut.num_procs());
//   DEBUG("e->pid: %d",e->pid());
   /// e1 is not a pred of e2 if e1 = e2
   /// e.cut having fewer elements than this->cut means there's no way for it to be this's successor.
   if (e->cut.num_procs() < cut.num_procs())
   {
//      DEBUG("e < this");
      return false;
   }

   ASSERT(cut.num_procs() <= e->cut.num_procs());

   if (pid() == e->pid())
   {
//      DEBUG("Two events in the same process");
      return is_pred_in_the_same_tree_of<0>(e);
   }


//   DEBUG("this->addr: %p",action.addr);
//   DEBUG("e->addr: %p",e->action.addr);

   if ((action.addr == e->action.addr) and (action.addr))
   {
//      DEBUG("Touch the same addr");
      return is_pred_in_the_same_tree_of<1>(e);
   }

   /// here, cut.num_procs() <= e->cut.num_procs(). It is enough to loop cut.num_procs() times

   for (int i = 0; i < cut.num_procs(); i++)
   {
      DEBUG("Proc %d:", i);
      if (!cut[i]->is_pred_in_the_same_tree_of<0>(e->cut[i]))
         return false;
      DEBUG("true");
   }
   return true;
}
/*
 * - two events in the same tree can only be in causality or conflict. So, if they are not in causality, they must be in conflict.
 * - Just consider the case where two events are in 2 different processes and touch different variable (addr)
 */
inline bool Event::in_cfl_with (const Event *e)
{
   if (e == nullptr) return false;
   if (this == e) return false;

   if (pid() == e->pid())
   {
      DEBUG("Same process tree");
      return (!is_pred_in_the_same_tree_of<0>(e) and !e->is_pred_in_the_same_tree_of<0>(this));
   }

   if ((action.addr == e->action.addr) and (action.addr))
   {
      DEBUG("Same addr tree");
      return (!is_pred_in_the_same_tree_of<1>(e) and !e->is_pred_in_the_same_tree_of<1>(this));
   }

   int nrp = (cut.num_procs() > e->cut.num_procs()) ? e->cut.num_procs() : cut.num_procs();
//   DEBUG("nrp: %d", nrp);

   DEBUG("Different processes");
   for (int i = 0; i < nrp; i++)
   {
      if (!cut[i]->is_pred_in_the_same_tree_of<0>(e->cut[i]) and !e->cut[i]->is_pred_in_the_same_tree_of<0>(cut[i]))
         return true; // there is a pair in conflict => 2 events are in cfl consequently
   }
   return false;
}



