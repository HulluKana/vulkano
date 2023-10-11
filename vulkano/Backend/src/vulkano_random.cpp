#include"../../vulkano_random.hpp"

namespace vul{

uint32_t random::uint32(uint32_t min, uint32_t max)
{
    int64_t preventOverflowingInt = (int64_t)dist(rng);
    return (uint32_t)(preventOverflowingInt + std::numeric_limits<int>::min()) % (max - min + 1) + min;
}

}