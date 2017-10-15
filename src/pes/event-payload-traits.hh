
#ifndef __PES_EVENTPAYLOADTRAITS_HH_
#define __PES_EVENTPAYLOADTRAITS_HH_

namespace dpu
{

template<typename T = void>
struct EventPayload
{
   size_t pointed_memory_sizej () const;
   std::string str() const;
};

/// Template specialization for the default (void) type
template<>
struct EventPayload<void>
{
   size_t pointed_memory_size () const { return 0; }
   std::string str() const { return "-"; }
};

} // namespace dpu
#endif
