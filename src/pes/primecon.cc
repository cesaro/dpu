
#include "pes/primecon.hh"
#include "pes/event.hh"
#include "pes/config.hh"

#include "config.h"

namespace dpu
{

int __cmp (const Event *e1, const Event *e2)
{
   ASSERT (e1->action.type == ActionType::MTXLOCK or
         e1->action.type == ActionType::MTXUNLK);
   ASSERT (e2->action.type == ActionType::MTXLOCK or
         e2->action.type == ActionType::MTXUNLK);

   return e1->action.addr - e2->action.addr;
}

void merge_2ways (std::vector<const Event*> &dst,
      const std::vector<const Event*> &src, Event *e)
{
   unsigned i, j;
   int cmp;
   
   ASSERT (e);
   ASSERT (e->action.type == ActionType::MTXLOCK or
         e->action.type == ActionType::MTXUNLK);

   dst.resize (src.size() + 1);

   for (i = 0, j = 0; i < src.size(); i++, j++)
   {
      cmp = __cmp (e, src[i]);
      if (cmp == 0)
      {
         ASSERT (e->node[1].depth > src[i]->node[1].depth);
         i++;
         break;
      }
      if (cmp < 0) break;
      dst[j] = src[i];
   }

   // insert e
   dst[j] = e;
   j++;

   // insert the remaining pointers from src
   for (; i < src.size(); i++, j++) dst[j] = src[i];
   dst.resize (j);
}

void merge_3ways (std::vector<const Event*> &dst,
      const std::vector<const Event*> &src1,
      const std::vector<const Event*> &src2,
      Event *e3)
{
   unsigned i, j, k;
   const Event *e = nullptr;;

   i = j = k = 0;

   while (1)
   {
      // get a pointer to the smallest element
      if (i < src1.size()) {
         if (j < src2.size()) {
            if (__cmp (src1[i], src2[j]) <= 0) e = src1[i]; else e = src2[j];
            if (e3 and __cmp (e3, e) < 0) e = e3;
         } else {
            if (e3) {
               if (__cmp (e3, src1[i]) < 0) e = e3; else e = src1[i];
            } else {
               e = src1[i];
            }
         }
      } else {
         if (j < src2.size()) {
            if (e3) {
               if (__cmp (e3, src2[j]) <= 0) e = e3; else e = src2[j];
            } else {
               e = src2[j];
            }
         } else {
            if (!e3) break;
            e = e3;
         }
      }

      // advance every input iterator that contains the selected address and get
      // the deepest representative of the address
      if (i < src1.size() and __cmp (e, src1[i]) == 0) {
         if (e->node[1].depth < src1[i]->node[1].depth) e = src1[i];
         i++;
      }
      if (j < src2.size() and __cmp (e, src2[j]) == 0) {
         if (e->node[1].depth < src2[j]->node[1].depth) e = src2[j];
         j++;
      }
      if (e3 and __cmp (e, e3) == 0) {
         if (e->node[1].depth < e3->node[1].depth) e = e3;
         e3 = nullptr;
      }

      // insert the event in the destination vector
      dst.push_back (e);
   }
}

Primecon::Primecon (unsigned n, Event *e) :
   Cut (n, e),
   lockmax () // empty
{
   ASSERT (e->action.type != ActionType::MTXLOCK);
   ASSERT (e->action.type != ActionType::MTXUNLK);
}

Primecon::Primecon (const Primecon &p, Event *e) :
   Cut (p, e),
   lockmax ()
{
   if (e->action.type == ActionType::MTXLOCK or
         e->action.type == ActionType::MTXUNLK)
   {
      merge_2ways (lockmax, p.lockmax, e);
   }
   else
   {
      lockmax = p.lockmax;
   }
}

Primecon::Primecon (const Primecon &p, const Primecon &m, Event *e) :
   Cut (p, m, e),
   lockmax ()
{
   if (e->action.type != ActionType::THJOIN)
   {
      // that was the fastest way to check if e is lock/unlock :)
      ASSERT (e->action.type == ActionType::MTXLOCK or
            e->action.type == ActionType::MTXUNLK);
      merge_3ways (lockmax, p.lockmax, m.lockmax, e);
   }
   else
   {
      merge_3ways (lockmax, p.lockmax, m.lockmax, nullptr);
   }
}

bool Primecon::in_cfl_with (const Primecon *other) const
{
   // two Primeconfigs are in conflict iff we can find one maximal event in each
   // lockmax array (sets of maximal lock/unlock events) that correspond to the
   // same address and are in conflict

   decltype(lockmax)::const_iterator it = lockmax.begin();
   decltype(other->lockmax)::const_iterator it_ = other->lockmax.begin();

#ifdef CONFIG_STATS_DETAILED
   Event::counters.conflict.trivial_empty++;
#endif
   // if one of the vectors is empty, there is no conflict
   if (it == lockmax.end()) return false;
   if (it_ == other->lockmax.end()) return false;
#ifdef CONFIG_STATS_DETAILED
   Event::counters.conflict.trivial_empty--;
#endif
   
   while (1)
   {
      // both vectors have at least one element; if the addresses are equal,
      // we check for conflicts here
      if ((*it)->action.addr == (*it_)->action.addr)
      {
         // if there is one, we are in conflict
         if ((*it)->node[1].in_cfl_with<1> (*it_)) return true;

         // if not, we advance both iterators, checking for the end of vector
         it++; it_++;
         if (it == lockmax.end()) return false;
         if (it_ == other->lockmax.end()) return false;
      }
      else
      {
         // otherwise we advance the iterator of the lowest address; if we find
         // the end of the vector, there is no conflict
         if (__cmp (*it, *it_) < 0)
         {
            it++;
            if (it == lockmax.end()) return false;
         }
         else
         {
            it_++;
            if (it_ == other->lockmax.end()) return false;
         }
      }
   }
}

bool Primecon::in_cfl_with (const Config &c) const
{
   // We need to return false iff this configuration union c is conflict-free.
   // The union is conflict-free iff for every "sequential tree" of the
   // unfolding (one per process, one per lock) the union contains only a
   // sequence and no two forking branches. Processes never branch on their own,
   // so we only need to check for forks (conflicts) in the lock trees.
   // So for each lock in this.lockmax we find the corresponding event in
   // c.mutexmax and test for conflicts in their lock-tree.
   
   const Event *ee;

   for (const Event *e: lockmax)
   {
      ee = c.mutex_max (e->action.addr);
      if (! ee) continue;
      if (e->node[1].in_cfl_with<1> (ee)) return true;
   }
   return false;
}

void Primecon::dump () const
{
   PRINT ("== begin cone =="); 
   __dump_cut ();
   __dump_lockmax ();
   PRINT ("== end cone =="); 
}

void Primecon::__dump_lockmax () const
{
   for (const Event *e : lockmax)
   {
      PRINT ("Addr %16p max %s",
            (void*) e->action.addr, e->str().c_str());
   }
}

std::string Primecon::str () const
{
   std::string s;
   unsigned i;

   // [0: 0x123123; 1: 0x12321; others: 0 || 0x123213: 0x123213]
   s = "[";
   for (i = 0; i < nrp; i++)
   {
      if (max[i]) s += fmt ("%u: %p; ", i, max[i]);
   }
   if (s.size() >= 2)
   {
      s.pop_back();
      s.pop_back();
   }
   s += " || ";

   for (const Event *e : lockmax)
   {
      s += fmt ("%p: %p; ", e->action.addr, e);
   }
   s.pop_back();
   s.pop_back();
   return s + "]";
}

const Event *Primecon::mutex_max (Addr a) const
{
   // FIXME - substitute this by a binary search!
   for (const Event *e : lockmax)
   {
      if (e->action.addr == a) return e;
      if (e->action.addr > a) return 0; // because it's sorted!
   }
   return 0;
}

Event *Primecon::mutex_max (Addr a)
{
   return const_cast<Event*> (const_cast<const Primecon*>(this)->mutex_max (a));
}

} // namespace dpu

