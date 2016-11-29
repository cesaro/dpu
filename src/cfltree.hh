
#ifndef __CFLTREE_HH_
#define __CFLTREE_HH_

namespace dpu{
class Event;
//-------template class Node ---------
template <class T, int SS>
class Node
{
public:
   unsigned depth;
   T * pre; // immediate predecessor
   T ** skip_preds;

   Node();
   Node(int idx, Event * pr);
   void set_up(int idx, Event * pr);
   int compute_size();
   void set_skip_preds(int idx);
   void print_skip_preds();

   template <int idx>
   T & find_pred(int d) const;

   template <int idx>
   bool is_pred(T &n) const;

};

//-------template class MultiNode------
template <class T, int S, int SS> // S: number of trees, SS: skip step
class MultiNode
{
public:
   Node<T,SS> node[S];

   MultiNode();
   MultiNode(T * pp, T * pm);
};

#endif
} // end of namespace
