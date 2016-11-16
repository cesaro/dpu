
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

