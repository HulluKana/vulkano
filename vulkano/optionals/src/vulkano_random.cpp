#include<vulkano_random.hpp>
#include <limits>

namespace vul{

uint32_t random::uint32(uint32_t min, uint32_t max)
{
    int64_t preventOverflowingInt = (int64_t)dist(rng);
    return (uint32_t)(preventOverflowingInt + std::numeric_limits<int>::min()) % (max - min + 1) + min;
}

float random::floatNormalized()
{
    uint32_t randomUint = uint32(0, std::numeric_limits<uint32_t>::max() - 1);
    return static_cast<double>(randomUint) / static_cast<double>(std::numeric_limits<uint32_t>::max() - 1);
}

}
