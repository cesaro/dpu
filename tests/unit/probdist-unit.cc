
#include "gtest/gtest.h"

#include "probdist.hh"

TEST (ProbdistTest, Integers)
{
   Probdist<int> p;

   ASSERT_EQ (p.size(), 0);
   ASSERT_EQ (p.counters.size(), 0);
   ASSERT_EQ (p.count (12), 0);

   p.clear ();
   p.sample (4);
   p.sample (4);
   p.sample (5);

   ASSERT_EQ (p.size(), 3);
   ASSERT_EQ (p.counters.size(), 2);
   ASSERT_EQ (p.count (12), 0);
   ASSERT_EQ (p.count (4), 2);
   ASSERT_EQ (p.count (5), 1);
   ASSERT_EQ (p.mass (12), 0);
   ASSERT_EQ (p.mass (5), 1 / (float) 3);
   ASSERT_EQ (p.mass (4), 2 / (float) 3);

   p.sample (20);
   p.sample (20);
   p.sample (2);
   ASSERT_EQ (p.size(), 6);
   ASSERT_EQ (p.mass (2), 1 / (float) p.size());
   ASSERT_EQ (p.mass (20), 2 / (float) p.size());

   ASSERT_EQ (p.min (), 2);
   ASSERT_EQ (p.max (), 20);
   ASSERT_EQ (p.average (), 55 / (float) p.size());

   Probdist<void*> q;

   q.sample (&p);
   q.sample (&p);
   q.sample (&q);
   q.sample (&q);
   ASSERT_EQ (q.size(), 4);
   ASSERT_EQ (q.mass (&p), 0.5);
   ASSERT_EQ (q.mass (&q), 0.5);

   // this could be correct or not depending on where thec compiler places p and
   // q in memory; remove it if it fails for you
   ASSERT_EQ (q.min (), &q);
   ASSERT_NE (q.max (), &q);
   ASSERT_EQ (q.max (), &p);
}
