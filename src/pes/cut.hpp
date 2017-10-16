

Cut::Cut (unsigned n) :
   nrp (n),
   max (new Event* [nrp])
{
   unsigned i;
   //DEBUG ("Cut.ctor: this %p nrp %d", this, nrp);
   ASSERT (nrp >= 1);

   // initialize the size elements of the vector to null
   for (i = 0; i < nrp; i++)
      max[i] = nullptr;
}

/// copy constructor
Cut::Cut (const Cut &other) :
   nrp (other.nrp),
   max (new Event* [nrp])
{
   //DEBUG ("Cut.ctor: this %p other %p nrp %d (copy)", this, &other, nrp);
   // copy the pointers
   memcpy (max, other.max, nrp * sizeof (Event*));
}

// Cut::Cut (const Cut &c1, const Cut &c2) in pes/cut.cc

Cut::Cut (unsigned n, Event *e) :
   Cut (n)
{
   fire (e);
}

// Cut::Cut (const Cut &other, Event *e) in pes/cut.cc

Cut::Cut (const Cut &c1, const Cut &c2, Event *e) :
   Cut (c1, c2)
{
   fire (e);
}

Cut::~Cut ()
{
   // delete the memory in the vector
   //DEBUG ("Cut.dtor: this %p nrp %d", this, nrp);
   delete[] max;
}

Cut & Cut::operator= (const Cut & other)
{
   //DEBUG("Cut.op= this %p other %p", this, &other);
   if (nrp < other.nrp)
   {
      delete[] max;
      max = new Event* [other.nrp];
   }
   nrp = other.nrp;
   memcpy (max, other.max, nrp * sizeof (Event*));
   return *this;
}

Cut & Cut::operator= (Cut && other)
{
   //DEBUG("Cut.op= this %p other %p (move)", this, &other);
   nrp = other.nrp;
   max = other.max;
   other.max = nullptr; // only suitable for destruction
   return *this;
}

// void Cut::fire (Event *e) in pes/cut.cc
// void Cut::unfire (Event *e) in pes/cut.cc

void Cut::clear ()
{
   unsigned i;
   for (i = 0; i < nrp; i++) max[i] = 0;
}

Event *Cut::operator[] (unsigned pid) const
{
   return pid < nrp ? max[pid] : nullptr;
}

Event *&Cut::operator[] (unsigned pid)
{
   ASSERT (pid < nrp);
   return max[pid];
}

unsigned Cut::num_procs() const
{
   return nrp;
}

bool Cut::empty() const
{
   for (unsigned i = 0; i < nrp; i++)
      if (max[i] != nullptr)
         return false;
   return true;
}

bool Cut::enabled (const Event *e) const
{
   // unimplemented
   ASSERT (0);
   return false;
}

bool Cut::extension (const Event *e) const
{
   // unimplemented
   ASSERT (0);
   return false;
}

bool Cut::cex (const Event *e) const
{
   // unimplemented
   ASSERT (0);
   return false;
}

/// returns the memory size of the data pointed by fields in this object
inline size_t Cut::pointed_memory_size () const
{
   return nrp * sizeof (Event*);
}

