

/// Specialization of StreamConverter<T>::convert_begin() for T = Unfolder
template<>
inline bool StreamConverter<Unfolder>::convert_begin (stid::action_streamt &s,
   stid::action_stream_itt &it, Event *e, Config &c)
{
   DEBUG ("unf: conv: begin");
   return true;
}

template<>
inline bool StreamConverter<Unfolder>::convert_end (stid::action_streamt &s, Config &c)
{
   DEBUG ("unf: conv: end");
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
   DEBUG ("unf: conv: start");
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
template<int bits>
inline bool StreamConverter<Unfolder>::convert_rd (Event *e, Config &c)
{
   DEBUG ("unf: conv: rd: %s bits %d", e->str().c_str(), bits);
   return true;
}

template<>
template<int bits>
inline bool StreamConverter<Unfolder>::convert_wr (Event *e, Config &c)
{
   DEBUG ("unf: conv: write: %s bits %d", e->str().c_str(), bits);
   return true;
}
