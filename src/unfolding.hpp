
inline Unfolding::Unfolding () :
   nrp (0),
   color (1)
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

inline Unfolding::Unfolding (const Unfolding &&other)
{
   // FIXME - to be implemented
   ASSERT (0);
}

Unfolding::~Unfolding ()
{
   DEBUG ("Unfolding.dtor: this %p", this);
   free (procs);
}

inline Process *Unfolding::proc (unsigned p) const
{
   return (Process *) (procs + p * PROC_SIZE);
}

/// THSTART(), creat is the corresponding THCREAT; this will
/// create a new process
inline Event *Unfolding::event (Event *creat)
{
   // if creat is null, we return bottom, already present in the unfolding
   if (creat == 0)
   {
      ASSERT (proc(0)->first_event());
      ASSERT (proc(0)->first_event()->action.type == ActionType::THSTART);
      return proc(0)->first_event();
   }

   // otherwise we are searching for the root event in a process that we might
   // or might not have already created; if creat has a causal successor in the
   // node[1] tree, then it can be the only one and it must be our event
   if (creat->node[1].post.size())
   {
      ASSERT (creat->node[1].post.size() == 1);
      ASSERT (creat->node[1].post[0]->action.type == ActionType::THSTART);
      ASSERT (creat->pid() < creat->node[1].post[0]->pid());
      return creat->node[1].post[0];
   }

   // otherwise we need to create a new process and return its root
   Process *p = new_proc (creat);
   ASSERT (p);
   Event *e = p->first_event();
   ASSERT (e->flags.boxfirst);
   return e;
}

/// THCREAT(tid) or THEXIT(), one predecessor (in the process)
inline Event *Unfolding::event (Action ac, Event *p)
{
   Event *e;

   ASSERT (p);
   ASSERT (ac.type == ActionType::THCREAT or ac.type == ActionType::THEXIT);

   // if the event already exist, we return it
   e = find1 (&ac, p);
   if (e) return e;

   // otherwise we create it
   return p->proc()->add_event_1p (ac, p);
}

/// THJOIN(tid), MTXLOCK(addr), MTXUNLK(addr), two predecessors (process, memory/exit)
inline Event *Unfolding::event (Action ac, Event *p, Event *m)
{
   Event *e;

   // checking that this method was correctly invoked
   ASSERT (p);
   if (ac.type == ActionType::THJOIN) ASSERT (m and m->action.type == ActionType::THEXIT);
   if (ac.type == ActionType::MTXLOCK)
   {
      ASSERT (ac.addr > nrp);
      ASSERT (!m or m->action.type == ActionType::MTXUNLK);
   }
   if (ac.type == ActionType::MTXUNLK)
   {
      ASSERT (ac.addr > nrp);
      ASSERT (m and m->action.type == ActionType::MTXLOCK);
   }

   // if the event already exist, we return it
   e = find2 (&ac, p, m);
   if (e) return e;

   // otherwise we create it
   e = p->proc()->add_event_2p (ac, p, m);

   // hack to be able to list in Event::icfl() the root events of an address
   // tree
   if (ac.type == ActionType::MTXLOCK and !m)
   {
      ASSERT (! e->node[1].skiptab);
      ASSERT (! e->node[1].depth)
      ASSERT (! e->node[1].pre)
      auto it = lockroots.find (ac.addr);
      if (it == lockroots.end())
      {
         e->node[1].skiptab = (Event **) e; // init the circular list
         lockroots[ac.addr] = e;
      }
      else
      {
         e->node[1].skiptab = it->second->node[1].skiptab; // add e to circular list
         it->second->node[1].skiptab = (Event **) e;
      }
   }

   return e;
}

inline Process *Unfolding::new_proc (Event *creat)
{
   void *p;
   if (nrp >= MAX_PROC)
      throw std::range_error ("Maximum number of processes reached, cannot create more");

   // construct the process object, increment nrp
   p = (void*) (procs + nrp * PROC_SIZE);
   nrp++;
   return new (p) Process (creat, *this);
}

inline Event *Unfolding::find1 (Action *ac, Event *p)
{
   ASSERT (p);
   ASSERT (p->node[0].post.size() <= 1);

   if (p->node[0].post.size() == 0) return nullptr;
   return p->node[0].post[0];

#if 0
   // Old implementation (remove):

   unsigned pid;

   // p should have 0 or 1 "mathematical" causal successor in the same process,
   // and its action should be ac; but since we store non-immediate causal
   // succesors in Event::post, we need to filter out using ac->type

   pid = p->pid();
#ifdef CONFIG_DEBUG
   int count = 0;
   for (auto e : p->post)
      if (e->pid () == pid and e->action.type == ac->type)
         count++;
   ASSERT (count <= 1);
#endif
   for (auto e : p->post)
   {
      if (e->pid () == pid and e->action.type == ac->type)
         return e;
   }

   // it does not exist
   return nullptr;
#endif
}

inline Event *Unfolding::find2 (Action *ac, Event *p, Event *m)
{
#ifdef CONFIG_DEBUG
   // exactly one event with preset {p,m} ...
   int count = 0;
   for (Event *e : p->node[0].post)
   {
      if (e->pre_other() == m) count++;
   }
   ASSERT (count <= 1);
#endif

   // FIXME - we could choose here p or m depending on the size of the post
   for (Event *e : p->node[0].post)
   {
      if (e->pre_other() == m)
      {
         // ... and that event is action *ac
         ASSERT (e->action == *ac);
         return e;
      }
   }
   return nullptr;
}

inline unsigned Unfolding::get_fresh_color ()
{
   color++;
   return color;
}
