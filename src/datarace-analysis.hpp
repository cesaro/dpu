
// Specialization of all methods in StreamConverter<T> for the DataRaceAnalysis.
// These methods get called by the translation method
// StreamConverter<T>::convert(), when transforming the action stream into
// events.

template<>
inline bool StreamConverter<DataRaceAnalysis>::convert_begin
   (stid::action_streamt &s, stid::action_stream_itt &it, Event *e, Config &c)
{
   DEBUG ("unf: conv: begin");
   ASSERT (e->is_bottom());
   ASSERT (redboxfac.empty());
   blue = e;
   return true;
}

template<>
inline bool StreamConverter<DataRaceAnalysis>::convert_end
   (stid::action_streamt &s, Event *e, Config &c)
{
   Redbox *b;
   DEBUG ("unf: conv: end: %s", e->str().c_str());
   SHOW (blue->str().c_str(), "s");

   if (blue->dat == nullptr)
   {
      blue->dat = redboxfac.create ();
   }
   else
   {
#ifdef CONFIG_DEBUG
      b = redboxfac.create ();
      ASSERT (*static_cast<Redbox*>(blue->dat) == *b);
      //delete b; // illegal, the redbox factory is responsible for this
#endif
      redboxfac.clear ();
   }
   ASSERT (redboxfac.empty());
   return true;
}

template<>
inline bool StreamConverter<DataRaceAnalysis>::convert_mtxlock (Event *e,
   Event *epp, Config &c)
{
   DEBUG ("unf: conv: mtxlock");
   return true;
}

template<>
inline bool StreamConverter<DataRaceAnalysis>::convert_mtxunlock (Event *e,
   Event *epp, Config &c)
{
   DEBUG ("unf: conv: mtxunlock");
   return true;
}

template<>
inline bool StreamConverter<DataRaceAnalysis>::convert_ctxsw (unsigned pid)
{
   DEBUG ("unf: conv: ctxsw");
   return true;
}

template<>
inline bool StreamConverter<DataRaceAnalysis>::convert_start (Event *e, Config &c)
{
   DEBUG ("unf: conv: start: %s", e->str().c_str());
   return true;
}

template<>
inline bool StreamConverter<DataRaceAnalysis>::convert_creat (Event *e,
   Event *epp, Config &c)
{
   DEBUG ("unf: conv: creat");
   return true;
}

template<>
inline bool StreamConverter<DataRaceAnalysis>::convert_exit (Event *e,
   Event *epp, Config &c)
{
   DEBUG ("unf: conv: exit");
   return true;
}

template<>
inline bool StreamConverter<DataRaceAnalysis>::convert_join (Event *e,
   Event *epp, Config &c)
{
   DEBUG ("unf: conv: join");
   return true;
}

template<>
inline bool StreamConverter<DataRaceAnalysis>::convert_abort (Event *e, Config &c)
{
   DEBUG ("unf: conv: abort");
   return true;
}

template<>
inline bool StreamConverter<DataRaceAnalysis>::convert_exitnz (Event *e, Config &c)
{
   DEBUG ("unf: conv: exitnz");
   return true;
}

template<>
template<int bytes>
inline bool StreamConverter<DataRaceAnalysis>::convert_rd (Event *e, Addr addr,
   uint64_t val, Config &c)
{
   Redbox *b;
   DEBUG ("unf: conv: read: %s addr %p bytes %d", e->str().c_str(),
      (void*) addr, bytes);

   if (blue != e)
   {
      if (blue->dat == nullptr)
      {
         blue->dat = redboxfac.create ();
      }
      else
      {
#ifdef CONFIG_DEBUG
         b = redboxfac.create ();
         ASSERT (*static_cast<Redbox*>(blue->dat) == *b);
         //delete b; // illegal, the redbox factory is responsible for this
#endif
         redboxfac.clear ();
      }
      blue = e;
      ASSERT (redboxfac.empty());
   }
   redboxfac.add_read (addr, bytes);
   return true;
}

template<>
template<int bytes>
inline bool StreamConverter<DataRaceAnalysis>::convert_wr (Event *e, Addr addr,
   uint64_t val, Config &c)
{
   Redbox *b;
   DEBUG ("unf: conv: write: %s addr %p bytes %d", e->str().c_str(),
      (void*) addr, bytes);

   if (blue != e)
   {
      if (blue->dat == nullptr)
      {
         blue->dat = redboxfac.create ();
      }
      else
      {
#ifdef CONFIG_DEBUG
         b = redboxfac.create ();
         ASSERT (*static_cast<Redbox*>(blue->dat) == *b);
         //delete b; // illegal, the redbox factory is responsible for this
#endif
         redboxfac.clear ();
      }
      blue = e;
      ASSERT (redboxfac.empty());
   }
   redboxfac.add_write (addr, bytes);
   return true;
}

