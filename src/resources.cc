
#include <sys/time.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <unistd.h>

#include "verbosity.h"
#include "resources.hh"

#define TIMEVAL_2_US(tv) ((tv).tv_sec * 1000 * 1000 + (tv).tv_usec)


Resources::Resources ()
{
   int ret;
   ret = gettimeofday (&start, nullptr);
   ASSERT (ret == 0);
}

void Resources::update ()
{
   struct timeval end;
	struct rusage r;
   int ret;

   // get current wall time
   ret = gettimeofday (&end, nullptr);
   ASSERT (ret == 0);

   // end - start
   ASSERT (end.tv_sec >= start.tv_sec);
   end.tv_sec -= start.tv_sec;
   if (end.tv_usec < start.tv_usec)
   {
      ASSERT (end.tv_sec >= 1);
      end.tv_sec -= 1;
      end.tv_usec += (1000 * 1000 - start.tv_usec);
   }
   else
   {
      end.tv_usec -= start.tv_usec;
   }
   walltime = TIMEVAL_2_US(end);

   // get cpu time
	ret = getrusage (RUSAGE_SELF, &r);
   ASSERT (ret == 0);
   cputime = TIMEVAL_2_US (r.ru_utime);
   systime = TIMEVAL_2_US (r.ru_stime);
   maxrss = 0;
   maxrss = r.ru_maxrss;
   //SHOW (r.ru_maxrss, "lu");
}

//	/* this will only work in linux, in macos u.unf.maxrss is set to maxrss
//	 * in kb */
//	fd = open ("/proc/self/statm", O_RDONLY);
//	if (fd < 0) return;
//	ret = read (fd, buff, 128);
//	close (fd);
//	buff[127] = 0;
//	u.unf.maxrss = strtoul (buff, 0, 10) * sysconf (_SC_PAGESIZE) >> 10;
