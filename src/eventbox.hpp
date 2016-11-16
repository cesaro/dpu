
inline EventBox::EventBox (Event *pre) :
   _pre (pre)
{
   DEBUG ("EventBox.ctor: this %p pid %u pre %p sizeof %zu",
         this, pid(), _pre, sizeof (EventBox));
}

inline Event *EventBox::event_above () const
{
   return (Event*) (this + 1);
}
inline Event *EventBox::event_below () const
{
   return ((Event *) this) - 1;
}

Event *EventBox::pre () const
{
   return _pre;
}

inline unsigned EventBox::pid () const
{
   return Unfolding::ptr2pid (this);
}

