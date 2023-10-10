#pragma once

#include"vul_buffer.hpp"

#include<string>

namespace vul{

class VulImage
{
    public:
        VulImage(vulB::VulDevice &vulDevice);
        void destroy();

        void createTextureImage(std::string fileName);

        uint32_t getWidth() const {return m_width;}
        uint32_t getHeight() const {return m_height;}

        VkImageView getImageView() const {return m_imageView;}

        std::string texturesPath = "Textures/";

        // Do I even need to explain this at this point?
        VulImage(const VulImage &) = delete;
        VulImage &operator=(const VulImage &) = delete;
        // Move instead of reference, you know the deal
        VulImage(VulImage &&) = default;
        VulImage &operator=(VulImage &&) = default;
    private:
        void createImage(VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties);
        void transitionImageLayout(VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
        void copyBufferToImage(VkBuffer buffer);

        void createTextureImageView();

        uint32_t m_width, m_height;

        VkImage m_image;
        VkDeviceMemory m_imageMemory;
        VkImageView m_imageView;

        vulB::VulDevice &m_vulDevice;
};

}