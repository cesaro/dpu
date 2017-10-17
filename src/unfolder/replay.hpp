
void Replay::extend_from (const Trail &t, unsigned limit)
{
   unsigned pid;
   int count;

   // if there is at least one event, then the first is for pid 0
   count = pid = 0;
   ASSERT (t.size() == 0 or t[0]->pid() == pid);

   for (Event *e : t)
   {
      if (limit == 0) break;
      // on context switches, add a new entry
      if (e->pid() != pid)
      {
         DEBUG ("c15u: trail-to-replay: r%u (#%d) count %d",
               pidmap.get(pid), pid, count);
         push_back ({.tid = (int) pidmap.get(pid), .count = count});
         pid = e->pid ();
         count = 0;
      }
      // on thread creation, add a new entry to the pid map
      if (e->action.type == ActionType::THCREAT)
         pidmap.add (e->action.val, pidmap.get_next_tid());
      count++;
      limit--;
   }
   if (count)
   {
      DEBUG ("c15u: trail-to-replay: r%u (#%d) count %d",
            pidmap.get(pid), pid, count);
      push_back ({(int) pidmap.get(pid), count});
   }
}

void Replay::extend_from (const Cut &c)
{
   static const Cut empty (Unfolding::MAX_PROC);

   extend_from (c, empty);
}

void Replay::extend_from (const Cut &c1, const Cut &c2)
{
   int count, len, j;
   unsigned i, nrp;
   bool progress;
   Event *e, *ee;
   Cut cc (std::min (c1.num_procs(), u.num_procs()));

   // we need to replay every event in c1 that is not in c2

   DEBUG ("c15u: cut-to-replay: cut1 %s cut2 %s",
         c1.str().c_str(), c2.str().c_str());

   // we use an auxiliary cut cc to run forward the events in c1 \setminus c2,
   // to do so, we set up and use the Event's next pointer
   nrp = cc.num_procs();
   //SHOW (nrp, "u");
   for (i = 0; i < nrp; i++)
   {
      // compute len, the number of events that we will run in process i
      e = c1[i];
      if (! e) continue;
      len = e->depth_proc();
      if (c2[i]) len -= (int) c2[i]->depth_proc(); else len++;
      if (len <= 0) continue;
      DEBUG ("c15u: cut-to-replay: pid %d: will replay %d events", i, len);

      // set up the Event's next pointer in order to run forward; the last event
      // that we set is one above c2[i], due to the way we have computed the len
      ee = nullptr;
      for (j = 0; j < len; j++)
      {
         e->next = ee;
         ee = e;
         e = e->pre_proc();
      }

      // we set cc to be a suitable starting state for (c1 \setminus c2), which
      // is "the layer of events one above" the cut of (c1 \cap c2)
      cc[i] = ee;
   }
   DEBUG ("c15u: cut-to-replay: now forward, from cc %s", cc.str().c_str());

   // in a round-robbin fashion, we run each process until we cannot continue in
   // that process; this continues until we are unable to make progress
   progress = true;
   while (progress)
   {
      progress = false;
      for (i = 0; i < nrp; i++)
      {
         // we replay as many events as possible on process i, starting from e
         // INVARIANT: we have not executed e, the loop below relies and
         // maintains this invariant, after the update c[i] = e below
         e = cc[i];
         count = 0;
         for (; e; e = e->next)
         {
            DEBUG ("c15u: cut-to-replay: at pid %d, trying %s", i, e->str().c_str());
            // if event has non-visited causal predecessor, abort this process
            if (e->pre_other()
                  and i != e->pre_other()->pid()
                  and cc[e->pre_other()->pid()]
                  and e->pre_other()->depth_proc() >= cc[e->pre_other()->pid()]->depth_proc())
                     break;
            count++;
            if (e->action.type == ActionType::THCREAT)
               pidmap.add (e->action.val, pidmap.get_next_tid());
         }

         if (count)
         {
            cc[i] = e;
            DEBUG ("c15u: cut-to-replay: r%u (#%u) count %d", pidmap.get(i), i, count);
            push_back ({(int) pidmap.get(i), count});
            progress = true;
         }
      }
   }
}

void Replay::build_from (const Trail &t, const Cut &c, const Cut &j)
{
   // the sequence of number that we need to compute here allows steroids to
   // replay the trail (exacty in that order) followed by the replay of
   // J \setminus C (in any order).

   unsigned lim, i;
   bool first;

   // We map pids in dpu to tids in steroids, the id of a given thread in a
   // replay is defined when the corresponding THCREAT event happens; the
   // communication protocol with steroids establishes that the tid which that
   // thread gets at that moment is equal to 1 + the number of times a THCREAT
   // event has been replayed before that THCREAT. Every time we replay a
   // THCREAT we add one entry to the map. We use the size of the map to keep
   // track of the next fresh tid. The map automagically keeps track of the
   // mapping 0 -> 0. We now reset the map.
   pidmap.clear ();
   ASSERT (pidmap.size() == 1);
   ASSERT (pidmap.get(0) == 0);

   clear();
   extend_from (t);
   lim = size();
   extend_from (j, c);
   finish ();

   // FIXME - we need to improve the TRACE/INFO/DEBUG macros so that we can
   // avoid this ugly code ...
   if (verb_trace)
   {
      TRACE_ ("c15u: explore: replay seq: %s", str(lim).c_str());
      first = true;
      for (i = 0; i < pidmap.size(); i++)
      {
         if (i != pidmap.get(i))
         {
            if (first) TRACE_ (" (pidmap: ");
            first = false;
            TRACE_ ("#%u -> r%u; ", i, pidmap.get(i));
         }
      }
      if (first)
         TRACE ("");
      else
         TRACE (")");
   }
}

void Replay::build_from (const Trail &t, unsigned limit)
{
   clear();
   extend_from (t, limit);
   finish ();
}

void Replay::build_from (const Cut &c)
{
   clear ();
   extend_from (c);
   finish ();
}

void Replay::finish ()
{
   push_back ({-1, -1});
}

std::string Replay::str (unsigned altidx)
{
   std::string s;
   unsigned i;

   i = 0;
   for (auto &e : *this)
   {
      if (i) s += "; ";
      if (i == altidx) s += "** ";
      if (e.tid == -1)
      {
         ASSERT (e.count == -1);
         s += "-1";
      }
      else
      {
         ASSERT (e.tid >= 0 and e.count >= 1);
         s += fmt ("%u %u", e.tid, e.count);
      }
      i++;
   }
   return s;
}

