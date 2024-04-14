#include "vul_device.hpp"
#include<vul_attachment_image.hpp>
#include <vulkan/vulkan_core.h>

namespace vulB{

VulAttachmentImage::VulAttachmentImage(VulDevice &vulDevice) : m_vulDevice{vulDevice}
{

}

VulAttachmentImage::~VulAttachmentImage()
{
    if (m_ownsImage && m_image != nullptr) vkDestroyImage(m_vulDevice.device(), m_image, nullptr);
    if (m_memory != nullptr) vkFreeMemory(m_vulDevice.device(), m_memory, nullptr);
    if (m_imageView != nullptr) vkDestroyImageView(m_vulDevice.device(), m_imageView, nullptr);
    if (m_sampler != nullptr) vkDestroySampler(m_vulDevice.device(), m_sampler, nullptr);
}

VkResult VulAttachmentImage::createEmptyImage(ImageType type, VkFormat format, VkExtent2D extent)
{
    m_type = type;
    m_format = format;
    m_extent = extent;
    m_ownsImage = true;

    VkImageUsageFlags imageUsage{};
    VkImageAspectFlags aspectFlags{};
    if (m_type == ImageType::colorAttachment) {
        imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
    } else {
        imageUsage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
    }

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = m_extent.width;
    imageInfo.extent.height = m_extent.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = m_format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = imageUsage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.flags = 0;
    VkResult result = vkCreateImage(m_vulDevice.device(), &imageInfo, nullptr, &m_image);
    if (result != VK_SUCCESS) return result;

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_vulDevice.device(), m_image, &memRequirements);
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = m_vulDevice.findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    result = vkAllocateMemory(m_vulDevice.device(), &allocInfo, nullptr, &m_memory);
    if (result != VK_SUCCESS) return result;
    result = vkBindImageMemory(m_vulDevice.device(), m_image, m_memory, 0);
    if (result != VK_SUCCESS) return result;

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = m_format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    result = vkCreateImageView(m_vulDevice.device(), &viewInfo, nullptr, &m_imageView);
    if (result != VK_SUCCESS) return result;

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = m_vulDevice.properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    return vkCreateSampler(m_vulDevice.device(), &samplerInfo, nullptr, &m_sampler);
}

VkResult VulAttachmentImage::createFromVkImage(VkImage image, VkFormat format, VkExtent2D extent)
{
    m_type = ImageType::colorAttachment;
    m_image = image;
    m_format = format;
    m_extent = extent;
    m_ownsImage = false;

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = m_format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    return vkCreateImageView(m_vulDevice.device(), &viewInfo, nullptr, &m_imageView);
}

void VulAttachmentImage::transitionLayout(VkCommandBuffer cmdBuf, VkImageLayout srcLayout, VkImageLayout dstLayout, bool beforeRender) const
{
    VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;
    VkAccessFlags access = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    if (preservePreviousContents) access = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
    if (m_type == ImageType::depthAttachment) {
        aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
        if (preservePreviousContents) access = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
        else access = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }

    VkImageSubresourceRange subResourceRange{};
    subResourceRange.aspectMask = aspect;
    subResourceRange.baseMipLevel = 0;
    subResourceRange.levelCount = 1;
    subResourceRange.baseArrayLayer = 0;
    subResourceRange.layerCount = 1;

    VkImageMemoryBarrier imageMemoryBarrier{};
    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    if (beforeRender) imageMemoryBarrier.dstAccessMask = access;
    else imageMemoryBarrier.srcAccessMask = access;
    imageMemoryBarrier.oldLayout = srcLayout;
    imageMemoryBarrier.newLayout = dstLayout;
    imageMemoryBarrier.image = m_image;
    imageMemoryBarrier.subresourceRange = subResourceRange;

    VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    if (m_type == ImageType::depthAttachment) srcStage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    if (beforeRender) {
        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        if (m_type == ImageType::depthAttachment) dstStage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    }
    vkCmdPipelineBarrier(cmdBuf, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
}

VkRenderingAttachmentInfo VulAttachmentImage::getAttachmentInfo(VkClearValue clearValue) const
{
    VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    if (preservePreviousContents) loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    if (storeCurrentContents) storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    if (m_type == ImageType::colorAttachment) {
        VkRenderingAttachmentInfo colorAttachmentInfo{};
        colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachmentInfo.imageView = m_imageView;
        colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
        colorAttachmentInfo.loadOp = loadOp;
        colorAttachmentInfo.storeOp = storeOp;
        colorAttachmentInfo.clearValue = clearValue;
        return colorAttachmentInfo;
    }
    else {
        VkRenderingAttachmentInfo depthAttachmentInfo{};
        depthAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        depthAttachmentInfo.imageView = m_imageView;
        depthAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
        depthAttachmentInfo.loadOp = loadOp;
        depthAttachmentInfo.storeOp = storeOp;
        depthAttachmentInfo.clearValue = clearValue;
        return depthAttachmentInfo;
    }
}

}
