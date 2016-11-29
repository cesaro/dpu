
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
   // if creat is null, we return bottom, already present in the unfolding
   if (creat == 0)
   {
      ASSERT (proc(0)->first_event());
      ASSERT (proc(0)->first_event()->action.type == ActionType::THSTART);
      return proc(0)->first_event();
   }

   // otherwise we are searching for the root event in a process that we might
   // or might not already created; if creat has a causal successor, then our
   // root event must be in the first position in the post vector
   if (creat->post.size())
   {
      ASSERT (creat->post[0]);
      ASSERT (creat->post[0]->action.type == ActionType::THSTART);
      ASSERT (creat->pid() < creat->post[0]->pid());
      for (int i = 1; i < creat->post.size(); i++)
         ASSERT (creat->post[i]->action.type != ActionType::THSTART);
      return creat->post[0];
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
   if (m == 0)
   {
      ASSERT (ac.type == ActionType::MTXLOCK); // for the first MTXLOCK
      DEBUG("The first LOCK");
   }
   else
   {
      if (ac.type == ActionType::THJOIN) ASSERT (m and m->action.type == ActionType::THEXIT);
      if (ac.type == ActionType::MTXLOCK) ASSERT (!m or m->action.type == ActionType::MTXUNLK);
      if (ac.type == ActionType::MTXUNLK) ASSERT (m or m->action.type == ActionType::MTXLOCK);
   }
   // if the event already exist, we return it
   e = find2 (&ac, p, m);
   if (e) return e;
   // otherwise we create it
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

inline Event *Unfolding::find1 (Action *ac, Event *p)
{
   // p should have 0 or 1 causal successor, and its action should be ac
   ASSERT (p->post.size() == 0 or p->post.size() == 1);
   ASSERT (p->post.size() == 0 or p->post[0]->action == *ac);

   if (p->post.size() == 0) return 0;
   return p->post[0];
}

inline Event *Unfolding::find2 (Action *ac, Event *p, Event *m)
{
#ifdef CONFIG_DEBUG
   // exactly one event with preset {p,m} ...
   int count = 0;
   for (Event *e : p->post)
   {
      if (e->pre_other() == m) count++;
   }
   ASSERT (count <= 1);
#endif

   // FIXME - we could choose here p or m depending on the size of the post
   for (Event *e : p->post)
   {
      if (e->pre_other() == m)
      {
         // ... and that event is action *ac
         ASSERT (e->action == *ac);
         return e;
      }
   }
   return 0;
}
