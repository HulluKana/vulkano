#pragma once

#include"vul_device.hpp"

namespace vulB{

class VulTexSampler{
    public:
        VulTexSampler(VulDevice &vulDevice);
        ~VulTexSampler();

        void createTextureSampler();
    private:
        VkSampler m_textureSampler;

        VulDevice &m_vulDevice;
};

}