
Eventbox::Eventbox (Event *pre) :
   _pre (pre)
{
   //DEBUG ("Eventbox.ctor: this %p pid %u pre %p sizeof %zu",
   //      this, pid(), _pre, sizeof (Eventbox));
}

Event *Eventbox::event_above () const
{
   return (Event*) (this + 1);
}

// Event *Eventbox::event_below () const in pes/eventbox.cc

Event *Eventbox::pre () const
{
   return _pre;
}

