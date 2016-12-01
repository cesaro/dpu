/*
 * cfltree.cc
 */

/*
 * Methods for class Node
 */
//-----------------------
template <class T, int SS >
inline Node<T,SS>::Node()
{
   depth = 0;
   pre   = nullptr;
   skip_preds = nullptr;
}

//-----------------------
template <class T, int SS >
inline Node<T,SS>::Node(int idx, Event * pr)
{
   //DEBUG("   -  Set up nodes");
   pre      = pr;
   depth    = pre->nodep[idx].depth + 1;
   // initialize skip_preds here
   set_skip_preds(idx);
}
//-----------------------
template <class T, int SS >
inline void Node<T,SS>:: set_skip_preds(int idx)
{
   DEBUG_("     + SET NODE[%d]", idx);
   // including immediate predecessor
   int size = this->compute_size();
   DEBUG("Size of skip_preds = %d", size);

   // initialize the elements
   if (size == 0)
   {
      DEBUG("   No skip_pred");
      return;
   }

   ASSERT(size > 0);

   /* mallocate the skip_preds */
   skip_preds = (Event**) malloc(sizeof(Event*) * size);

   // the first element skip_preds[0]
   T * p = pre;
   int k = 1;

   /* go back k times by pre*/
   while ((k < SS) and (p->node[idx].depth > 0))
   {
      p = p->node[idx].pre;
      k++;
   }

   skip_preds[0] = p;

   /* initialize the rest */
   if (size > 1)
   {
      for (unsigned i = 1; i < size; i++)
      {
         p = skip_preds[i - 1];
         int k = 1;

         /* go back k times by pre*/
         while ((k < SS) and (p->node[idx].depth > 0))
         {
            p = p->node[idx].skip_preds[i -1];
            k++;
         }
         skip_preds[i] = p;
      }
   }
   print_skip_preds();
}
//-----------------------
template <class T, int SS >
inline void Node<T,SS>:: set_up(int idx, Event * pr)
{
   pre      = pr;
   depth    = pre->node[idx].depth + 1;
   // initialize skip_preds here
   //DEBUG("Set skip preds");
   set_skip_preds(idx);
}
//---------------------
template <class T, int SS >
inline int Node<T,SS>::compute_size()
{
   int d = this->depth;
   int skip = SS;
   //DEBUG("cmpsize.depth = %d", d);
   if ((d == 0) or (d < skip)) return 0;

   while (d % skip != 0)
      d--;

   int temp = 1;
   while (d % skip == 0)
   {
      skip = skip * SS;
      temp++;
   }
   return --temp;
}
//-------------------
template <class T, int SS >
inline void Node<T,SS>:: print_skip_preds()
{
   DEBUG_("Node: %p", this);
   DEBUG_(", depth: %d", this->depth);
   DEBUG_(", Pre: %p", this->pre);
   int size = compute_size();
   if (size == 0)
   {
      DEBUG_(", No skip predecessor\n");
      return;
   }
   DEBUG_(", Skip_preds: ");
   for (unsigned i = 0; i < size; i++)
   {
      if (i == size-1)
         DEBUG_ ("%p", skip_preds[i]);
      else
         DEBUG_ ("%p, ", skip_preds[i]);
   }

   DEBUG_("\n");
}
//----------

inline int max_skip(int d, int base)
{
   int i = 0;
   int pow = 1;
   while (d % pow == 0)
   {
      pow = pow * base;
      i++;
   }
   return i-1;
}

//----------
/*
 * This is function to find a node at a specific depth.
 * !make sure that d < this->depth to use this method
 */
template <class T, int SS >
template <int idx>
inline T & Node<T,SS>:: find_pred(int d)
{
   T * next = nullptr;
   ASSET(this->depth > d);
   int i, dis = this->depth - d;
   ASSERT(dis != 0); // at the beginning dis != 0

   // initial next for the very first time
   //DEBUG("dis = %d", dis);
   // find maximal number of steps to skip

   i = max_skip(dis,SS);

   if (i == 0)
      next = pre;
   else
      next = skip_preds[i-1];
   dis = next->node[idx].depth - d;
   //DEBUG("Now dis = %d", dis);

   // for the second loop and so on
   while (dis != 0)
   {
      i = max_skip(dis,SS);
      DEBUG("i = %d", i);
      if (i == 0)
         next = next->node[idx].pre;
      else
         next = next->node[idx].skip_preds[i-1];

      dis = next->node[idx].depth - d;
   }
   //DEBUG("next = %d", next->idx);
   return *next;
}
#if 0
//-----------
template <class T, int SS >
//template <int idx>
inline bool Node<T,SS>::is_pred_of(Node &n) const
{
   if (n.depth < this->depth) return false;

   ASSERT(n.depth > this->depth);
   Node ne;
   ne = n.find_pred(this->depth);

   if (&ne == this) return true;

   return false;
}
#endif
/*
 * Methods for class MultiNode
 */
template <class T, int S, int SS> // S: number of trees, SS: skip step
inline MultiNode<T,S,SS> ::MultiNode()
{
}

template <class T, int S, int SS> // S: number of trees, SS: skip step
inline MultiNode<T,S,SS> :: MultiNode(T * pp, T * pm)
{
   DEBUG("Set up node[0]");
   node[0].set_up(0,pp);
   DEBUG("Set up node[1]");
   node[1].set_up(1,pm);
}




