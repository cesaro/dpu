
#include "gtest/gtest.h"

#include "redbox.hh"
#include "redbox-factory.hh"

TEST (RedboxFactoryTest, test)
{
   dpu::RedboxFactory r;
   dpu::Redbox *b;

   // the {read,write}_regions are private, to test this you need to temporarily
   // make them public
#if 0
   ASSERT_EQ (r.read_regions.size(), 0);
   ASSERT_EQ (r.write_regions.size(), 0);

   ASSERT_TRUE (r.read_regions.empty ());
   ASSERT_TRUE (r.write_regions.empty ());
#endif

   r.add_read (0x41, 2);
   r.add_read (0x43, 32);

   r.add_read (0x22, 2);
   r.add_read (0x24, 2);
   r.add_read (0x21, 20);
   r.add_read (0x21, 8);
   r.add_read (0x21, 2);

   r.add_read (0x10, 1);
   r.add_read (0x10, 1);
   r.add_read (0x11, 2);

   r.add_write (0x0, 4);
   r.add_write (0x4, 4);
   r.add_write (0x8, 4);

   r.add_write (0x100, 1);
   r.add_write (0x101, 1);
   r.add_write (0x101, 4);
   r.add_write (0x105, 2);

   r.add_write (0x110, 2);

   b = r.create ();

   //b->dump ();
   ASSERT_EQ (b->readpool.size(), 3);
   ASSERT_EQ (b->writepool.size(), 3);
}

TEST (RedboxFactoryTest, overlap1)
{
   dpu::MemoryRegion<dpu::Addr> inter;
   dpu::RedboxFactory r;
   dpu::Redbox *b;

   // all regions are non-adjacent
   r.add_read (0x10, 6);
   r.add_read (0x20, 2);
   r.add_read (0x30, 0xf);
   r.add_read (0x50, 4); // overlaps
   r.add_read (0x90, 1);
   r.add_read (0xa0, 2);

   r.add_write (0x40, 4);
   r.add_write (0x52, 7); // overlaps
   r.add_write (0x60, 4);

   b = r.create ();

   //b->dump ();
   ASSERT_EQ (b->readpool.size(), 6);
   ASSERT_EQ (b->writepool.size(), 3);

   ASSERT_TRUE (b->readpool.overlaps (b->writepool, inter));
   ASSERT_EQ (inter.lower, 0x52);
   ASSERT_EQ (inter.upper, 0x54);
   ASSERT_EQ (inter.size(), 2);

   ASSERT_TRUE (b->writepool.overlaps (b->readpool, inter));
   ASSERT_EQ (inter.lower, 0x52);
   ASSERT_EQ (inter.upper, 0x54);
   ASSERT_EQ (inter.size(), 2);
}

TEST (RedboxFactoryTest, overlap2)
{
   dpu::MemoryRegion<dpu::Addr> inter;
   dpu::RedboxFactory r;
   dpu::Redbox *b;

   // all regions are non-adjacent
   r.add_read (0x10, 6);
   r.add_read (0x30, 0xf);
   r.add_read (0x90, 1);
   r.add_read (0xa0, 2);

   r.add_write (0x20, 4);
   r.add_write (0x52, 7);
   r.add_write (0x60, 4);
   r.add_write (0xb0, 4);

   b = r.create ();

   //b->dump ();
   ASSERT_EQ (b->readpool.size(), 4);
   ASSERT_EQ (b->writepool.size(), 4);

   ASSERT_FALSE (b->readpool.overlaps (b->writepool, inter));
   ASSERT_EQ (inter.size(), 0);

   ASSERT_FALSE (b->writepool.overlaps (b->readpool, inter));
   ASSERT_EQ (inter.size(), 0);
}

TEST (RedboxFactory, empty)
{
   dpu::RedboxFactory r;
   dpu::Redbox *b;
   b = r.create ();
   b->dump ();
   ASSERT_EQ (b->readpool.size(), 0);
   ASSERT_EQ (b->writepool.size(), 0);
}

