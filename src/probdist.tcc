
#include <algorithm>
#include <cmath>

template <class T>
Probdist<T>::Probdist () :
   counters (_counters),
   samples (0),
   _counters ()
{
}

template <class T>
void Probdist<T>::clear ()
{
   samples = 0;
   _counters.clear ();
}

template <class T>
void Probdist<T>::sample (T e)
{
   auto it = _counters.find (e);
   if (it == _counters.end())
      _counters[e] = 1;
   else
      it->second += 1;
   samples++;
}

template <class T>
size_t Probdist<T>::size () const
{
   return samples;
}

template <class T>
unsigned Probdist<T>::count (T e) const
{
   auto it = _counters.find (e);
   if (it == _counters.end()) return 0;
   return it->second;
}

template <class T>
float Probdist<T>::mass (T e) const
{
   if (samples == 0) return NAN;
   return count2mass (count(e));
}

template <class T>
T Probdist<T>::max () const
{
   bool first;
   T max;

   if (samples == 0)
      throw std::out_of_range ("Empty sample");

   first = true;
   for (auto &kv : _counters)
   {
      if (first)
      {
         first = false;
         max = kv.first;
      }
      if (kv.first > max) max = kv.first;
   }
   return max;
}

template <class T>
T Probdist<T>::min () const
{
   bool first;
   T min;

   if (samples == 0)
      throw std::out_of_range ("Empty sample");

   first = true;
   for (auto &kv : _counters)
   {
      if (first)
      {
         first = false;
         min = kv.first;
      }
      if (kv.first < min) min = kv.first;
   }
   return min;
}


template <class T>
float Probdist<T>::average () const
{
   T avg;

   avg = 0;
   for (auto &kv : _counters)
   {
      avg += kv.first * kv.second;
   }
   return avg / (float) samples;
}

template <class T>
std::string Probdist<T>::summary_snc () const
{
   std::ostringstream ss;
   ss << size() << "/" << _counters.size();
   return ss.str();
}

template <class T>
std::string Probdist<T>::summary_mma () const
{
   std::ostringstream ss;
   ss.precision (2);
   if (size())
      ss << min() << "/" << max() << "/" << std::fixed << average();
   else
      ss << "nan/nan/nan";
   return ss.str();
}

template <class T>
std::string Probdist<T>::summary_mmac () const
{
   std::ostringstream ss;
   ss.precision (2);
   if (size()) {
      ss << _counters.size() << "/";
      ss << min() << "/" << max() << "/" << std::fixed << average();
   } else {
      ss << "0/nan/nan/nan";
   }
   return ss.str();
}

template <class T>
std::string Probdist<T>::summary_freq_maxc (unsigned limit) const
{
   std::ostringstream ss;
   std::vector<std::pair<T,unsigned>> selected;
   unsigned i, rest;

   // copy all pairs from the counters to the vector
   for (auto &kv : _counters) selected.push_back (kv);

   if (limit == 0 or limit > selected.size())
      limit = selected.size();

   // sort them by decreasing frequency (count)
   struct {
      bool operator() (const std::pair<T,unsigned> &a, const std::pair<T,unsigned> &b)
         { return a.second > b.second; }
   } cmp;
   std::partial_sort (selected.begin(), selected.begin() + limit, selected.end(), cmp);

   // how many samples are left out?
   rest = 0;
   for (i = limit; i < selected.size(); i++) rest += selected[i].second;

   // truncate the vector to "limit" pairs
   selected.resize (limit);

   ss.precision (1);
   ss << std::fixed;
   summary_freq (ss, selected);
   if (rest)
      ss << "; (rest)=" << rest << "/" << count2mass (rest) * 100 << "%";
   return ss.str();
}

template <class T>
void Probdist<T>::summary_freq (std::ostringstream &ss,
      const std::vector<std::pair<T,unsigned>> &vec) const
{
   bool notfirst = false;
   for (auto &kv : vec)
   {
      if (notfirst) ss << "; ";
      notfirst = true;
      summary_freq (ss, kv.first);
   }
}

template <class T>
void Probdist<T>::summary_freq (std::ostringstream &ss, const T &e) const
{
   ss << e << "=" << count(e) << "/" << mass(e) * 100 << "%";
}

template <class T>
float Probdist<T>::count2mass (unsigned c) const
{
   return c / (float) samples;
}
