

/// Specialization of StreamConverter<T>::convert_begin() for T = Unfolder
template<>
inline bool StreamConverter<Unfolder>::convert_begin (stid::action_streamt &s,
   stid::action_stream_itt &it, Event *e, Config &c)
{
   DEBUG ("unf: conv: begin");
   ASSERT (e->is_bottom());
   ASSERT (redboxfac.empty());
   blue = e;
   return true;
}

template<>
inline bool StreamConverter<Unfolder>::convert_end (stid::action_streamt &s,
   Event *e, Config &c)
{
   DEBUG ("unf: conv: end");
   ASSERT (blue->dat == nullptr);
   blue->dat = redboxfac.create ();
   return true;
}

template<>
inline bool StreamConverter<Unfolder>::convert_mtxlock (Event *e, Event *epp,
      Config &c)
{
   DEBUG ("unf: conv: mtxlock");
   return true;
}

template<>
inline bool StreamConverter<Unfolder>::convert_mtxunlock (Event *e, Event *epp,
      Config &c)
{
   DEBUG ("unf: conv: mtxunlock");
   return true;
}

template<>
inline bool StreamConverter<Unfolder>::convert_ctxsw (unsigned pid)
{
   DEBUG ("unf: conv: ctxsw");
   return true;
}

template<>
inline bool StreamConverter<Unfolder>::convert_start (Event *e, Config &c)
{
   DEBUG ("unf: conv: start: %s", e->str().c_str());
   return true;
}

template<>
inline bool StreamConverter<Unfolder>::convert_creat (Event *e, Event *epp, Config &c)
{
   DEBUG ("unf: conv: creat");
   return true;
}

template<>
inline bool StreamConverter<Unfolder>::convert_exit (Event *e, Event *epp, Config &c)
{
   DEBUG ("unf: conv: exit");
   return true;
}

template<>
inline bool StreamConverter<Unfolder>::convert_join (Event *e, Event *epp, Config &c)
{
   DEBUG ("unf: conv: join");
   return true;
}

template<>
inline bool StreamConverter<Unfolder>::convert_abort (Event *e, Config &c)
{
   DEBUG ("unf: conv: abort");
   return true;
}

template<>
inline bool StreamConverter<Unfolder>::convert_exitnz (Event *e, Config &c)
{
   DEBUG ("unf: conv: exitnz");
   return true;
}

template<>
template<int bytes>
inline bool StreamConverter<Unfolder>::convert_rd (Event *e, Addr addr,
   uint64_t val, Config &c)
{
   DEBUG ("unf: conv: read: %s addr %p bytes %d", e->str().c_str(),
      (void*) addr, bytes);

   if (blue == e)
   {
      redboxfac.add_read (addr, bytes);
   }
   else
   {
      ASSERT (blue->dat == nullptr);
      blue->dat = redboxfac.create ();
      blue = e;
      ASSERT (redboxfac.empty());
      redboxfac.add_read (addr, bytes);
   }
   return true;
}

template<>
template<int bytes>
inline bool StreamConverter<Unfolder>::convert_wr (Event *e, Addr addr,
   uint64_t val, Config &c)
{
   DEBUG ("unf: conv: write: %s addr %p bytes %d", e->str().c_str(),
      (void*) addr, bytes);
   if (blue == e)
   {
      redboxfac.add_write (addr, bytes);
   }
   else
   {
      ASSERT (blue->dat == nullptr);
      blue->dat = redboxfac.create ();
      blue = e;
      ASSERT (redboxfac.empty());
      redboxfac.add_read (addr, bytes);
   }
   return true;
}
