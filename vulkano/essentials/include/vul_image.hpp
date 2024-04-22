#pragma once

#include "vul_buffer.hpp"
#include"vul_device.hpp"
#include<string>
#include <vulkan/vulkan_core.h>

namespace vul{

class VulImage
{
    public:
        VulImage(vulB::VulDevice &vulDevice);
        ~VulImage();

        void loadFile(const std::string &fileName);
        void loadKtxFile(const std::string &fileName, bool unorm);
        void loadData(const void *data, uint32_t width, uint32_t height, uint32_t channels);
        void keepEmpty(uint32_t width, uint32_t height, uint32_t channels);

        enum class ImageType {
            texture,
            storageFloat,
            storageUint
        };
        void createImageSingleTime(bool createSampler, bool isDeviceLocal, ImageType type, int dimensions);
        void createImage(bool createSampler, bool isDeviceLocal, ImageType type, int dimensions, VkCommandBuffer cmdBuf);
        void addSampler(VkSampler sampler);
        void modifyImage(void *data);

        void deleteCpuResources();
        void setDescriptorSet(VkDescriptorSet descriptorSet) {m_descriptorSet = descriptorSet;}
    
        bool isLocal() const {return m_isLocal;}
        ImageType getType() const {return m_type;}
        uint32_t getWidth() const {return m_width;}
        uint32_t getHeight() const {return m_height;}
        uint32_t getChannels() const {return m_channels;}
        VkImage getImage() const {return m_image;}
        VkImageView getImageView() const {return m_imageView;}
        VkSampler getTextureSampler() const {return m_textureSampler;}
        VkFormat getFormat() const {return m_format;}
        VkDescriptorSet getImGuiTextureID() const {return m_descriptorSet;}

        bool usableByImGui{false};
        std::string name;

        // Do I even need to explain this at this point?
        VulImage(const VulImage &) = delete;
        VulImage &operator=(const VulImage &) = delete;
        // Move instead of reference, you know the deal
        VulImage(VulImage &&) = default;
        VulImage &operator=(VulImage &&) = default;
    private:
        void createImageView(VkImageViewType imageViewType);
        void createTextureSampler();
        void createVkImage(VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImageType imageType);
        void transitionImageLayout(VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, VkCommandBuffer cmdBuf);
        void copyBufferToImage(VkBuffer buffer, VkCommandBuffer cmdBuf);

        VkFormat m_format = VK_FORMAT_UNDEFINED;
        uint32_t m_width = 0;
        uint32_t m_height = 0;
        uint32_t m_channels = 0;
        ImageType m_type = ImageType::texture;
        bool m_isLocal = false;
        bool m_hasSampler = false;
        VkDeviceMemory m_imageMemory;
        VkImageView m_imageView;
        VkSampler m_textureSampler;
        VkDescriptorSet m_descriptorSet;

        void *m_data = nullptr;
        const void *m_constData = nullptr;
        std::unique_ptr<vulB::VulBuffer> m_stagingBuffer = nullptr;
        VkImageLayout m_layout;
        VkImage m_image;
        void *m_mappedMemory;
        vulB::VulDevice &m_vulDevice;
};

}
