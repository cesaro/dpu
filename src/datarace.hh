#ifndef _DATARACE_HH_
#define _DATARACE_HH_

#include "pes/event.hh"
#include "pes/cut.hh"
#include "unfolder/replay.hh"
#include "defect.hh"
#include "verbosity.h"

namespace dpu {

struct DataRace : public Defect
{
   const Event *e1;
   const Event *e2;
   Addr addr;
   unsigned size;
   Cut trace;

   DataRace (const Event *e1, const Event *e2, Addr a, unsigned size) :
      e1 (e1),
      e2 (e2),
      addr (a),
      size (size),
      trace (e1->cone, e2->cone)
   {
      ASSERT (e1);
      ASSERT (e2);

      Replay r (*e1->unfolding(), trace);

      description = fmt ("Data race: threads %d and %d can concurrently "
            "access and modify variable %p (%d bytes)",
            r.pidmap.get (e1->pid()), r.pidmap.get (e2->pid()), (void*) a, size);
      replay = r;
   }

   void dump () const
   {
      PRINT ("== begin data race =="); 
      PRINT (" %s", description.c_str());
      PRINT ("\n Offending execution:");
      trace.dump ();
      PRINT ("== end data race =="); 
   }
};

} // namespace

#endif

