
#ifndef __UNFOLDER_STREAMTOEVENTS_HH_
#define __UNFOLDER_STREAMTOEVENTS_HH_

#include "stid/action_stream.hh"

#include "pes/event.hh"
#include "pes/config.hh"
#include "pes/unfolding.hh"

#include "unfolder/pidpool.hh"
#include "unfolder/pidmap.hh"

namespace dpu
{

/// Specialize this template for a type T in order to inject fields in a
/// StreamConverter<T>. By default nothing is inserted.
template<typename T>
class StreamConverterTraits
{
};

template<typename T = void>
class StreamConverter : public StreamConverterTraits<T>
{
public:

   StreamConverter () :
      StreamConverterTraits<T> (),
      u (),
      pidpool (u),
      pidmap ()
   {}

   /// The unfolding data structure
   Unfolding u;

   /// This object is used to select the pid of a process whenver we create a
   /// new THCREAT event
   Pidpool pidpool;

   /// A pid map is a mapping from steroids pids to dpu pids valid in the
   /// context of 1 single execution. This field needs to be reinitialized on
   /// every call to convert().
   Pidmap pidmap;

   /// The method tream_match_trail() needs to communicate to
   /// stream_to_events() the THSTART events of every thread whose THCREAT was
   /// parsed in stream_match_trail() but whose THSTART didn't make it to the
   /// configuration before stream_to_events() got the control. We use this map
   /// to achieve that, we map each pid to the THSTART event in that thread for
   /// this execution.
   Event *start[Unfolding::MAX_PROC];

   /// Translates the stream of actions \p s coming from steroids into events,
   /// inserting them in \p c and updating all the other fields in this class.
   /// As the conversion progresses it calls the convert_* methods below, which
   /// will often be templace-specialized by users of this class. Default
   /// implementations, below, do nothing.
   bool convert (stid::action_streamt &s, Config &c);

protected:
   bool convert_begin (stid::action_streamt &s, stid::action_stream_itt &it,
      Event *e, Config &c) { return true; }
   bool convert_end (stid::action_streamt &s, Event *e, Config &c)
      { return true; }

   bool convert_mtxlock (Event *e, Event *epp, Config &c) { return true; }
   bool convert_mtxunlock (Event *e, Event *epp, Config &c) { return true; }
   bool convert_ctxsw (unsigned pid) { return true; }
   bool convert_start (Event *e, Config &c) { return true; }
   bool convert_creat (Event *e, Event *epp, Config &c) { return true; }
   bool convert_exit (Event *e, Event *epp, Config &c) { return true; }
   bool convert_join (Event *e, Event *epp, Config &c) { return true; }

   bool convert_abort (Event *e, Config &c) { return true; }
   bool convert_exitnz (Event *e, Config &c) { return true; }

   template<int bytes>
   bool convert_rd (Event *e, Addr addr, uint64_t val, Config &c)
      { return true; }

   template<int bits>
   bool convert_wr (Event *e, Addr addr, uint64_t val, Config &c)
      { return true; }
};

#include "unfolder/stream-converter.hpp"

} // namespace dpu
#endif
