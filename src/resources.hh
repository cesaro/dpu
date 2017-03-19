
#ifndef __RESOURCES_HH_
#define __RESOURCES_HH_

class Resources
{
public:

   // wall time, in us (microseconds, 1/1000000 secs)
   unsigned long walltime;
   // user time, in us
   unsigned long cputime;
   // kernel time, in us
   unsigned long systime;
   // maximum resident set, in kb
   unsigned long maxrss;

   Resources ();
   void update ();
private:
   struct timeval start;
};

#endif
