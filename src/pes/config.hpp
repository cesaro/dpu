
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
   DEBUG ("Config.ctor: this %p other %p (copy)", this, &other);
}

/// assignment operator
Config & Config::operator= (const Config & other)
{
   DEBUG("Config.op=: this %p other %p", this, &other);
   Cut::operator= (other);
   mutexmax = other.mutexmax;
   return *this;
}

/// move-assignment operator
Config & Config::operator= (Config && other)
{
   DEBUG("Config.op=: this %p other %p (move)", this, &other);
   Cut::operator= (std::move (other));
   mutexmax = std::move (other.mutexmax);
   return *this;
}

void Config::add (Event *e)
{
   DEBUG("Config.add: this %p e %p e.pid %d", this, e, e->pid());
   ASSERT (e);
   ASSERT (e->pid() < nrp);

   // the unfolding might have changed the number of process after this
   // configuration was constructed; assert it didn't happen
   ASSERT (e->pid() < nrp);

   // pre-proc must be the event max[e.pid()]
   ASSERT (e->pre_proc() == max[e->pid()]);
   // the pid() and the address of the event need to be different
   if (e->action.type == ActionType::MTXLOCK or e->action.type == ActionType::MTXUNLK)
      ASSERT (e->pid() < e->action.addr);

   // update tables
   max[e->pid()] = e;
   switch (e->action.type)
   {
   case ActionType::THCREAT   :
      ASSERT (mutex_max (e->action.val) == 0);
      mutexmax[e->action.val] = e;
      break;

   case ActionType::THEXIT    :
      ASSERT (! e->pre_other());
      if (e->pid() != 0)
      {
         // for all but the first thread, we keep track of the creat/exit
         // actions in the mutexmax table using the pid as an address
         ASSERT (mutex_max (e->pid()));
         ASSERT (mutex_max (e->pid())->action.type == ActionType::THCREAT);
         mutexmax[e->pid()] = e;
      }
      break;

   case ActionType::MTXLOCK   :
   case ActionType::MTXUNLK   :
      //DEBUG("mutex_max: %p, pre_other: %p",mutex_max (e->action.addr), e->pre_other());
      ASSERT (mutex_max (e->action.addr) == e->pre_other());
      mutexmax[e->action.addr] = e;
      break;

   case ActionType::THJOIN   :
      ASSERT (e->pre_other());
      ASSERT (e->pre_other()->action.type == ActionType::THEXIT);
      ASSERT (e->pre_other()->pid() == e->action.val);
   
   default :
      break;
   }
}

void Config::clear ()
{
   Cut::clear ();
   mutexmax.clear();
}



Event *Config::proc_max (unsigned pid)
{
   return (*this)[pid]; // inherited Cut::operator[]
}

Event *Config::mutex_max (Addr a)
{
   auto it = mutexmax.find (a);
   return it == mutexmax.end() ? 0 : it->second;
}

