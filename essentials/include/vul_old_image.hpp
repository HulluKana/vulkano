#pragma once

#include "vul_buffer.hpp"
#include"vul_device.hpp"
#include<string>
#include <vulkan/vulkan_core.h>

namespace vul{

class VulOldImage
{
    public:
        VulOldImage(vulB::VulDevice &vulDevice);
        ~VulOldImage();

        void loadUncompressedFromFile(const std::string &fileName);
        enum class CompressedFromat {
            bc7Srgb,
            bc7Unorm
        };
        void loadCompressedFromKtxFile(const std::string &fileName, CompressedFromat format);
        void loadData(const void *data, uint32_t width, uint32_t height, uint32_t channels, uint32_t bytesPerPixel);
        void keepEmpty(uint32_t width, uint32_t height, uint32_t channels, uint32_t bytesPerPixel);

        enum class ImageType {
            texture,
            storageFloat,
            storageUint
        };
        void createImageSingleTime(bool createSampler, bool isDeviceLocal, ImageType type, int dimensions);
        void createImage(bool createSampler, bool isDeviceLocal, ImageType type, int dimensions, VkCommandBuffer cmdBuf);
        void createImageLowLevel(bool createSampler, VkFormat format, VkImageType type, VkImageLayout layout, VkImageUsageFlags usage, VkMemoryPropertyFlags memoryProperties, VkImageTiling tiling, VkCommandBuffer cmdBuf);
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

        VulOldImage(const VulOldImage &) = delete;
        VulOldImage &operator=(const VulOldImage &) = delete;
        VulOldImage(VulOldImage &&) = default;
    private:
        void createImageView(VkImageViewType imageViewType);
        void createTextureSampler();
        void createVkImage(VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImageType imageType);
        void transitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout, VkCommandBuffer cmdBuf);
        void copyBufferToImage(VkBuffer buffer, uint32_t mipLevel, VkCommandBuffer cmdBuf);

        VkFormat m_format = VK_FORMAT_UNDEFINED;
        uint32_t m_width = 0;
        uint32_t m_height = 0;
        uint32_t m_channels = 0;
        uint32_t m_bytesPerPixel = 0;
        uint32_t m_mipLevels = 1;
        ImageType m_type = ImageType::texture;
        bool m_isLocal = false;
        bool m_hasSampler = false;
        VkDeviceMemory m_imageMemory;
        VkImageView m_imageView;
        VkSampler m_textureSampler;
        VkDescriptorSet m_descriptorSet;

        void *m_data = nullptr;
        const void *m_constData = nullptr;
        size_t m_dataSize = 0;
        std::vector<std::unique_ptr<vulB::VulBuffer>> m_stagingBuffers;
        VkImageLayout m_layout;
        VkImage m_image;
        void *m_mappedMemory;
        vulB::VulDevice &m_vulDevice;
};

}
