
void C15unfolder::__stream_match_trail
         (const action_streamt &s,
         action_stream_itt &it,
         Trail &t)
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

   unsigned i, count, pid;
   const action_stream_itt end (s.end());

   // match trail events as long as the trail AND the stream contain events
   ASSERT (t.size());
   ASSERT (t[0]->is_bottom());
   count = 0;
   pid = 0;
   for (i = 0; i < t.size() - 1 and it != end; ++it)
   {
      switch (it.type())
      {
      // blue events make the trail advance
      case RT_MTXLOCK :
         ASSERT (t[i]->redbox.size() == count);
         i++;
         ASSERT (t[i]->action.type == ActionType::MTXLOCK);
         ASSERT (t[i]->pid() == pid);
         ASSERT (t[i]->flags.crb);
         count = 0;
         break;

      case RT_MTXUNLK :
         ASSERT (t[i]->redbox.size() == count);
         i++;
         ASSERT (t[i]->action.type == ActionType::MTXUNLK);
         ASSERT (t[i]->pid() == pid);
         ASSERT (t[i]->flags.crb);
         count = 0;
         break;

      case RT_THCREAT :
         ASSERT (t[i]->redbox.size() == count);
         i++;
         ASSERT (t[i]->action.type == ActionType::THCREAT);
         ASSERT (t[i]->pid() == pid);
         ASSERT (t[i]->flags.crb);
         ASSERT (s2dpid (it.id()) == t[i]->action.val);
         count = 0;
         break;

      case RT_THEXIT :
         ASSERT (t[i]->redbox.size() == count);
         i++;
         ASSERT (t[i]->action.type == ActionType::THEXIT);
         ASSERT (t[i]->pid() == pid);
         ASSERT (t[i]->flags.crb);
         count = 0;
         break;

      case RT_THJOIN :
         ASSERT (t[i]->redbox.size() == count);
         i++;
         ASSERT (t[i]->action.type == ActionType::THJOIN);
         ASSERT (t[i]->pid() == pid);
         ASSERT (t[i]->flags.crb);
         count = 0;
         break;

      case RT_THCTXSW :
         ASSERT (t[i]->redbox.size() == count);
         // map is defined
         ASSERT (it.id() == 0 or s2dpid (it.id()) != 0);
         pid = s2dpid(it.id());
         // if this is the first context switch (whcih we detect by looking into
         // the trail), we match a THSTART action with the trail and reset the
         // counter and 
         if (t[i + 1]->action.type == ActionType::THSTART)
         {
            count = 0;
            i++;
         }
         else
         {
            // otherwise this is not the first context switch and the stream
            // *must* contain a blue action immediately after this RT_THCTXSW
            // action; when that action arrives we will check again that the
            // redbox.size() == count for the last event in the previous process
            // (if red events happen between this context switch and that blue
            // event, we will get an assertion violation); there is NOTHING to
            // do in this "else"
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
         // this is supposed to match only RD/WR actions, will produce an
         // assertion violation somewhere if it doesn't
         count++;
         break;
      }
   }
}

bool C15unfolder::stream_to_events
      (Config &c, const action_streamt &s, Trail *t, Disset *d)
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
   Event *e, *ee;
   action_stream_itt it (s.begin());
   const action_stream_itt end (s.end());

   // disset => trail
   ASSERT (!d or t);
   // no trail => empty config
   ASSERT (t or c.is_empty());
   // if got a trail and it is empty => empty config
   ASSERT (!t or t->size() or c.is_empty());
   // if got a with at least 1 event => first event is bottom
   ASSERT (!t or !t->size() or (*t)[0]->is_bottom ());
   // non-empty stream
   ASSERT (it != end);
   // we didn't get too many threads
   ASSERT (s.get_rt()->trace.num_ths <= Unfolding::MAX_PROC);

   DEBUG ("c15u: s2e: c %s t %d", c.str().c_str(), t ? t->size() : -1);

   // skip the first context switch to pid 0, if present
   if (it.type () == RT_THCTXSW)
   {
      ASSERT (it.has_id ());
      ASSERT (it.id() == 0);
      it++;
   }

   // if we got a non-empty trail, we need to first match events from there
   if (t and t->size())
   {
      // otherwise our "last blue event" is the last of the trail, and the
      // config must not be empty
      ASSERT (! c.is_empty());
      e = t->back();
      __stream_match_trail (s, it, *t);
   }
   else
   {
      // we got no trail, or an empty trail; the config will be empty and we
      // need to insert bottom; our "last blue event" (e) is bottom
      ASSERT (c.is_empty());
      e = u.event (nullptr); // bottom
      c.fire (e);
      ASSERT (!e->flags.ind);
      if (d) d->trail_push (e, t->size());
      if (t) t->push(e);
   }

   // we create (or retrieve) an event for every action remaining in the stream
   for (; it != end; ++it)
   {
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
         ASSERT (it.id() == 0 or s2dpid(it.id()) != 0); // map is defined
         e = c[s2dpid(it.id())];
         // on first context switch to a thread we push the THSTART event
         if (!e)
         {
            e = u.proc(s2dpid(it.id()))->first_event();
            if (d and ! d->trail_push (e, t->size())) return false;
            if (t) t->push (e);
            c.fire (e);
         }
         break;

      case RT_THCREAT :
         e->flags.crb = 1;
         ASSERT (it.id() >= 1);
         // creat
         e = u.event ({.type = ActionType::THCREAT}, e);
         // we create now the new process but delay inserting the THSTART event
         // into the config and the trail until the first context switch
         ee = u.event (e);
         e->action.val = ee->pid();
         s2dpid(it.id()) = ee->pid(); // update the pid maps
         d2spid(ee->pid()) = it.id();
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
         ASSERT (it.id() >= 1 and s2dpid(it.id()) != 0); // map is defined
         ee = c[s2dpid(it.id())];
         e = u.event ({.type = ActionType::THJOIN, .val = s2dpid(it.id())}, e, ee);
         if (d and ! d->trail_push (e, t->size())) return false;
         if (t) t->push (e);
         c.fire (e);
         break;

      case RT_RD8 :
         if (! e->flags.crb)
            e->redbox.push_back
               ({.type = ActionType::RD8, .addr = it.addr(), .val = *it.val()});
         break;
      case RT_RD16 :
         if (! e->flags.crb)
            e->redbox.push_back
               ({.type = ActionType::RD16, .addr = it.addr(), .val = *it.val()});
         break;
      case RT_RD32 :
         if (! e->flags.crb)
            e->redbox.push_back
               ({.type = ActionType::RD32, .addr = it.addr(), .val = *it.val()});
         break;
      case RT_RD64 :
         if (! e->flags.crb)
            e->redbox.push_back
               ({.type = ActionType::RD64, .addr = it.addr(), .val = *it.val()});
         break;
      case RT_RD128 :
         ASSERT (it.val_size() == 2);
         if (! e->flags.crb)
         {
            e->redbox.push_back
               ({.type = ActionType::RD64, .addr = it.addr(), .val = it.val()[0]});
            e->redbox.push_back
               ({.type = ActionType::RD64, .addr = it.addr()+8, .val = it.val()[1]});
         }
         break;

      case RT_WR8 :
         if (! e->flags.crb)
            e->redbox.push_back
               ({.type = ActionType::WR8, .addr = it.addr(), .val = *it.val()});
         break;
      case RT_WR16 :
         if (! e->flags.crb)
            e->redbox.push_back
               ({.type = ActionType::WR16, .addr = it.addr(), .val = *it.val()});
         break;
      case RT_WR32 :
         if (! e->flags.crb)
            e->redbox.push_back
               ({.type = ActionType::WR32, .addr = it.addr(), .val = *it.val()});
         break;
      case RT_WR64 :
         if (! e->flags.crb)
            e->redbox.push_back
               ({.type = ActionType::WR64, .addr = it.addr(), .val = *it.val()});
         break;
      case RT_WR128 :
         ASSERT (it.val_size() == 2);
         if (! e->flags.crb)
         {
            e->redbox.push_back
               ({.type = ActionType::WR64, .addr = it.addr(), .val = it.val()[0]});
            e->redbox.push_back
               ({.type = ActionType::WR64, .addr = it.addr()+8, .val = it.val()[1]});
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
   return true;
}

unsigned & C15unfolder::s2dpid (unsigned stidpid)
{
   ASSERT (stidpid < pidmap_s2d.size());
   return pidmap_s2d[stidpid];
}

unsigned & C15unfolder::d2spid (unsigned dpupid)
{
   ASSERT (dpupid < pidmap_d2s.size());
   return pidmap_d2s[dpupid];
}
