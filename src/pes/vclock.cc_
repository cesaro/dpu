/*
 * Vclock.cc
 *
 *  Created on: Nov 15, 2016
 *      Author: tnguyen
 */

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <utility>
#include <memory>
#include <algorithm>
#include <inttypes.h>

#include "vclock.hh"
#include "verbosity.h"
#include "misc.hh"

namespace dpu {

Vclock::Vclock()
{
   tab.clear();
   mask = 0;
}

Vclock::Vclock(unsigned tid, int count)
:mask(0)
{
   tab.push_back(std::make_pair(tid,count));
   mask |= (((uint64_t) 1) << tid);
}

Vclock::Vclock (const Vclock &v)
:tab(v.tab),
 mask(v.mask)
{
}

unsigned int countSetBits(unsigned int n)
{
  unsigned int count = 0;
  while(n)
  {
    count += n & 1;
    n >>= 1;
  }
  return count;
}

Vclock::Vclock (const Vclock &v1, const Vclock &v2) :
   tab(v1.tab),
   mask(v1.mask)
{
   unsigned tid, size;
   if ((mask & v2.mask) == 0) // v1 has no shared component with v2
   {
      size = tab.size() + v2.tab.size();
      tab.reserve(size);
      tab.insert(tab.end(), v2.tab.begin(), v2.tab.end());
      mask += v2.mask;
   }
   else
   {
      int difbits = mask ^ v2.mask; // XOR two masks
      if (difbits == 0) // all threads are the same in both vectors
      {
         for (unsigned int i = 0; i < v2.tab.size(); i++)
         for (unsigned int j = 0; j < tab.size(); j++)
            if (tab[j].first == v2.tab[i].first)
               tab[j].second = std::max(tab[j].second, v2.tab[i].second);
      }
      else
         for (unsigned int i = 0; i < v2.tab.size(); i++)
         {
            tid = v2.tab[i].first;
            if ((mask & (((uint64_t) 1) << tid)) != 0) // tid is a shared thread
            {
               for (unsigned int j = 0; j < tab.size(); j++)
                  if (tab[j].first == tid)
                     tab[j].second = std::max(tab[j].second, v2.tab[i].second);
            }
            else // new thread to add to v
            {
               tab.push_back(std::make_pair(tid,v2.tab[i].second));
               this->mask += (((uint64_t) 1) << tid);
            }
         }
   }
}


//----------------------
bool Vclock::operator== (const Vclock &other) const
{
   if (tab.size() != other.tab.size())    return false;
   if ((this->mask ^ other.mask) != 0)    return false;
   // here, they have same size, same mask
   for (unsigned i = 0; i < tab.size(); i++)
   {
      for (unsigned j = 0; j < other.tab.size(); j++)
      {
         if ((tab[i].first == other.tab[j].first) && (tab[i].second != other.tab[j].second))
            return false;
      }
   }
   return true;
}

bool Vclock::operator< (const Vclock &other) const
{
   int tid, count;
   if (*this == other) return false;
   if ((mask & other.mask) == 0) // no shared component
      return false; // cannot compare
   // check if this is a subset of other
   // using masks: A & (A XOR B) = 0 => A is a subset of B
   if ((mask & (mask ^ other.mask)) == 0) // A is subset of B
   {
      for (unsigned int i = 0; i < tab.size(); i++)
      {
         tid   = tab[i].first;
         count = tab[i].second;
         for (unsigned j = 0; j < other.tab.size(); j++)
            if ( (tid == other.tab[j].first) & (count > other.tab[j].second ))
               return false;
      }
      return true;
   }

   return false;
}

bool Vclock::operator> (const Vclock &other) const
{
   int tid, count;
   if (*this == other) return false;
   if ((mask & other.mask) == 0) // no shared component
      return false; // cannot compare
   // check if other is a subset of this

   if ((other.mask & (mask ^ other.mask)) == 0) // other is subset of this
   {
      //printf("other is a subset of this");
      for (unsigned int i = 0; i < tab.size(); i++)
      {
         tid   = tab[i].first;
         count = tab[i].second;
         for (unsigned j = 0; j < other.tab.size(); j++)
            if ( (tid == other.tab[j].first) & (count < other.tab[j].second ))
               return false;
      }
      return true;
   }

   return false;
}

void Vclock::operator= (const Vclock &other)
{
   mask  =  other.mask;
   tab   =  other.tab;
}
//---------------
Vclock Vclock::operator+ (const Vclock &other)
{
   Vclock sum(*this);
   unsigned tid, size;
   if ((mask & other.mask) == 0) // this has no shared component with other
   {
      size = tab.size() + other.tab.size();
      sum.tab.reserve(size);
      sum.tab.insert(sum.tab.end(), other.tab.begin(), other.tab.end());
      sum.mask += other.mask;
   }
   else
   {
      int difbits = sum.mask ^ other.mask; // XOR two masks
      if (difbits == 0) // all threads are the same in both vectors
      {
         for (unsigned int i = 0; i < other.tab.size(); i++)
         for (unsigned int j = 0; j < sum.tab.size(); j++)
            if (sum.tab[j].first == other.tab[i].first)
               sum.tab[j].second = std::max(sum.tab[j].second, other.tab[i].second);
      }
      else
         for (unsigned int i = 0; i < other.tab.size(); i++)
         {
            tid = other.tab[i].first;
            if ((sum.mask & (((uint64_t) 1) << tid)) != 0) // tid is a shared thread
            {
               for (unsigned int j = 0; j < sum.tab.size(); j++)
                  if (sum.tab[j].first == tid)
                     sum.tab[j].second = std::max(sum.tab[j].second, other.tab[i].second);
            }
            else // new thread to add to v
            {
               sum.tab.push_back(std::make_pair(tid, other.tab[i].second));
               sum.mask += (((uint64_t) 1) << tid);
            }
         }
   }
   return sum;

}
//-========================
int Vclock:: operator[] (unsigned pid)
{
   ASSERT(mask && ((uint64_t)1 << pid));
   for (unsigned i = 0; i < tab.size(); i++)
      if (tab[i].first == pid)
         return tab[i].second;

   return -1;
}
//-----------------
void Vclock::add_clock(unsigned pid, int count)
{
   tab.push_back(std::make_pair(pid,count));
   mask |= ((uint64_t)1 << pid);
}
//-----------------
void Vclock::inc_clock(unsigned pid)
{
   //ASSERT(mask && ((uint64_t)1 << pid));
   for (unsigned int i = 0; i < tab.size(); i++)
      if (tab[i].first == pid)
         tab[i].second++;
}
//-----------------
int Vclock::get_size() const
{
   return tab.size();
}
//-----------------

void Vclock::print()
{
   printf("\ntid \t counter \n");
   for (unsigned i = 0; i < tab.size(); i++)
      printf("%d \t %d \n", tab[i].first, tab[i].second);

   printf("mask = %ju \n",mask);
}
//-------------
std::string Vclock::print_dot()
{
   std::string st;
   for (unsigned int i = 0; i < tab.size(); i++)
      st += std::to_string(tab[i].first) + "-" + std::to_string(tab[i].second) + " , ";
   return st;
}

} // end of namespace





