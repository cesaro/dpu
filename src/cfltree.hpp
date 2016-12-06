/*
 * cfltree.cc
 */

///*
// * Methods for class Node
// */
////-----------------------
//template <class T, int SS >
//inline Node<T,SS>::Node()
//{
//   depth = 0;
//   pre   = nullptr;
//   skip_preds = nullptr;
//}

//-----------------------
template <class T, int SS >
inline Node<T,SS>::Node(int idx, T * pr) :
   depth (pr ? pr->node[idx].depth + 1 : 0),
   pre (pr),
   skip_preds (__ctor_skip_preds (idx))
{
   DEBUG("Node<?,%d>.ctor: idx %d depth %d pre %p skip-preds %p",
         SS, idx, depth, pre, skip_preds);
}

template <class T, int SS >
Node<T,SS>::~Node ()
{
   delete [] skip_preds;
}

//-----------------------
template <class T, int SS >
inline T **Node<T,SS>::__ctor_skip_preds (int idx)
{
//   DEBUG_("     + SET NODE[%d]", idx);
   // including immediate predecessor
   int size = __ctor_compute_size_skip_preds();
   DEBUG(" + Size of skip_preds = %d", size);

   // initialize the elements
   if (size == 0)
   {
      DEBUG("   No skip_pred");
      return nullptr;
   }

   ASSERT(size > 0);

   /* allocate the skip_preds */
   T ** skip_preds = new T* [size];

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

   //print_skip_preds();
   return skip_preds;
}

//---------------------
template <class T, int SS >
inline int Node<T,SS>::__ctor_compute_size_skip_preds()
{
   unsigned d = depth;
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

////-------------------
//template <class T, int SS >
//inline void Node<T,SS>:: print_skip_preds()
//{
//   DEBUG_(" + Node: %p", this);
//   DEBUG_(", depth: %d", this->depth);
//   DEBUG_(", Pre: %p", this->pre);
//   int size = compute_size();
//   if (size == 0)
//   {
//      DEBUG_(", No skip predecessor\n");
//      return;
//   }
//   DEBUG_(", Skip_preds: ");
//   for (unsigned i = 0; i < size; i++)
//   {
//      if (i == size-1)
//         DEBUG_ ("%p", skip_preds[i]);
//      else
//         DEBUG_ ("%p, ", skip_preds[i]);
//   }
//
//   DEBUG_("\n");
//}
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
inline T & Node<T,SS>:: find_pred(int d) const
{
   T * next = nullptr;
   ASSERT(depth > d);
   int i, dis = depth - d;
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
//      DEBUG("i = %d", i);
      if (i == 0)
         next = next->node[idx].pre;
      else
         next = next->node[idx].skip_preds[i-1];

      dis = next->node[idx].depth - d;
   }
   //DEBUG("next = %d", next->idx);
   return *next;
}

//-----------
template <class T, int SS >
template <int idx>
inline bool Node<T,SS>::is_pred(Node &n) const
{
   if (n.depth < depth) return false;

   ASSERT(n.depth > depth);
   T * e = n.find_pred<idx>(depth);

//   if (e.node[idx] == this) return true;

   return false;
}

/*
 * Methods for class MultiNode
 */
template <class T, int SS> // SS: skip step
inline MultiNode<T,SS> :: MultiNode(T *p0, T *p1) :
   node {{0, p0}, {1, p1}}
{
   DEBUG("Set up node[0,1]");
}

template <class T, int SS> // SS: skip step
inline MultiNode<T,SS> :: MultiNode(T *p0) :
   node {{0, p0}, {1, nullptr}}
{
   DEBUG("Set up node[0,1]");
}

