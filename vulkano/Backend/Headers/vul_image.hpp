#pragma once

#include"vul_buffer.hpp"

#include<string>
#include<memory>

namespace vul{

class VulImage
{
    public:
        VulImage(vulB::VulDevice &vulDevice);
        ~VulImage();

        static std::unique_ptr<VulImage> createAsUniquePtr(vulB::VulDevice &vulDevice)
        {
            std::unique_ptr<VulImage> vulImage{new VulImage(vulDevice)};
            return vulImage;
        }

        void createTextureFromFile(std::string fileName, bool modifyLater = false);
        void createTextureFromData(uint8_t* pixels, uint32_t width, uint32_t height, bool modifyLater = false);
        void createRtImage(uint32_t width, uint32_t height);
        void modifyTextureImage(uint8_t* pixels);

        uint32_t getWidth() const {return m_width;}
        uint32_t getHeight() const {return m_height;}
        bool isModifiable() const {return m_modifiable;}

        VkImageView getImageView() const {return m_imageView;}
        VkSampler getTextureSampler() const {return m_textureSampler;}
        VkDescriptorSet getImGuiTextureID() const {return m_descriptorSet;}

        void setDescriptorSet(VkDescriptorSet descriptorSet) {m_descriptorSet = descriptorSet;}

        std::string texturesPath{"../Textures/"};
        bool usableByImGui{false};

        // Do I even need to explain this at this point?
        VulImage(const VulImage &) = delete;
        VulImage &operator=(const VulImage &) = delete;
        // Move instead of reference, you know the deal
        VulImage(VulImage &&) = default;
        VulImage &operator=(VulImage &&) = default;
    private:
        void createTextureImage(uint8_t* pixels);
        void createModifiableTextureImage(uint8_t* pixels);
        void createImage(VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties);
        void transitionImageLayout(VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
        void copyBufferToImage(VkBuffer buffer);

        void createImageView(bool isStorageImage = false);
        void createTextureSampler();

        uint32_t m_width, m_height;
        bool m_modifiable{false}; 
        bool m_hasSampler = false;

        VkImage m_image;
        void *m_mappedMemory;
        VkDeviceMemory m_imageMemory;
        VkImageView m_imageView;
        VkSampler m_textureSampler;
        VkDescriptorSet m_descriptorSet;

        vulB::VulDevice &m_vulDevice;
};

}