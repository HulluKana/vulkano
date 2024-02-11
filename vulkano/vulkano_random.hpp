#pragma once

#include<random>

namespace vul{

class random{
    public:
        static uint32_t uint32(uint32_t min, uint32_t max);
        static float floatNormalized();
    private:
        static inline std::random_device dev{};
        static inline std::mt19937 rng{dev()};
        static inline std::uniform_int_distribution<> dist{std::numeric_limits<int>::min(), std::numeric_limits<int>::max()};
};

}
