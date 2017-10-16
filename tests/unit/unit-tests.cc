
#include <stdio.h>

#include "verbosity.h"
#include "gtest/gtest.h"

namespace dpu {

void test1 ();
void test2 ();
void test3 ();
void test4 ();
void test5 ();
void test6 ();
void test7 ();
void test8 ();
void test9 ();

void test10 ();
void test11 ();
void test12 ();
void test13 ();
void test14 ();
void test15 ();
void test16 ();
void test17 ();
void test18 ();
void test19 ();

void test20 ();
void test21 ();
void test22 ();
void test23 ();
void test24 ();
void test25 ();
void test26 ();
void test27 ();
void test28 ();
void test29 ();

void test30 ();
void test31 ();
void test32 ();
void test33 ();
void test34 ();
void test35 ();
void test36 ();
void test37 ();
void test38 ();
void test39 ();

void test40 ();
void test41 ();
void test42 ();
void test43 ();
void test44 ();
void test45 ();
void test46 ();
void test47 ();
void test48 ();
void test49 ();

void test50 ();
void test51 ();
void test52 ();
void test53 ();
void test54 ();
void test55 ();
void test56 ();
void test57 ();
void test58 ();
void test59 ();

} // namespace

int main (int argc, char **argv)
{
   verb_set (VERB_DEBUG);
#if 0
   dpu::test57 ();
   return 0;
#else
   ::testing::InitGoogleTest(&argc, argv);
   return RUN_ALL_TESTS();
#endif
}

