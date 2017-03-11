#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <array>
#include <memory>
#include <algorithm>
#include <sys/stat.h>

#include "verbosity.h"
#include "misc.hh"
#include "pes/event.hh"
#include "pes/unfolding.hh"
#include "pes/process.hh"
#include "pes/eventbox.hh"
#include "pes/config.hh"
#include "pes/cut.hh"

#include "gtest/gtest.h"

TEST (Unfolding, Addressing)
{
  EXPECT_EQ(1, 2);
  EXPECT_GT(10, -2);
  EXPECT_FALSE(true);
}
