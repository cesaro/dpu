
inline bool operator< (const MemoryPool &a, const MemoryPool &b)
{
   return a.upper() < b.lower();
}

inline bool operator> (const MemoryPool &a, const MemoryPool &b)
{
   return b < a;
}

bool MemoryPool::overlaps (const MemoryPool &other) const
{
   const_iterator it1 (begin());
   const_iterator it2 (other.begin());
   const_iterator end1 (end());
   const_iterator end2 (other.end());

   // if any of the two is empty, then the answer is no
   if (it1 == end1) return false;
   if (it2 == end2) return false;

   // if they are clearly disjoint, the answer is no
   if (*this < other or other < *this) return false;

   // otherwise the range of memory regions overlap and we need to check the
   // underlying MemoryRegions
   while (true)
   {
      // fast forward it1
      while (true)
      {
         if (it1 == end1) return false;
         if (! (*it1 < *it2)) break;
         ++it1;
      }

      // fast forward it2
      while (true)
      {
         if (it2 == end2) return false;
         if (! (*it2 < *it1)) break;
         ++it2;
      }
      // check if the two memory regions overlap
      if (it1->overlaps (*it2)) return true;
   }
   return false;
}
