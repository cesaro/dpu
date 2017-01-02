
/// THSTART(), creat is the corresponding THCREAT (or null for p0)
Event::Event (Event *creat) :
   MultiNode(nullptr, creat),
   flags ({.boxfirst = 1, .boxlast = 0, .crb = 0, .ind = 0}),
   action ({.type = ActionType::THSTART}),
   redbox (),
   color (0),
   cone (creat ? Primecon (creat->cone, this) : Primecon (pid() + 1, this)),
   depth (creat ? creat->depth + 1 : 0)
{
   ASSERT (action.type == ActionType::THSTART);
   ASSERT (pre_proc() == 0);
   ASSERT (pre_other() == creat);
   ASSERT (!creat or creat->action.type == ActionType::THCREAT);

   //DEBUG ("Event.ctor: %s", str().c_str());
   //DEBUG ("Event.ctor: e %-16p %s", this, cone.str().c_str());
}

/// THCREAT(tid) or THEXIT(), one predecessor (in the process)
Event::Event (Action ac, bool bf) :
   MultiNode(pre_proc(bf), nullptr),
   flags ({.boxfirst = bf, .boxlast = 0, .crb = 0, .ind = 0}),
   action (ac),
   redbox (),
   color (0),
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
   redbox (),
   color (0),
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

const Event *Event::pre_proc () const
{
   ASSERT (action.type != ActionType::THSTART or flags.boxfirst);
   return pre_proc (flags.boxfirst);
}
Event *Event::pre_proc ()
{
   return const_cast<Event*> (static_cast<const Event*>(this)->pre_proc ());
}

const Event *Event::pre_proc (bool bf) const
{

   // if this is the first in the box, then make magic with the addresses ...
   // if the vent is the THSTART (root of the tree), the Eventbox::pre already
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

// Process *Event::proc () const in pes/event.cc

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

//std::vector<Event *> Event::get_local_config()
//{
//   DEBUG("Event.local_config");
//   Event * next;
//   std::vector<Event *> lc;
//   //DEBUG("Proc_maxevt.size = %d", proc_maxevt.size());
//   DEBUG("cone.num_proc: %d", cone.num_procs());
//
//   for (unsigned i = 0; i < cone.num_procs(); i++)
//   {
//      DEBUG("cone[%d]: %p", i, cone[i]);
//      next = cone[i];
//
//      while (next->action.type != ActionType::THSTART)
//      {
//         lc.push_back(next);
//         next = next->pre_proc();
//         DEBUG("next->idx: %p", next);
//      }
//      // add THSTART event
//      lc.push_back(next);
//   }
//
//   DEBUG("LC inside the function");
//   for (unsigned j = 0; j < lc.size(); j++)
//      DEBUG_("%p ", lc[j]);
//
//   return lc;
//}
/*
 *
 */
//template <int i>
//bool Event:: is_pred_in_the_same_tree_of (const Event *e) const
//{
//   const Event *ee;
//   if (this == e) return true;
//
//   if (node[i].depth > e->node[i].depth)
//   {
////      DEBUG("node[i] cfl or is a successor of e");
//      return false;
//   }
//
//   if (node[i].depth == e->node[i].depth)
//   {
//      DEBUG("Two nodes have the same depth");
//      if (this == e)
//         return true;
//      else
//         return false;
//   }
//
//   ASSERT(node[i].depth < e->node[i].depth);
//
//   ee = e->node[i].find_pred<i>(node[i].depth);
//   DEBUG("%p", ee);
//   DEBUG("%d", this);
//
//   if (ee == this)
//   {
//      DEBUG("not cfl here");
//      return true;
//   }
//
//   return false;
//}

bool Event::is_predeq_of (const Event *e) const
{
   Event *ee;

   // find the maximal event in [e] for process pid()
   ee = e->cone[pid()];
   //DEBUG ("Event.is_pred_of: this %p pid %u e %p pid %u ee %p",
   //      this, pid(), e, e->pid(), ee);

   // if there is no event, or it's depth is superior to this.depth, then false
   if (! ee) return false;
   if (ee == this) return true;
   if (node[0].depth >= ee->node[0].depth) return false;

   // otherwise, we need to scan the process tree
   return this == ee->node[0].find_pred<0> (node[0].depth);
}

bool Event::is_pred_of (const Event *e) const
{
   if (e == this) return false;
   return is_predeq_of (e);
}

bool Event::in_cfl_with (const Event *e) const
{
   bool b = this == e ? false : cone.in_cfl_with (&e->cone);
   //DEBUG ("Event.in_cfl_with: this %p pid %u e %p pid %u ret %d", this, pid(), e, e->pid(), b);
   return b;
}

bool Event::in_cfl_with (const Config &c) const
{
   bool b = cone.in_cfl_with (c);
   //DEBUG ("Event.in_cfl_with: this %p pid %u c %p ret %d", this, pid(), &c, b);
   return b;
}

bool Event::in_icfl_with (const Event *e) const
{
   // two events are in immediate conflict iff both of them are MTXLOCK and
   // their pre_other pointer is equal
   return action.type == ActionType::MTXLOCK and
         e->action.type == ActionType::MTXLOCK and
         pre_other() == e->pre_other();
}

std::vector<Event*> Event::icfls () const
{
   std::vector<Event*> v;
   Event *e;

   // only locks have immediate conflicts
   if (action.type != ActionType::MTXLOCK) return v;

   // if the event has depth 0, we have to use the circular-list hack to get to
   // the 'other roots' of the forest
   if (node[1].depth == 0)
   {
      for (e = (Event*) node[1].skiptab; e != this;
            e = (Event*) e->node[1].skiptab)
      {
         if (e->proc() != proc()) v.push_back (e);
      }
      return v;
   }

   // for non-root events, the parent's post contains a superset of my immediate
   // conflicts
   for (Event *e : node[1].pre->node[1].post)
   {
      if (e->proc() != proc()) v.push_back (e);
   }
   return v;
}