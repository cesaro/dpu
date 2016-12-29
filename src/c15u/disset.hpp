
Disset::Disset () :
   stack (),
   just (nullptr),
   unjust (nullptr),
   top_disabler (-1),
   top_idx (-1)
{
}

void Disset::just_push (Elem *e)
{
   ASSERT (e);
   e->next = just;
   just = e;
}

Disset::Elem *Disset::just_pop ()
{
   Elem *e;

   ASSERT (just);
   e = just;
   just = just->next;
   return e;
}

bool Disset::just_isempty ()
{
   return just == nullptr;
}

Disset::Elem *Disset::just_peek ()
{
   ASSERT (just);
   return just;
}

void Disset::unjust_add (Elem *e)
{
   // we add e to the head of the list

   ASSERT (e);
   e->next = unjust;
   e->prev = nullptr;
   if (unjust) unjust->prev = e;
   unjust = e;
}

void Disset::unjust_remove (Elem *e)
{
   // we extract e from the list of unjustified events

   ASSERT (e);
   ASSERT (unjust);
   if (e->prev)
      e->prev->next = e->next;
   else
   {
      ASSERT (unjust == e);
      unjust = e->next;
   }
   if (e->next) e->next->prev = e->prev;
}

bool Disset::unjust_isempty ()
{
   return unjust == nullptr;
}

void Disset::add (Event *e, int idx)
{
   // we add e to the stack, add it to the list of unjustified events and update
   // the top_idx variable

   ASSERT (e);
   ASSERT (idx >= 0);
   ASSERT (idx >= top_idx);

   stack.push_back ({.e = e, .idx = idx, .disabler = 0});
   unjust_add (&stack.back());
   top_idx = idx;
}

void Disset::trail_push (Event *e, int idx)
{
   // iterate through the list of unjustified events and move to the list of
   // justified events those that get justified by event e

   ASSERT (e);
   ASSERT (idx >= 0);

   Elem *el, *nxt;
   for (el = unjust; el; el = nxt)
   {
      nxt = el->next;
      if (e->in_icfl_with (el->e))
      {
         unjust_remove (el);
         just_push (el);
         el->disabler = idx;
         top_disabler = idx;
      }
   }
}

void Disset::trail_pop (int idx)
{
   // if the last event popped out of C (at depth idx) is less or equal to the
   // last event inserted in D (at depth top_idx), then we are backtracking and
   // we need to remove one event from D
   ASSERT (idx >= 0);
   if (idx <= top_idx)
   {
      ASSERT (idx == top_idx);
      unjust_remove (&stack.back());
      stack.pop_back ();
      top_idx = stack.size() ? stack.back().idx : -1;
      ASSERT (idx > top_idx);
   }

   // removing one event from the trail means that some events that were so far
   // justified could now become unjustified; those events will be in the list
   // of unjustified events and will be such that their "disabler" field will be
   // equal to idx; we iterate through them and transfer them from one list to
   // the other
   while (idx <= top_disabler)
   {
      ASSERT (idx == top_disabler);
      ASSERT (! just_isempty ());
      ASSERT (idx == just_peek()->disabler);
      unjust_add (just_pop ());
      top_disabler = just_isempty () ? -1 : just_peek()->disabler;
   }
}

