
inline void C15unfolder::report_add_nondet_violation (const Trail &t, unsigned where, ActionType found)
{
   std::string description;

   ASSERT (where < t.size());
   ASSERT (t[where]->action.type != found);

   description = fmt (
      "WARNING: the program dind't replay deterministically: "
      "expected %s but got %s",
      action_type_str (t[where]->action.type),
      action_type_str (found));
   Replay replay (u, t, where + 1);

   report.add_defect (Defect (description, replay));
}

bool C15unfolder::stream_match_trail
   (const stid::action_streamt &s, stid::action_stream_itt &it, Trail &t,
   Pidmap &pidmap)
{
   // We can see the stream as a sequence of subsequences, starting either by a
   // blue action or an "invisible" THSTART action, and followed by red actions.
   // When we read a blue action, we determine that the previous subsequence is
   // over. When we read a context switch, the previous sequence is also over,
   // and two things might happen: we context-switch to a thread for the first
   // time, or not. In the former case we will "see" an invisible THSTART action;
   // in the latter case we will see a regular (visible) blue action.
   //
   // The loop below uses variable i to point to the blue event in the trail
   // that marks the beginning of the subsequence. Initially it points to
   // bottom.  We increment a counter per red action read. When we finish a
   // subsequence (new blue action or context switch), we assert that we got the
   // good number of red events at trail[i] and increment i. If we saw a blue
   // action, we match it with trail[i] now. If we saw
   // context switch, and it is the first one, i correctly points now to the
   // corresponding blue THSTART and there is nothing to do. If it is not, i
   // incorrectly points to the next blue event in the trail BEFORE we have read
   // it from the stream, so we need to decrement i by one, so that the arrival
   // of the next blue action in the stream will correctly increment i.

   unsigned i, count, pid, ret;
   const stid::action_stream_itt end (s.end());

   // match trail events as long as the trail AND the stream contain events
   ASSERT (t.size());
   ASSERT (it != end);
   ASSERT (t[0]->is_bottom());
   count = 0;
   pid = 0;

   // match trail events as long as the trail AND the stream contain events
   for (i = 0; i < t.size() - 1 and it != end; ++it)
   {
      SHOW (it.str(), "s");
      switch (it.type())
      {
      // blue events make the trail advance
      case RT_MTXLOCK :
         //ASSERT (t[i]->data<Redbox>().size() == count);
         i++;
         if (t[i]->action.type != ActionType::MTXLOCK)
         {
            report_add_nondet_violation (t, i, ActionType::MTXLOCK);
            return false;
         }
         ASSERT (t[i]->pid() == pid);
         ASSERT (t[i]->flags.crb);
         count = 0;
         break;

      case RT_MTXUNLK :
         //ASSERT (t[i]->data<Redbox>().size() == count);
         i++;
         if (t[i]->action.type != ActionType::MTXUNLK)
         {
            report_add_nondet_violation (t, i, ActionType::MTXLOCK);
            return false;
         }
         ASSERT (t[i]->pid() == pid);
         ASSERT (t[i]->flags.crb);
         count = 0;
         break;

      case RT_THCREAT :
         //ASSERT (t[i]->data<Redbox>().size() == count);
         i++;
         if (t[i]->action.type != ActionType::THCREAT)
         {
            report_add_nondet_violation (t, i, ActionType::THCREAT);
            return false;
         }
         ASSERT (t[i]->pid() == pid);
         ASSERT (t[i]->flags.crb);
         SHOW (it.str(), "s");
         SHOW (t[i]->str().c_str(), "s");
         // we let the pidpool know that we saw a THCREAT
         ret = pidpool.create (t[i]);
         ASSERT (ret == 0);
         // we map the steroids tid for this new thread (it.id()) to the pid in
         // dpu for that thread (t[id]->action.val)
         pidmap.add (it.id(), t[i]->action.val);
         // we record the corresponding THSTART in start[]
         start[t[i]->action.val] = u.event (t[i]);
         SHOW (u.event(t[i])->str().c_str(), "s");
         count = 0;
         break;

      case RT_THEXIT :
         //ASSERT (t[i]->data<Redbox>().size() == count);
         i++;
         if (t[i]->action.type != ActionType::THEXIT)
         {
            report_add_nondet_violation (t, i, ActionType::THEXIT);
            return false;
         }
         ASSERT (t[i]->pid() == pid);
         ASSERT (t[i]->flags.crb);
         count = 0;
         break;

      case RT_THJOIN :
         //ASSERT (t[i]->data<Redbox>().size() == count);
         i++;
         if (t[i]->action.type != ActionType::THJOIN)
         {
            report_add_nondet_violation (t, i, ActionType::THJOIN);
            return false;
         }
         ASSERT (t[i]->pid() == pid);
         ASSERT (t[i]->flags.crb);
         SHOW (it.str(), "s");
         // we let the pidpool know that we saw a THJOIN
         pidpool.join (t[i]);
         count = 0;
         break;

      case RT_THCTXSW :
         //ASSERT (t[i]->data<Redbox>().size() == count);
         // if this is the first context switch (whcih we detect by looking into
         // the trail), we match a THSTART action with the trail and reset the
         // counter and clear the start[] vector
         pid = pidmap.get (it.id());
         if (t[i + 1]->action.type == ActionType::THSTART)
         {
            count = 0;
            i++;
            ASSERT (start[pid]);
            start[pid] = nullptr;
         }
         else
         {
            ASSERT (!start[pid]);
            // otherwise this is not the first context switch and the stream
            // *must* contain a blue action immediately after this RT_THCTXSW
            // action; when that action arrives we will check again that the
            // data<Redbox>.size() == count for the last event in the previous
            // process (if red events happen between this context switch and
            // that blue event, we will get an assertion violation); there is
            // NOTHING to do in this "else"
         }
         break;

      case RT_ABORT :
      case RT_EXITNZ :
         // nothing to do with these when we see them again; first time we see
         // them they will produce warnings in the defect report
         break;

#if 0
      // currently steroids does not generate these events
      case RT_ALLOCA :
      case RT_MALLOC :
      case RT_FREE :
      case RT_CALL :
      case RT_RET :
         // not implemented, but doesn't hurt ;)
         break;
#endif

      default :
         // this is supposed to match only RD/WR actions, will produce an
         // assertion violation somewhere if it doesn't
         count++;
         break;
      }
   }
   return true;
}

bool C15unfolder::stream_to_events
      (Config &c, const stid::action_streamt &s, Trail *t, Disset *d)
{
   // - If we get a Disset, then we also need to get a trail.
   // - If we get a Trail, then the Config represents the state *at the end* of
   //   the Trail.
   // - If there is no trail or the provide trail is empty, then the Config
   //   needs to be empty
   // - The stream is a sequence of actions starting from the initial state
   //
   // This function transforms the stream into events, updating the Config, the
   // Trail and the Disset if they are provided. If a trail is provided the
   // function will assert that the stream actions correspond to the events in
   // the trail, until the trail (or the stream) ends. After that it will rely
   // on the fact that the state of the Config equals the state reached so far
   // in the stream, and will continue inserting new events and extending the
   // Config and the Trail as new actions are read from the stream.  If a Disset
   // is provided, it will be updated (with trail_push) only for the new events
   // added to the trail. If, when inserting elements into the Trail, the
   // Disset reports that this is an SSB, we stop and return false. If we never
   // encounter a SSB, we return true.

   // variables
   unsigned newpid;
   Event *e, *ee;
   stid::action_stream_itt it (s.begin());
   const stid::action_stream_itt end (s.end());
   Pidmap pidmap;
   Defect defect;

   // NOTE: all of these checks can be done later
   // disset => trail
   ASSERT (!d or t);
   // no trail => empty config
   ASSERT (t or c.empty());
   // if got a trail and it is empty => empty config
   ASSERT (!t or t->size() or c.empty());
   // if got a with at least 1 event => first event is bottom
   ASSERT (!t or !t->size() or (*t)[0]->is_bottom ());
   // we didn't get too many threads
   if (s.get_rt()->trace.num_ths > Unfolding::MAX_PROC)
      throw std::out_of_range (
         fmt ("Execution created %u threads, but unfolder supports up to %u",
               s.get_rt()->trace.num_ths, Unfolding::MAX_PROC));

   DEBUG ("c15u: s2e: c %s t %zd", c.str().c_str(), t ? t->size() : -1);

   // reset the pidpool and the pidmap for this execution
   pidpool.clear ();
   for (unsigned i = 0; i < Unfolding::MAX_PROC; i++) ASSERT (start[i] == nullptr);

   // skip the first context switch to pid 0, if present
   if (it != end and it.type () == RT_THCTXSW)
   {
      ASSERT (it.has_id ());
      ASSERT (it.id() == 0);
      it++;
   }

   // if we got a non-empty trail, we need to first match events from there
   if (t and t->size())
   {
      // our "last blue event" is the last of the trail, and the
      // config must not be empty
      ASSERT (! c.empty());
      e = t->back();
      if (not stream_match_trail (s, it, *t, pidmap)) return false;
   }
   else
   {
      // we got no trail, or an empty trail; the config will be empty and we
      // need to insert bottom; our "last blue event" (e) is bottom
      ASSERT (c.empty());
      e = u.event (nullptr); // bottom
      c.fire (e);
      ASSERT (!e->flags.ind);
      if (d) d->trail_push (e, t->size());
      if (t) t->push(e);
   }

   // we create (or retrieve) an event for every action remaining in the stream
   for (; it != end; ++it)
   {
      SHOW (it.str(), "s");
      switch (it.type())
      {
      case RT_MTXLOCK :
         e->flags.crb = 1;
         ee = c.mutex_max (it.addr());
         e = u.event ({.type = ActionType::MTXLOCK, .addr = it.addr()}, e, ee);
         if (d and ! d->trail_push (e, t->size())) return false;
         if (t) t->push(e);
         c.fire (e);
         break;

      case RT_MTXUNLK :
         e->flags.crb = 1;
         ee = c.mutex_max (it.addr());
         e = u.event ({.type = ActionType::MTXUNLK, .addr = it.addr()}, e, ee);
         if (d and ! d->trail_push (e, t->size())) return false;
         if (t) t->push (e);
         c.fire (e);
         break;

      case RT_THCTXSW :
         e->flags.crb = 1;
         // on first context switch to a thread we push the THSTART event
         e = start[pidmap.get(it.id())];
         if (e)
         {
            start[pidmap.get(it.id())] = nullptr;
            if (d and ! d->trail_push (e, t->size())) return false;
            if (t) t->push (e);
            c.fire (e);
         }
         else
         {
            // not the 1st context switch
            e = c[pidmap.get(it.id())];
            ASSERT (e);
         }
         break;

      case RT_THCREAT :
         e->flags.crb = 1;
         ASSERT (it.id() >= 1);
         // we insert or retrive THCREAT event, requesting insertion with pid=0
         e = u.event ({.type = ActionType::THCREAT, .val = 0}, e);
         // we now let the pidpool choose the pid of the new process
         newpid = pidpool.create (e);
         // init pid if the event has just been inserted in the unfolding
         if (e->action.val == 0)
         {
            ASSERT (newpid != 0);
            e->action.val = newpid;
         }
         else
         {
            ASSERT (newpid == 0); // the pidpool must have been aware
         }
         // we update the pidmap for this execution
         pidmap.add (it.id(), e->action.val);
         // we create now the new process but delay inserting the THSTART event
         // into the config and the trail until the first context switch
         ee = u.event (e);
         start[e->action.val] = ee;
         ASSERT (ee->pid() == e->action.val);
         if (d and ! d->trail_push (e, t->size())) return false;
         if (t) t->push (e);
         c.fire (e);
         break;

      case RT_THEXIT :
         e->flags.crb = 1;
         e = u.event ({.type = ActionType::THEXIT}, e);
         c.fire (e);
         if (d and ! d->trail_push (e, t->size())) return false;
         if (t) t->push (e);
         break;

      case RT_THJOIN :
         e->flags.crb = 1;
         ee = c[pidmap.get(it.id())];
         e = u.event ({.type = ActionType::THJOIN, .val = pidmap.get(it.id())}, e, ee);
         // notify the pidpool that we saw a THJOIN
         pidpool.join (e);
         if (d and ! d->trail_push (e, t->size())) return false;
         if (t) t->push (e);
         c.fire (e);
         break;

      case RT_ABORT :
         if (report.nr_abort >= CONFIG_MAX_DEFECT_REPETITION) break;
         report.nr_abort++;
         defect.description = "The program called abort()";
         if (t)
            defect.replay = std::move (Replay (u, *t));
         else
            defect.replay.clear();
         report.add_defect (defect);
         break;

      case RT_EXITNZ :
         if (report.nr_exitnz >= CONFIG_MAX_DEFECT_REPETITION) break;
         report.nr_exitnz++;
         defect.description =
               fmt ("The program exited with errorcode %d", it.id());
         if (t)
            defect.replay = std::move (Replay (u, *t));
         else
            defect.replay.clear();
         report.add_defect (defect);
         break;

      case RT_RD8 :
         if (! e->flags.crb)
            //e->data<Redbox>().push_back
            //   ({.type = ActionType::RD8, .addr = it.addr(), .val = *it.val()});
         break;
      case RT_RD16 :
         if (! e->flags.crb)
            //e->data<Redbox>().push_back
            //   ({.type = ActionType::RD16, .addr = it.addr(), .val = *it.val()});
         break;
      case RT_RD32 :
         if (! e->flags.crb)
            //e->data<Redbox>().push_back
            //   ({.type = ActionType::RD32, .addr = it.addr(), .val = *it.val()});
         break;
      case RT_RD64 :
         if (! e->flags.crb)
            //e->data<Redbox>().push_back
            //   ({.type = ActionType::RD64, .addr = it.addr(), .val = *it.val()});
         break;
      case RT_RD128 :
         ASSERT (it.val_size() == 2);
         if (! e->flags.crb)
         {
            //e->data<Redbox>().push_back
            //   ({.type = ActionType::RD64, .addr = it.addr(), .val = it.val()[0]});
            //e->data<Redbox>().push_back
            //   ({.type = ActionType::RD64, .addr = it.addr()+8, .val = it.val()[1]});
         }
         break;

      case RT_WR8 :
         if (! e->flags.crb)
            //e->data<Redbox>().push_back
            //   ({.type = ActionType::WR8, .addr = it.addr(), .val = *it.val()});
         break;
      case RT_WR16 :
         if (! e->flags.crb)
            //e->data<Redbox>().push_back
            //   ({.type = ActionType::WR16, .addr = it.addr(), .val = *it.val()});
         break;
      case RT_WR32 :
         if (! e->flags.crb)
            //e->data<Redbox>().push_back
            //   ({.type = ActionType::WR32, .addr = it.addr(), .val = *it.val()});
         break;
      case RT_WR64 :
         if (! e->flags.crb)
            //e->data<Redbox>().push_back
            //   ({.type = ActionType::WR64, .addr = it.addr(), .val = *it.val()});
         break;
      case RT_WR128 :
         ASSERT (it.val_size() == 2);
         if (! e->flags.crb)
         {
            //e->data<Redbox>().push_back
            //   ({.type = ActionType::WR64, .addr = it.addr(), .val = it.val()[0]});
            //e->data<Redbox>().push_back
            //   ({.type = ActionType::WR64, .addr = it.addr()+8, .val = it.val()[1]});
         }
         break;

      case RT_ALLOCA :
      case RT_MALLOC :
      case RT_FREE :
      case RT_CALL :
      case RT_RET :
         // not implemented, but doesn't hurt ;)
         break;

      default :
         SHOW (it.type(), "d");
         SHOW (it.addr(), "lu");
         SHOW (it.val()[0], "lu");
         SHOW (it.id(), "u");
         SHOW (it.str(), "s");
         ASSERT (0);
      }
   }
   if (verb_debug) pidmap.dump (true);
   return true;
}

