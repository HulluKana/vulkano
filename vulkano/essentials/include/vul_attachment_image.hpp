#pragma once

#include"vul_device.hpp"
#include <vulkan/vulkan_core.h>

namespace vulB {

class VulAttachmentImage {
    public:
        VulAttachmentImage(VulDevice &vulDevice);
        ~VulAttachmentImage();

        VulAttachmentImage(const VulAttachmentImage &) = delete;
        VulAttachmentImage &operator=(const VulAttachmentImage &) = delete;
        VulAttachmentImage(VulAttachmentImage &&) = default;

        enum class ImageType {
            colorAttachment,
            depthAttachment
        };
        VkResult createEmptyImage(ImageType type, VkFormat format, VkExtent2D extent);
        VkResult createFromVkImage(VkImage image, VkFormat format, VkExtent2D extent);
        
        void transitionLayout(VkCommandBuffer cmdBuf, VkImageLayout srcLayout, VkImageLayout dstLayout, bool beforeRender) const;
        VkRenderingAttachmentInfo getAttachmentInfo(VkClearValue clearValue) const;

        VkImage getImage() const {return m_image;}
        VkImageView getImageView() const {return m_imageView;}
        VkSampler getSampler() const {return m_sampler;}

        bool preservePreviousContents = false;
    private:
        VulDevice &m_vulDevice;

        VkImage m_image = nullptr;
        VkDeviceMemory m_memory = nullptr; 
        VkImageView m_imageView = nullptr;
        VkSampler m_sampler = nullptr;

        bool m_ownsImage = true;
        ImageType m_type{};
        VkFormat m_format{};
        VkExtent2D m_extent{};
};

}
