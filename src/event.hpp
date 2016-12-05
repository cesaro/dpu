
/// THSTART(), creat is the corresponding THCREAT (or null for p0)
inline Event::Event (Event *creat)
:  _pre_other (creat),
   flags ({.boxfirst = 1, .boxlast = 0, .inc = 0}),
   action ({.type = ActionType::THSTART}),
   redbox (),
   vclock(),
   color (0),
   post ()
{
   ASSERT (action.type == ActionType::THSTART);
   ASSERT (pre_proc() == 0);
   ASSERT (pre_other() == creat);
   ASSERT (!creat or creat->action.type == ActionType::THCREAT);

   DEBUG ("Event.ctor: this %p pid %u action %s pre-proc %p pre-other %p",
         this, pid(), action_type_str (action.type), pre_proc(), creat);

   if (creat) creat->post_add (this);
   if (creat == nullptr)
      vclock.add_clock(0,0);
   else
   {
      vclock = creat->vclock;
      vclock.add_clock(pid(),0);
   }
}

/// THCREAT(tid) or THEXIT(), one predecessor (in the process)
inline Event::Event (Action ac) :
   _pre_other (0),
   flags ({.boxfirst = 0, .boxlast = 0, .inc = 0}),
   action (ac),
   redbox (),
   vclock(pre_proc()->vclock),
   color (0),
   post ()
{
   ASSERT (pre_proc() != 0);
   ASSERT (pre_other() == 0);

   DEBUG ("Event.ctor: this %p pid %u action %s pre-proc %p pre-other %p",
         this, pid(), action_type_str (action.type), pre_proc(), pre_other());
   
   pre_proc()->post_add (this);
   vclock.inc_clock(pid());
}

/// THJOIN(tid), MTXLOCK(addr), MTXUNLK(addr), two predecessors (process, memory/exit)
inline Event::Event (Action ac, Event *m) :
   _pre_other (m),
   flags ({.boxfirst = 0, .boxlast = 0, .inc = 0}),
   action (ac),
   redbox (),
   vclock (pre_proc()->vclock),
   color (0),
   post ()
{
   // m could be null (eg, first lock of an execution)
   ASSERT (pre_proc() != 0);
   ASSERT (pre_other() == m);

   DEBUG ("Event.ctor: this %p pid %u action %s pre-proc %p pre-other %p",
         this, pid(), action_type_str (action.type), pre_proc(), pre_other());

   pre_proc()->post_add (this);
   if (m and pre_proc() != m) m->post_add (this);
   vclock.inc_clock(pid());
}

inline void Event::post_add (Event * succ)
{
   post.push_back (succ);
}

inline unsigned Event::pid () const
{
   return Unfolding::ptr2pid (this);
}

inline Event *Event::pre_proc ()
{
   ASSERT (action.type != ActionType::THSTART or flags.boxfirst);

   // if this is the first in the box, then make magic with the addresses ...
   // if the vent is the THSTART (root of the tree), the EventBox::pre already
   // stores a null pointer ;)
   if (flags.boxfirst) return box_below()->pre();
   
   // otherwise it's the precessor in the box
   return this - 1;
}

inline Event *Event::pre_other ()
{
   return _pre_other;
}

inline bool Event::is_bottom ()
{
   return action.type == ActionType::THSTART and pid () == 0;
}

inline Process *Event::proc ()
{
   return Unfolding::ptr2proc (this);
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

