
/// returns the memory size of the data pointed by fields in this object
size_t Primecon::pointed_memory_size () const
{
   return Cut::pointed_memory_size() + lockmax.capacity() * sizeof(Event*);
}

template<typename T>
void Primecon::dump () const
{
   PRINT ("== begin cone ==");
   __dump_cut<T> ();
   __dump_lockmax<T> ();
   PRINT ("== end cone ==");
}

template<typename T>
void Primecon::__dump_lockmax () const
{
   for (const Event *e : lockmax)
   {
      PRINT ("Addr %16p max %s",
            (void*) e->action.addr, e->str().c_str());
   }
}
