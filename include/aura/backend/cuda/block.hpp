#ifndef AURA_BACKEND_CUDA_BLOCK_HPP
#define AURA_BACKEND_CUDA_BLOCK_HPP

#include <array>

namespace aura {
namespace backend_detail {
namespace cuda {

typedef svec<std::size_t, AURA_MAX_BLOCK_DIMS> block;

} // cuda 
} // backend_detail
} // aura


#endif // AURA_BACKEND_CUDA_BLOCK_HPP

