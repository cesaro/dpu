
Config::Config (const Unfolding &u) :
   Config (u.num_procs())
{
}

Config::Config (unsigned n) :
   Cut (n),
   mutexmax ()
{
}

/// copy constructor
Config::Config (const Config &other) :
   Cut (other),
   mutexmax (other.mutexmax)
{
   //DEBUG ("Config.ctor: this %p other %p (copy)", this, &other);
}

/// assignment operator
Config & Config::operator= (const Config & other)
{
   //DEBUG("Config.op=: this %p other %p", this, &other);
   Cut::operator= (other);
   mutexmax = other.mutexmax;
   return *this;
}

/// move-assignment operator
Config & Config::operator= (Config && other)
{
   //DEBUG("Config.op=: this %p other %p (move)", this, &other);
   Cut::operator= (std::move (other));
   mutexmax = std::move (other.mutexmax);
   return *this;
}

void Config::fire (Event *e)
{
   //DEBUG("Config.fire: this %p e %p e.pid %d", this, e, e->pid());

   Cut::fire (e);
   switch (e->action.type)
   {
   case ActionType::MTXLOCK   :
   case ActionType::MTXUNLK   :
      ASSERT (mutex_max (e->action.addr) == e->pre_other());
      mutexmax[e->action.addr] = e;
      break;

   default :
      break;
   }
}

void Config::unfire (Event *e)
{
   Cut::unfire (e);
   switch (e->action.type)
   {
   case ActionType::MTXLOCK :
   case ActionType::MTXUNLK :
      ASSERT (mutex_max (e->action.addr) == e);
      if (! e->pre_other())
         mutexmax.erase  (e->action.addr);
      else
         mutexmax[e->action.addr] = e->pre_other();
      break;

   default :
      // locks/unlocks should be the only ones that create immediate conflicts
      ASSERT (e->icfl_count() == 0);
      break;
   }
}

void Config::clear ()
{
   Cut::clear ();
   mutexmax.clear ();
}

Event *Config::proc_max (unsigned pid)
{
   return (*this)[pid]; // inherited Cut::operator[]
}

const Event *Config::mutex_max (Addr a) const
{
   auto it = mutexmax.find (a);
   return it == mutexmax.end() ? nullptr : it->second;
}

Event *Config::mutex_max (Addr a)
{
   return const_cast<Event*> (const_cast<const Config*>(this)->mutex_max (a));
}
