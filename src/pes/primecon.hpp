// nothing here so far

/// returns the memory size of the data pointed by fields in this object
size_t Primecon::pointed_memory_size () const
{
   return Cut::pointed_memory_size() + lockmax.capacity() * sizeof(Event*);
}

