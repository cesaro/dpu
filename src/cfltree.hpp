/*
 * cfltree.cc
 */

//-----------------------
template <class T, int SS >
inline Node<T,SS>::Node(int idx, T * pr) :
   depth (pr ? pr->node[idx].depth + 1 : 0),
   pre (pr),
   skiptab (skiptab_alloc (idx))
{
   std::string s = "[";
   for (int i = 0; i < skiptab_size (); i++)
      s += fmt ("%p(%u) ", skiptab[i], skiptab[i]->node[idx].depth);
   s += "]";
   DEBUG("Node<Ev,%d>.ctor: idx %d depth %d pre %p |tab| %u %s",
         SS, idx, depth, pre, skiptab_size (), s.c_str());
}

template <class T, int SS >
inline Node<T,SS>::~Node ()
{
   delete [] skiptab;
}

template <class T, int SS >
inline T **Node<T,SS>::skiptab_alloc (int idx)
{
   unsigned size, i, j;
   T **tab;
   T *p;
   
   // compute the size of the table and allocate memory
   size = skiptab_size ();
   if (size == 0) return nullptr;
   tab = new T* [size];

   // tab[i] stores a pointer to the predecessor at distance SS^{i+1} for
   // 0 <= i < skiptab_size(). So, we initialize skiptab as follows:
   // i     distance    how to
   // ===== =========== ==========================
   // 0     SS^1        scan the tree manually
   // 1     SS^2        skiptab[0]->skiptab[0]
   // 2     SS^3        skiptab[1]->skiptab[1]
   // 3     SS^4        skiptab[2]->skiptab[2]
   // ...

   // manual scan for tab[0]
   p = pre;
   for (i = 1; i < SS; i++) p = p->node[idx].pre;
   tab[0] = p;
   ASSERT (p);

   // initialize the remaining entries
   for (i = 1; i < size; i++)
   {
      p = tab[i-1];
      for (j = 1; j < SS; j++) p = p->node[idx].skiptab[i-1];
      tab[i] = p;
      ASSERT (tab[i]);
   }

   return tab;
}

template <class T, int SS >
inline unsigned Node<T,SS>::skiptab_size () const
{
   // The size of the skiptab table is the number of trailing zeros in the
   // SS-ary representation of this->depth

   unsigned d = depth;
   int count = 0;

   if (d == 0) return 0;
   while (d % SS == 0)
   {
      d /= SS;
      count++;
   }
   return count;
}

template <class T, int SS >
T *Node<T,SS>::best_pred (unsigned target) const
{
   // Every node has a depth.
   // The depth determines the number of skip predecessors.
   // This method returns the closest node to the (only) node at depth "target"
   // that is immediately accessible through {pre,skiptab}.  Similarly to
   // skiptab_size(), it computes the available pointers in skiptab and chooses
   // the best (longest jump) that is still a successor of the target.

   unsigned d, i, jmp;

   ASSERT (target < depth); // so depth >= 1

   // if we only have pre, use it
   if (depth % SS != 0) return pre;
   
   // otherwise skiptab[0] is defined; if using it would take us too far
   // (distance SS), we need to use pre as well
   // (observe that depth - SS could be negative, so we use target + SS)
   if (depth < target + SS) return pre;

   // we now know that skiptab[0] is a valid jump; increment the jump distance
   // (and the index i) until skiptab[i+1] jumps too far, thus computing the
   // longest jump we can do
   ASSERT (depth >= SS);
   jmp = SS * SS;
   d = depth / SS;
   i = 0;
   ASSERT (d >= 1);
   while (1)
   {
      if (d % SS != 0 or depth < target + jmp) return skiptab[i];
      ASSERT (d >= SS);
      i++;
      d /= SS;
      jmp *= SS;
   }
}

template <class T, int SS>
template <int idx>
inline const T *Node<T,SS>::find_pred (unsigned d) const
{
   const T *p;

   ASSERT (d < depth);
   DEBUG ("Node<Ev,%d>.find-pred: target %u depth %u", SS, d, depth);
   for (p = best_pred (d); p->node[idx].depth > d; p = p->node[idx].best_pred (d))
   {
      ASSERT (p->node[idx].best_pred(d)->node[idx].depth < p->node[idx].depth);
      DEBUG ("Node<Ev,%d>.find-pred: at %u", SS, p->node[idx].depth);
   }
   DEBUG ("Node<Ev,%d>.find-pred: at %u", SS, p->node[idx].depth);
   ASSERT (p->node[idx].depth == d);
   return p;
}

template <class T, int SS>
template <int idx>
inline T *Node<T,SS>::find_pred (unsigned d)
{
   return const_cast<T*> (static_cast<const Node<T,SS>*>(this)->find_pred<idx> (d));
}

/*
 * Methods for class MultiNode
 */
template <class T, int SS> // SS: skip step
inline MultiNode<T,SS> :: MultiNode(T *p0, T *p1) :
   node {{0, p0}, {1, p1}}
{
}

template <class T, int SS> // SS: skip step
inline MultiNode<T,SS> :: MultiNode(T *p0) :
   node {{0, p0}, {1, nullptr}}
{
}

