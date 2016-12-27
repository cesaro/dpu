
#ifndef __VCLOCK_HH_
#define __VCLOCK_HH_

#include <vector>

namespace dpu
{

class Vclock
{
public:
   Vclock();
   Vclock(unsigned pid, int count);
   Vclock (const Vclock &v); //copy constructor
   Vclock (const Vclock &v1, const Vclock &v2);

   bool operator== (const Vclock &other) const;
   bool operator< (const Vclock &other) const;
   bool operator> (const Vclock &other) const;
   void operator= (const Vclock &other);
   Vclock operator+ (const Vclock &other);
   int operator[] (unsigned tid);

   void add_clock(unsigned pid, int count);
   void inc_clock(unsigned pid);
   int get_size()const;
   void print();
   std::string print_dot();

private:
   std::vector<std::pair<unsigned,int> > tab;
   uint64_t mask;
};


} // namespace dpu

#endif
