
void C15unfolder::__stream_match_trail
         (const action_streamt &s,
         action_stream_itt &it,
         std::vector<unsigned> &pidmap,
         Trail &t)
{
   unsigned i, count, pid;
   const action_stream_itt end (s.end());

   // match trail events as long as the trail AND the stream contain events
   count = 0;
   pid = 0;
   for (i = 0; i < t.size() and it != end; ++it)
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
         ASSERT (it.id() < pidmap.size()); // pid in bounds
         pidmap[it.id()] = t[i]->action.val; // update pidmap
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
         ASSERT (it.id() < pidmap.size()); // pid in bounds
         ASSERT (it.id() == 0 or pidmap[it.id()] != 0); // map is defined
         pid = pidmap[it.id()];
         break;

      case RT_ALLOCA :
      case RT_MALLOC :
      case RT_FREE :
      case RT_CALL :
      case RT_RET :
         // not implemented, but doesn't hurt ;)
         break;

      default :
         // this is supposed to be only RD/WR actions, will produce an assertion
         // violation somewhere if it's not
         count++;
         break;
      }
   }
}

void C15unfolder::stream_to_events
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
   // Config and the Trail as new actions are read from the stream. If a Disset
   // is provided, it will be updated (with trail_push) only for the new events
   // added to the trail.

   // variables
   Event *e, *ee;
   action_stream_itt it (s.begin());
   const action_stream_itt end (s.end());
   std::vector<unsigned> pidmap (s.get_rt()->trace.num_ths);

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
      __stream_match_trail (s, it, pidmap, *t);
   }
   else
   {
      // we got no trail, or an empty trail; the config will be empty and we
      // need to insert bottom; our "last blue event" (e) is bottom
      ASSERT (c.is_empty());
      e = u.event (nullptr); // bottom
      c.fire (e);
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
         c.fire (e);
         if (d) d->trail_push (e, t->size());
         if (t) t->push (e);
         DEBUG ("");
         break;

      case RT_MTXUNLK :
         e->flags.crb = 1;
         ee = c.mutex_max (it.addr());
         e = u.event ({.type = ActionType::MTXUNLK, .addr = it.addr()}, e, ee);
         c.fire (e);
         if (d) d->trail_push (e, t->size());
         if (t) t->push (e);
         DEBUG ("");
         break;

      case RT_THCTXSW :
         e->flags.crb = 1;
         ASSERT (it.id() < pidmap.size()); // pid in bounds
         ASSERT (it.id() == 0 or pidmap[it.id()] != 0); // map is defined
         e = c[pidmap[it.id()]];
         ASSERT (e); // we have at least one event there
         // on the the first context switch to a thread, we put the start in the
         // trail and disset
         if (e->action.type == ActionType::THSTART)
         {
            if (d) d->trail_push (e, t->size());
            if (t) t->push (e);
         }
         break;

      case RT_THCREAT :
         e->flags.crb = 1;
         ASSERT (it.id() < pidmap.size()); // pid in bounds
         ASSERT (it.id() >= 1 and pidmap[it.id()] == 0); // map entry undefined
         // creat
         e = u.event ({.type = ActionType::THCREAT}, e);
         // start
         ee = u.event (e);
         e->action.val = ee->pid();
         pidmap[it.id()] = ee->pid();
         c.fire (e);
         c.fire (ee);
         if (d) d->trail_push (e, t->size());
         if (t) t->push (e);
         DEBUG ("");
         break;

      case RT_THEXIT :
         e->flags.crb = 1;
         e = u.event ({.type = ActionType::THEXIT}, e);
         c.fire (e);
         if (d) d->trail_push (e, t->size());
         if (t) t->push (e);
         DEBUG ("");
         break;

      case RT_THJOIN :
         e->flags.crb = 1;
         ASSERT (it.id() < pidmap.size()); // pid in bounds
         ASSERT (it.id() >= 1 and pidmap[it.id()] != 0); // map is defined
         ee = c.mutex_max (pidmap[it.id()]);
         ASSERT (ee and ee->action.type == ActionType::THEXIT);
         e = u.event ({.type = ActionType::THJOIN, .val = pidmap[it.id()]}, e, ee);
         c.fire (e);
         if (d) d->trail_push (e, t->size());
         if (t) t->push (e);
         DEBUG ("");
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
}

