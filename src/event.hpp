/// THSTART(), creat is the corresponding THCREAT (or null for p0)
inline Event::Event (Event *creat) :
   MultiNode(nullptr),
   _pre_other (creat),
   flags ({.boxfirst = 1, .boxlast = 0, .inc = 0}),
   action ({.type = ActionType::THSTART}),
   redbox (),
   vclock(0,0),
   color (0),
   post ()
   //maxproc(creat ? create->maxproc : clear())
{
   ASSERT (action.type == ActionType::THSTART);
   ASSERT (pre_proc() == 0);
   ASSERT (pre_other() == creat);
   ASSERT (!creat or creat->action.type == ActionType::THCREAT);

   DEBUG ("Event.ctor: this %p pid %u action %s pre-proc %p pre-other %p bf 1",
         this, pid(), action_type_str (action.type), pre_proc(), creat);

   if (creat) creat->post_add (this);

   if (creat == nullptr)
      vclock.add_clock(0,0);
   else
   {
      vclock = creat->vclock;
      vclock.add_clock(pid(),0);
   }


   if (maxproc.size() < pid())
   {
      for (int i = maxproc.size(); i < pid()- 1; i++)
         maxproc.push_back(nullptr);

      maxproc.push_back(this);
   }
   else
   {
      maxproc[pid()] = this;
   }
}

/// THCREAT(tid) or THEXIT(), one predecessor (in the process)
inline Event::Event (Action ac, bool bf) :
   MultiNode(pre_proc(bf)),
   _pre_other (0),
   flags ({.boxfirst = bf, .boxlast = 0, .inc = 0}),
   action (ac),
   redbox (),
   vclock(pre_proc()->vclock),
   color (0),
   post (),
   maxproc(pre_proc()->maxproc)
{
   ASSERT (pre_proc() != 0);
   ASSERT (pre_other() == 0);

   DEBUG ("Event.ctor: this %p pid %u action %s pre-proc %p pre-other %p bf %d",
         this, pid(), action_type_str (action.type), pre_proc(), pre_other(), bf);
   
   pre_proc()->post_add (this);
   vclock.inc_clock(pid()); // wrong because pid() is not the index of proc in the vector

   maxproc[pid()] = this;
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
   vclock (m ? pre_proc()->vclock + m->vclock : pre_proc()->vclock),
   color (0),
   post (),
   maxproc(pre_proc()->maxproc)
{
   // m could be null (eg, first lock of an execution)
   ASSERT (pre_proc() != 0);
   ASSERT (pre_other() == m);

//   DEBUG("pre_proc %p", pre_proc());
   DEBUG ("Event.ctor: this %p pid %u action %s pre-proc %p pre-other %p bf %d",
         this, pid(), action_type_str (action.type), pre_proc(), pre_other(), bf);

   // update postsets and clocks
   pre_proc()->post_add (this);
   if (m)
   {
      if (pre_proc() != m) m->post_add (this);
      // FIXME - I commented out next line, as the clock should be correctly
      // constructed by now. Cesar
      //vclock = vclock + m->vclock;
   }
   vclock.inc_clock (pid());

   if (m)
   {
      if (maxproc.size() >= m->maxproc.size())
      {
         for (int i = 0; i < m->maxproc.size(); i++)
            if (maxproc[i]->is_pred_same_tree_of(m->maxproc[i])) // is_pred in the same tree of process
               maxproc[i] = m-> maxproc[i];
      }
      else
      {
         for (int i = 0; i < maxproc.size(); i++)
            if (maxproc[i]->is_pred_same_tree_of(m->maxproc[i]))
               maxproc[i] = m-> maxproc[i];
         for (int i = maxproc.size(); i < m->maxproc.size(); i++)
            maxproc.push_back(m->maxproc[i]);
      }

   }

   maxproc[pid()] = this;
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
   return pre_proc (flags.boxfirst);
}

inline Event *Event::pre_proc (bool bf)
{

   // if this is the first in the box, then make magic with the addresses ...
   // if the vent is the THSTART (root of the tree), the EventBox::pre already
   // stores a null pointer ;)
   if (bf) return box_below()->pre();
   
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

inline bool Event::is_pred_same_tree_of (const Event *e) const
{
   return true;
}

/// need to make sure that this->node[i].depth < e->node[i].depth
inline bool Event::is_pred_of (const Event *e) const
{
   Event *ee;
   DEBUG("this->pid: %d",this->pid());
   DEBUG("e->pid: %d",e->pid());


   if ((this->pid() == e->pid()) and (this->node[0].depth < e->node[0].depth))
   {
      ee = &e->node[0].find_pred<0>(this->node[0].depth);
//      DEBUG("ee: %p",ee);
//      DEBUG("this: %p",this);
      if (ee == this)
         return true;
   }

//   DEBUG("this->depth: %d",this->node[1].depth);
//   DEBUG("e->depth: %d",e->node[1].depth);

   DEBUG("this->addr: %p",this->action.addr);
   DEBUG("e->addr: %p",e->action.addr);
  // if ((this->action.addr) and (e->action.addr) and (this->action.addr == e->action.addr))
    if (this->action.addr == e->action.addr)
   {
      ee = &e->node[1].find_pred<1>(this->node[1].depth);
      if (ee == this)
         return true;
   }

   return false;
}



