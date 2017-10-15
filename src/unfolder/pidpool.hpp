
Pidpool::Pidpool (Unfolding &u_) :
   u (u_),
   procs ()
{
   procs.reserve (Unfolding::MAX_PROC);
   new_prochist();
}

unsigned Pidpool::create (const Event *c)
{
   int newpid;
   Prochist *h;

   ASSERT (c);
   ASSERT (c->action.type == ActionType::THCREAT);
   ASSERT (c->pid() < procs.size());

   // get a pointer to the history in this process
   h = &procs[c->pid()];

   // if this THCREAT event had been assigned a pid already in a previous
   // execution, we only need to increment the current depth and note down that
   // c->pid() is assigned also in this execution, and remove it from the joined
   // set, as it could be there
   if (c->action.val != 0)
   {
      DEBUG ("c15u: pidpool: create: e %s val %lu depth %u (seen)",
            c->suid().c_str(), c->action.val, h->currdepth);
      ASSERT (h->currdepth < h->pids.size());
      h->joined.erase (c->action.val);
      update_hist_create (h, c->action.val);
      return 0;
   }

   // if this is the deepest THCREAT that we have ever seen on this process,
   // then either we pick one from the joined set if we have one, or we create a
   // new process slot in the unfolding
   if (h->currdepth == h->pids.size())
   {
      if (h->joined.size ())
      {
         // extract one pid from the joined set
         auto fst = h->joined.begin();
         newpid = *fst;
         h->joined.erase (fst);
         // add it to the spike, but negative, since we need to check if it is
         // available next time we want to return it
         DEBUG ("c15u: pidpool: create: e %s val %lu depth %u size %zu newpid %u (from joined)",
               c->suid().c_str(), c->action.val, h->currdepth, h->pids.size(), newpid);
         h->pids.push_back (-newpid);
      }
      else
      {
         // create a new "spike" in the procs variable
         ASSERT (procs.size() == u.num_procs());
         new_prochist ();
         // get a fresh pid from the unfolding structure; write it in the spike
         newpid = u.num_procs();
         DEBUG ("c15u: pidpool: create: e %s val %lu depth %u size %zu newpid %u (new proc)",
               c->suid().c_str(), c->action.val, h->currdepth, h->pids.size(), newpid);
         h->pids.push_back (newpid);
      }
      update_hist_create (h, newpid);
      return newpid;
   }

   // otherwise the spike (h->pids) contains a suggestion for the pid of this
   // THCREAT; if it is a positive number, then that pid is available for this
   // THCREAT; if it is negative, it means that that, in the context of a
   // previous execution, it has been returned to a THCREAT at this depth
   // (h->currdepth) but it had been taken from the set of joined pids
   // (h->joined); it will be available for this THCREAT **only if** we have
   // seen a THJOIN in this execution (causal predecessor of this THCREAT), so
   // we will return it only if it is also now in h->joined, otherwise we need
   // to use a new process
   ASSERT (h->currdepth < h->pids.size());
   newpid = h->pids[h->currdepth];
   ASSERT (newpid != 0);
   if (newpid > 0)
   {
      // nothing to do here
      DEBUG ("c15u: pidpool: create: e %s val %lu depth %u size %zu newpid %u (reused)",
            c->suid().c_str(), c->action.val, h->currdepth, h->pids.size(), newpid);
   }
   else
   {
      newpid = -newpid;
      auto it = h->joined.find (newpid);
      if (it != h->joined.end())
      {
         // if newpid is in the joined set, then we can use it
         h->joined.erase (it);
         DEBUG ("c15u: pidpool: create: e %s val %lu depth %u size %zu newpid %u (reused from join)",
               c->suid().c_str(), c->action.val, h->currdepth, h->pids.size(), newpid);
      }
      else
      {
         // if it is not but it was on a previous execution (since we found it
         // in the spike), it must still be in the assigned set; we need a new
         // process
         ASSERT (h->assigned.find (newpid) != h->assigned.end());
         // create a new "spike" in the procs variable
         ASSERT (procs.size() == u.num_procs());
         new_prochist ();
         // get a fresh pid from the unfolding structure; write it in the spike
         newpid = u.num_procs();
         h->pids[h->currdepth] = newpid;
         DEBUG ("c15u: pidpool: create: e %s val %lu depth %u size %zu newpid %u (new proc, join unavailable)",
               c->suid().c_str(), c->action.val, h->currdepth, h->pids.size(), newpid);
      }
   }
   update_hist_create (h, newpid);
   return newpid;
}

void Pidpool::join (const Event *j)
{
   unsigned p;
   Prochist *h;

   ASSERT (j);
   ASSERT (j->action.type == ActionType::THJOIN);
   ASSERT (j->pid() < procs.size());
   ASSERT (j->action.val < procs.size());

   // get a pointer to the history in this process
   p = j->action.val;
   h = &procs[j->pid()];
   ASSERT (h->joined.find (p) == h->joined.end()); // shall not be here

   // if this process created the process that we are now joining, then we
   // note down the joined pid in the h->joined set for future reuse, as any
   // future THCREAT is a causal successor of "j" and it can safely reuse the
   // joined pid (the reason why is long and we won't explain it here); if the
   // joined pid was started by another process, then this process cannot reuse
   // that pid
   auto it = h->assigned.find (p);
   if (it != h->assigned.end())
   {
      DEBUG ("c15u: pidpool: join: e %s val %lu depth %u size %zu (assigned -> joined)",
            j->suid().c_str(), j->action.val, h->currdepth, h->pids.size());
      h->assigned.erase (it);
      h->joined.insert (p);
   }
   else
   {
      DEBUG ("c15u: pidpool: join: e %s val %lu depth %u size %zu (not assigned, join of external tid)",
            j->suid().c_str(), j->action.val, h->currdepth, h->pids.size());
   }
}

void Pidpool::clear ()
{
   for (auto &h : procs)
   {
      h.currdepth = 0;
      h.assigned.clear ();
      h.joined.clear ();
   }
}

Pidpool::Prochist * Pidpool::new_prochist ()
{
   procs.emplace_back ();
   return &procs.back();
}

void Pidpool::update_hist_create (Prochist *h, unsigned pid)
{
   h->currdepth++;
   // not present in the joined or assigned sets
   ASSERT (h->joined.find (pid) == h->joined.end());
   ASSERT (h->assigned.find (pid) == h->assigned.end());
   h->assigned.insert (pid);
}

