#pragma once

#include"vul_device.hpp"

namespace vulB{

class VulTexSampler{
    public:
        VulTexSampler(VulDevice &vulDevice);
        ~VulTexSampler();

        void createTextureSampler();

        VkSampler getTextureSampler() const {return m_textureSampler;}
    private:
        VkSampler m_textureSampler;

        VulDevice &m_vulDevice;
};

}