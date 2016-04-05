
#ifndef __DPU_FE_LLVM_UTIL_TPP_
#define __DPU_FE_LLVM_UTIL_TPP_

namespace dpu {
namespace fe {
namespace llvm {

template <class T> bool itemexists( std::vector<T> vec, T item ) {
    return std::find( vec.begin(), vec.end(), item ) != vec.end();
}

template <class T> void appendVector( std::vector<T> v1, std::vector<T> v2 ) {
    v1.insert( v1.end(), v2.begin(), v2.end() );
}

} } } // namespace dpu::fe::llvm

#endif
