
/// THSTART(), creat is the corresponding THCREAT (or null for p0)
inline Event::Event (Event *creat) :
   _pre_other (creat),
   flags ({.boxfirst = 1, .boxlast = 0, .inc = 0}),
   action ({.type = ActionType::THSTART}),
   redbox (),
   vclock (), // FIXME what to put here?
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
}

/// THCREAT(tid) or THEXIT(), one predecessor (in the process)
inline Event::Event (Action ac) :
   _pre_other (0),
   flags ({.boxfirst = 0, .boxlast = 0, .inc = 0}),
   action (ac),
   redbox (),
   vclock (), // FIXME what to put here?
   color (0),
   post ()
{
   ASSERT (pre_proc() != 0);
   ASSERT (pre_other() == 0);

   DEBUG ("Event.ctor: this %p pid %u action %s pre-proc %p pre-other %p",
         this, pid(), action_type_str (action.type), pre_proc(), pre_other());
   
   pre_proc()->post_add (this);
}

/// THJOIN(tid), MTXLOCK(addr), MTXUNLK(addr), two predecessors (process, memory/exit)
inline Event::Event (Action ac, Event *m) :
   _pre_other (m),
   flags ({.boxfirst = 0, .boxlast = 0, .inc = 0}),
   action (ac),
   redbox (),
   vclock (), // FIXME what to put here?
   color (0),
   post ()
{
   // m could be null (eg, first lock of an execution)
   ASSERT (pre_proc() != 0);
   ASSERT (pre_other() == m);

   DEBUG ("Event.ctor: this %p pid %u action %s pre-proc %p pre-other %p",
         this, pid(), action_type_str (action.type), pre_proc(), pre_other());

   pre_proc()->post_add (this);
   if (m) m->post_add (this);
}

inline void Event::post_add (Event * succ)
{
   post.push_back (succ);
}

inline unsigned Event::pid ()
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

inline Unfolding::Unfolding () :
   nrp (0)
{
   static_assert (MAX_PROC >= 2, "At lest 2 processes");
   static_assert (PROC_SIZE >= sizeof (Process), "Proccess size too small");
   static_assert (PROC_SIZE >= 4 << 10, "Process size too small"); // 4k
   static_assert (ALIGN >= 16, "Process size too small");

   DEBUG ("Unfolding.ctor: this %p max-proc %zu proc-size %zu align %zu%s size %zu%s",
         this, MAX_PROC, PROC_SIZE,
         UNITS_SIZE (ALIGN),
         UNITS_UNIT (ALIGN),
         UNITS_SIZE (MAX_PROC * PROC_SIZE),
         UNITS_UNIT (MAX_PROC * PROC_SIZE));

   // allocate suitably aligned memory for the processes
   DEBUG ("Unfolding.ctor: allocating memory and constructing bottom ...");
   procs = (char *) aligned_alloc (ALIGN, MAX_PROC * PROC_SIZE);
   if (procs == 0)
      throw std::bad_alloc ();

   // create the first process and the bottom event in it; creat is NULL
   new_proc (0);

   DEBUG ("Unfolding.ctor: done, procs %p", procs);
}

inline Process *Unfolding::proc (unsigned p) const
{
   return (Process *) (procs + p * PROC_SIZE);
}

/// THSTART() for process != 0, creat is the corresponding THCREAT; this will
/// create a new process
inline Event *Unfolding::event (Event *creat)
{
   // FIXME check if the event exists already
   if (creat == 0) return proc(0)->first_event();

   // for the time being
   ASSERT (creat->pid() == 0);

   Process *p = new_proc (creat);
   ASSERT (p);
   ASSERT (p->pid() == 1); // for the time being :)
   Event *e = p->first_event();
   ASSERT (e->flags.boxfirst);
   return e;
}

/// THCREAT(tid) or THEXIT(), one predecessor (in the process)
inline Event *Unfolding::event (Action ac, Event *p)
{
   ASSERT (p);

   // FIXME check if the event exists already
   if (false) return 0;

   return p->proc()->add_event_1p (ac, p);
}

/// THJOIN(tid), MTXLOCK(addr), MTXUNLK(addr), two predecessors (process, memory/exit)
inline Event *Unfolding::event (Action ac, Event *p, Event *m)
{
   ASSERT (p);

   // FIXME check if the event exists already
   if (false) return 0;

   return p->proc()->add_event_2p (ac, p, m);
}

inline Process *Unfolding::new_proc (Event *creat)
{
   void *p;
   if (nrp >= MAX_PROC)
      throw std::range_error ("Maximum number of processes reached, cannot create more");

   // construct the process object, increment nrp
   p = (void*) (procs + nrp * PROC_SIZE);
   nrp++;
   return new (p) Process (creat);
}

inline Process::Process (Event *creat)
{
   DEBUG ("Process.ctor: this %p pid %d sizeof %d", this, pid(), sizeof (Process));

   // construct the first event box
   EventBox *b = first_box ();
   new (b) EventBox (0);

   // construct a THSTART event in the first box
   Event *e = b->event_above ();
   new (e) Event (creat);
   e->flags.boxfirst = 1;

   // initialize the pointer to the last event created
   last = b->event_above();
}

inline EventIt Process::begin ()
{
   return EventIt (first_event());
}
inline EventIt Process::end ()
{
   return EventIt (last + 1);
}

inline unsigned Process::pid () const
{
   return Unfolding::ptr2pid (this);
}
inline size_t Process::offset (void *ptr) const
{
   return ((size_t) ptr) - (size_t) this;
}

inline Event *Process::first_event () const
{
   return first_box()->event_above();
}

inline EventBox *Process::first_box () const
{
   return ((EventBox *) (this + 1));
}

/// THCREAT(tid), THEXIT(), one predecessor (in the process)
inline Event *Process::add_event_1p (Action ac, Event *p)
{
   Event *e;

   ASSERT (last);
   ASSERT (pid() == last->pid());
   ASSERT (! last->flags.boxlast);

   // check that we have enough memory
   // last points to the last event
   // last+1 is the base address of the new one; last+3 is the upper limit of
   // one event + one box
   static_assert (sizeof (Event) >= sizeof (EventBox), "Event should be larger than EventBox");
   if (offset (last + 3) >= Unfolding::PROC_SIZE)
   {
      throw std::range_error (fmt
            ("Process %d: failure to add new events: out of memory", pid ()));
   }

   if (p == last)
   {
      // add one event at the end of the pool
      e = last + 1;
      new (e) Event (ac);
   }
   else
   {
      // create one box and add the event
      last->flags.boxlast = 1;
      EventBox *b = new (last + 1) EventBox (p);
      e = b->event_above ();
      new (e) Event (ac);
      e->flags.boxfirst = 1;
   }

   last = e;
   return e;
}

/// THJOIN(tid), MTXLOCK(addr), MTXUNLK(addr), two predecessors (process, memory/exit)
inline Event * Process::add_event_2p (Action ac, Event *p, Event *m)
{
   Event *e;

   ASSERT (last);
   ASSERT (pid() == last->pid());
   ASSERT (! last->flags.boxlast);

   // check that we have enough memory
   // last points to the last event
   // last+1 is the base address of the new one; last+3 is the upper limit of
   // one event + one box
   static_assert (sizeof (Event) >= sizeof (EventBox), "Event should be larger than EventBox");
   if (offset (last + 3) >= Unfolding::PROC_SIZE)
   {
      throw std::range_error (fmt
            ("Process %d: failure to add new events: out of memory", pid ()));
   }

   if (p == last)
   {
      // add one event at the end of the pool
      e = last + 1;
      new (e) Event (ac, m);
   }
   else
   {
      // create one box and add the event
      last->flags.boxlast = 1;
      EventBox *b = new (last + 1) EventBox (p);
      e = b->event_above ();
      new (e) Event (ac, m);
      e->flags.boxfirst = 1;
   }

   last = e;
   return e;
}

inline EventBox::EventBox (Event *pre) :
   _pre (pre)
{
   DEBUG ("EventBox.ctor: this %p pid %u pre %p sizeof %zu",
         this, pid(), _pre, sizeof (EventBox));
}

inline Event *EventBox::event_above () const
{
   return (Event*) (this + 1);
}
inline Event *EventBox::event_below () const
{
   return ((Event *) this) - 1;
}

Event *EventBox::pre () const
{
   return _pre;
}

inline unsigned EventBox::pid () const
{
   return Unfolding::ptr2pid (this);
}

