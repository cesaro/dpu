
template<typename T>
bool StreamConverter<T>::convert (stid::action_streamt &s, Config &c)
{
   // - The stream is a sequence of actions starting from the initial state
   // - The config is empty
   //
   // FIXME: review this text
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
   Event *e, *epp, *ee;
   stid::action_stream_itt it (s.begin());
   const stid::action_stream_itt end (s.end());

   DEBUG ("sconv: convert: c %s", c.str().c_str());

   // reset the pidpool, the pidmap, and the start map for this execution
   pidmap.clear ();
   pidpool.clear ();
   ASSERT (c.empty ());
   for (unsigned i = 0; i < Unfolding::MAX_PROC; i++)
      ASSERT (start[i] == nullptr);

   // skip the first context switch to pid 0, if present
   if (it != end and it.type () == RT_THCTXSW)
   {
      ASSERT (it.has_id ());
      ASSERT (it.id() == 0);
      it++;
   }

   // event "begin"
   e = u.event (nullptr); // bottom
   if (! convert_begin (s, it, e, c)) return false;
   c.fire (e);

   // we create (or retrieve) an event for every action remaining in the stream
   for (; it != end; ++it)
   {
      DEBUG ("sconv: convert: it %s", it.str());
      epp = e;
      switch (it.type())
      {
      case RT_MTXLOCK :
         ee = c.mutex_max (it.addr());
         e = u.event ({.type = ActionType::MTXLOCK, .addr = it.addr()}, e, ee);
         if (! convert_mtxlock (e, epp, c)) return false;
         c.fire (e);
         break;

      case RT_MTXUNLK :
         ee = c.mutex_max (it.addr());
         e = u.event ({.type = ActionType::MTXUNLK, .addr = it.addr()}, e, ee);
         if (! convert_mtxunlock (e, epp, c)) return false;
         c.fire (e);
         break;

      case RT_THCTXSW :
         if (! convert_ctxsw (it.id())) return false;

         // on first context switch to a thread we push the THSTART event
         e = start[pidmap.get(it.id())];
         if (e)
         {
            if (! convert_start (e, c)) return false;
            start[pidmap.get(it.id())] = nullptr;
            c.fire (e);
         }
         else
         {
            // not the 1st context switch, set e to the last event in the new
            // thread
            e = c[pidmap.get(it.id())];
            ASSERT (e);
         }
         break;

      case RT_THCREAT :
         ASSERT (it.id() >= 1);
         // we insert or retrive THCREAT event, requesting insertion with pid=0
         e = u.event ({.type = ActionType::THCREAT, .val = 0}, e);
         // we insert or retrive THCREAT event, requesting insertion with pid=0
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

         if (! convert_creat (e, epp, c)) return false;
         c.fire (e);

         // we create now the new process but delay inserting the THSTART event
         // into the config and the trail until the first context switch
         ee = u.event (e);
         start[e->action.val] = ee;
         ASSERT (ee->pid() == e->action.val);
         break;

      case RT_THEXIT :
         e = u.event ({.type = ActionType::THEXIT}, e);
         if (! convert_exit (e, epp, c)) return false;
         c.fire (e);
         break;

      case RT_THJOIN :
         ee = c[pidmap.get(it.id())];
         e = u.event ({.type = ActionType::THJOIN, .val = pidmap.get(it.id())}, e, ee);
         // notify the pidpool that we saw a THJOIN
         pidpool.join (e);
         if (! convert_join (e, epp, c)) return false;
         c.fire (e);
         break;

      case RT_ABORT :
         if (! convert_abort (e, c)) return false;
         break;

      case RT_EXITNZ :
         if (! convert_exitnz (e, c)) return false;
         break;
      case RT_RD8 :
         if (! convert_rd<1> (e, it.addr(), *it.val(), c)) return false;
         break;
      case RT_RD16 :
         if (! convert_rd<2> (e, it.addr(), *it.val(), c)) return false;
         break;
      case RT_RD32 :
         if (! convert_rd<4> (e, it.addr(), *it.val(), c)) return false;
         break;
      case RT_RD64 :
         if (! convert_rd<8> (e, it.addr(), *it.val(), c)) return false;
         break;
      case RT_RD128 :
         ASSERT (it.val_size() == 2);
         if (! convert_rd<8> (e, it.addr(), it.val()[0], c)) return false;
         if (! convert_rd<8> (e, it.addr() + 8, it.val()[1], c)) return false;
         break;

      case RT_WR8 :
         if (! convert_wr<1> (e, it.addr(), *it.val(), c)) return false;
         break;
      case RT_WR16 :
         if (! convert_wr<2> (e, it.addr(), *it.val(), c)) return false;
         break;
      case RT_WR32 :
         if (! convert_wr<4> (e, it.addr(), *it.val(), c)) return false;
         break;
      case RT_WR64 :
         if (! convert_wr<8> (e, it.addr(), *it.val(), c)) return false;
         break;
      case RT_WR128 :
         ASSERT (it.val_size() == 2);
         if (! convert_wr<8> (e, it.addr(), it.val()[0], c)) return false;
         if (! convert_wr<8> (e, it.addr() + 8, it.val()[1], c)) return false;
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

   if (! convert_end (s, e, c)) return false;

   if (verb_debug) pidmap.dump (true);
   return true;
}
