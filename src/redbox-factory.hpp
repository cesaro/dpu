
void RedboxFactory::add (Addr addr, ActionType t)
{
   switch (t)
   {
   case ActionType::RD8 :
      return add_read (addr, 1);
   case ActionType::RD16 :
      return add_read (addr, 2);
   case ActionType::RD32 :
      return add_read (addr, 4);
   case ActionType::RD64 :
      return add_read (addr, 8);
   case ActionType::WR8 :
      return add_write (addr, 1);
   case ActionType::WR16 :
      return add_write (addr, 2);
   case ActionType::WR32 :
      return add_write (addr, 4);
   case ActionType::WR64 :
      return add_write (addr, 8);
   default :
      SHOW (t, "d");
      ASSERT (0);
   }
}

void RedboxFactory::add_read (Addr addr, unsigned size)
{
   ASSERT (size);
   read_regions.emplace_back (addr, size);
}

void RedboxFactory::add_write (Addr addr, unsigned size)
{
   ASSERT (size);
   write_regions.emplace_back (addr, size);
}

void RedboxFactory::clear ()
{
   read_regions.clear ();
   write_regions.clear ();
}

Redbox *RedboxFactory::create ()
{
   Redbox *b;

   // Special case: we return a null pointer if both memory pools are empty.
   // In this case the event will continue being labelled by a null pointer
   // rather than a pointer to a Redbox. This is useful during the data race
   // detection in DataRaceAnalysis::find_data_races(), because that way we
   // don't even need to look inside of the red box to see if it has events, the
   // event will straightfowardly be discarde for DR detection
   if (read_regions.empty() and write_regions.empty()) return nullptr;

   // allocate a new Redbox and keep the pointer to it, we are the container
   b = new Redbox ();
   boxes.push_back (b);
   
   // compress the lists of memory areas
   compress (read_regions);
   compress (write_regions);

   // copy them to the new redbox
   b->readpool = read_regions;
   b->writepool = write_regions;

#ifdef CONFIG_DEBUG
   if (verb_debug) b->dump ();
   // this will assert that the memory pools are a sorted sequence of disjoint
   // memory areas
   b->readpool.assertt ();
   b->writepool.assertt ();
#endif

   // restart the internal arrays
   read_regions.clear ();
   write_regions.clear ();
   ASSERT (empty());
   return b;
}

void RedboxFactory::compress (MemoryPool::Container &regions)
{
   unsigned i, j;
   size_t s;

   // nothing to do if we have no regions; code below assumes we have at least 1
   if (regions.empty ()) return;

   // sort the memory regions by increasing value of lower bound
   struct compare {
      bool operator() (const MemoryRegion<Addr> &a, const MemoryRegion<Addr> &b)
      {
         return a.lower < b.lower;
      }
   } cmp;
   std::sort (regions.begin(), regions.end(), cmp);

   // compress regions
   s = regions.size ();
   breakme ();
   for (i = 0, j = 1; j < s; ++j)
   {
      ASSERT (i < j);
      ASSERT (regions[i].lower <= regions[j].lower);

      // if the next region's lower bound is below i's region upper bound, we can
      // extend i's range
      if (regions[i].upper >= regions[j].lower)
      {
         regions[i].upper = std::max (regions[i].upper, regions[j].upper);
      }
      else
      {
         // otherwise there is a gap betwen interval i and interval j, so we
         // need to create a new interval at offset i+1, only if i+1 != j
         ++i;
         if (i != j)
         {
            // copy j into i
            regions[i] = regions[j];
         }
      }
   }
   DEBUG ("redbox-factory: compressed %zu regions into %u", regions.size(), i+1);
   regions.resize (i + 1);
}
