
#ifndef __CFLTREE_HH_
#define __CFLTREE_HH_

#include <vector>

// needed in the .hpp
#include "misc.hh"

namespace dpu{

template <class T, int SS>
class Node
{
public:
   /// depth of this node in the tree
   unsigned depth;
   /// immediate predecessor
   T * pre;
   /// skiptab[i] is the predecessors at distance SS^(i+1), for i < skiptab_size()
   T ** skiptab;
   /// immediate causal successors
   std::vector<T*> post;

   /// constructor; idx is the position of this node in the MultiNode class;
   /// _pre is the immediate predecessor; _this is the "host" class
   inline Node (int idx, T *_this, T *_pre);
   inline ~Node ();

   /// returns the unique predecessor of this node at depth d, with d < depth
   template <int idx>
   inline const T *find_pred (unsigned d) const;
   template <int idx>
   inline T *find_pred (unsigned d);

   template <int idx>
   inline bool in_cfl_with (const T* other) const;

   template <int idx>
   void dump () const;

private:
   /// allocates and initializes the skiptab, called only from the ctor
   inline T **skiptab_alloc (int idx);

   /// computes the size of the skiptab table
   inline unsigned skiptab_size () const;

   /// returns a the predecessor of this node that makes for the longest
   /// possible jump that still allows to reach the target depth
   inline T *best_pred (unsigned target) const;
};

//-------template class MultiNode------
template <class T, int SS> // SS: skip step
class MultiNode
{
public:
   Node<T,SS> node[2];

  // inline MultiNode();
   inline MultiNode(T *pred0, T *pred1);
   inline MultiNode(T *pred0);
};

// implemntation of inline methods
#include "cfltree.hpp"

} // end of namespace

#endif
