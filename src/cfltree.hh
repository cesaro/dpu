
#ifndef __CFLTREE_HH_
#define __CFLTREE_HH_

namespace dpu{

//-------template class Node ---------
template <class T, int SS>
class Node
{
public:
   unsigned depth;
   T * pre; // immediate predecessor
   T ** skip_preds;

   //inline Node();
   inline Node(int idx, T * pr);
   ~Node ();

   //inline void set_up(int idx, Event * pr);


   template <int idx>
   inline T & find_pred(int d) const;

//   template <int idx>
   inline bool is_pred(Node &n) const;

private:

   inline void set_skip_preds(int idx);
   inline T **__ctor_skip_preds (int idx);
   inline int __ctor_compute_size_skip_preds();
   //inline void print_skip_preds();

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

#include "cfltree.hpp"

} // end of namespace

#endif

