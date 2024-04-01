#pragma once
 
#include <functional>
 
namespace vulB {
 
// from: https://stackoverflow.com/a/57595105
template <typename T, typename... Rest>
void hashCombine(std::size_t& seed, const T& v, const Rest&... rest) {
    seed ^= std::hash<T>{}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    (hashCombine(seed, rest), ...);
};

constexpr u_int32_t alignUp(u_int32_t alignable, u_int32_t alignSize)
{
    return (alignable + (alignSize - 1)) & ~(alignSize - 1);
}
 
}