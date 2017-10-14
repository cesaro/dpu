
#include "unfolder/pidpool.hh"

namespace dpu
{

#if 0

== begin pidpool ==
Proc 0
 curr-depth: 1
 assigned  : 2, 8
 joined    : 
  d 0 pid 3
 >d 1 pid 8
  d 2 pid -3

Proc 1
 curr-depth: 1
 assigned  : 2, 8
 joined    : 
  d 0 pid 3
 >d 1 pid 8
  d 2 pid -3
== end pidpool ==

#endif

void Pidpool::dump () const
{
   unsigned i, j;

   PRINT ("== begin pidpool =="); 
   for (i = 0; i < procs.size(); i++)
   {
      PRINT ("Proc %u", i);
      PRINT (" curr-depth: %d", procs[i].currdepth);
      PRINT_ (" assigned  : ");
      for (unsigned pid : procs[i].assigned) PRINT_ ("%u ", pid);
      PRINT_ ("\n joined    : ");
      for (unsigned pid : procs[i].joined) PRINT_ ("%u ", pid);
      PRINT ("");
      for (j = 0; j < procs[i].pids.size(); j++)
         PRINT (" %sd %u pid %d",
               j == procs[i].currdepth ? ">" : " ", j, procs[i].pids[j]);
   }
   PRINT ("== end pidpool =="); 
}

} // namespace
