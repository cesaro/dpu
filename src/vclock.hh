
#ifndef __VCLOCK_HH_
#define __VCLOCK_HH_

#include <vector>

namespace dpu
{

class Vclock
{
public:
   Vclock();
   Vclock(int tid, int count);
   Vclock (const Vclock &v); //copy constructor
   Vclock (const Vclock &v1, const Vclock &v2);

   bool operator== (const Vclock &other) const;
   bool operator< (const Vclock &other) const;
   bool operator> (const Vclock &other) const;
   void operator= (const Vclock &other);
   std::pair<int, int> operator[] (int tid);

   void add_clock(int pid, int count);
   void inc_clock(int pid);
   int get_size()const;
   void print();
   std::string print_dot();

private:
   std::vector<std::pair<int,int> > tab;
   uint64_t mask;
};


} // namespace dpu

#endif
