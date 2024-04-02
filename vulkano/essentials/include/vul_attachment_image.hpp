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
        
        void establishPreAttachmentPipelineBarrier(VkCommandBuffer cmdBuf);
        void establishPostAttachmentPipelineBarrier(VkCommandBuffer cmdBuf);
        VkRenderingAttachmentInfo getAttachmentInfo(VkClearValue clearValue);

        VkImage getImage() const {return m_image;}
        VkImageView getImageView() const {return m_imageView;}
    private:
        VulDevice &m_vulDevice;

        VkImage m_image = nullptr;
        VkImageView m_imageView = nullptr;
        VkDeviceMemory m_memory = nullptr; 

        bool m_ownsImage = true;
        ImageType m_type{};
        VkFormat m_format{};
        VkExtent2D m_extent{};
};

}
