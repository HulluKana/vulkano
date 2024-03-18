#include"../Headers/vul_image.hpp"
#include"../Headers/vul_buffer.hpp"
#include <cstring>
#include <vulkan/vulkan_core.h>

#define STB_IMAGE_IMPLEMENTATION
#include"../../../3rdParty/stb_image.h"

#include<stdexcept>

namespace vul{

VulImage::VulImage(vulB::VulDevice &vulDevice) : m_vulDevice{vulDevice}
{

}

VulImage::~VulImage()
{
    vkDestroyImageView(m_vulDevice.device(), m_imageView, nullptr);
    vkDestroyImage(m_vulDevice.device(), m_image, nullptr);
    if (m_hasSampler) vkDestroySampler(m_vulDevice.device(), m_textureSampler, nullptr);
    vkFreeMemory(m_vulDevice.device(), m_imageMemory, nullptr);
}

void VulImage::loadFile(const std::string &fileName)
{
    m_data = stbi_load(fileName.c_str(), (int *)&m_width, (int *)&m_height, (int *)&m_channels, STBI_rgb_alpha);
    if (!m_data) throw std::runtime_error("failed to load image!");
}

void VulImage::loadData(const void *data, uint32_t width, uint32_t height, uint32_t channels)
{
    m_constData = data;
    keepEmpty(width, height, channels);
}

void VulImage::keepEmpty(uint32_t width, uint32_t height, uint32_t channels)
{
    if (channels != 1 && channels != 2 && channels != 4) throw std::runtime_error("The amount of channels in an image must be 1, 2 or 4");
    m_width = width;
    m_height = height;
    m_channels = channels;
}

void VulImage::createImage(bool createSampler, bool isDeviceLocal, bool isStorageImage, int dimensions)
{
    if (dimensions < 1 || dimensions > 3) throw std::runtime_error("Vul image must have between 1 and 3 dimensions");
    if (createSampler && isStorageImage) throw std::runtime_error("Cannot create a sampler for a storage image");
    m_isLocal = isDeviceLocal;
    m_isStorage = isStorageImage;

    const void *data = nullptr;
    if (m_constData != nullptr) data = m_constData;
    else if (m_data != nullptr) data = m_data;

    VkDeviceSize imageSize = m_width * m_height * m_channels;
    if (isStorageImage) imageSize *= sizeof(float);

    if (isStorageImage){
        if (m_channels == 1) m_format = VK_FORMAT_R32_SFLOAT;
        else if (m_channels == 2) m_format = VK_FORMAT_R32G32_SFLOAT;
        else if (m_channels == 4) m_format = VK_FORMAT_R32G32B32A32_SFLOAT;
    } else{
        if (m_channels == 1) m_format = VK_FORMAT_R8_SRGB;
        else if (m_channels == 2) m_format = VK_FORMAT_R8G8_SRGB;
        else if (m_channels == 4) m_format = VK_FORMAT_R8G8B8A8_SRGB;
    }

    VkImageUsageFlags usage = 0;
    if (isStorageImage) usage |= VK_IMAGE_USAGE_STORAGE_BIT;
    else usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
    if (isDeviceLocal) usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    VkMemoryPropertyFlags memoryProperties;
    if (isDeviceLocal) memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    else memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    VkImageTiling tiling = VK_IMAGE_TILING_LINEAR;
    if (isDeviceLocal && !isStorageImage) tiling = VK_IMAGE_TILING_OPTIMAL; 

    m_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    if (isStorageImage) m_layout = VK_IMAGE_LAYOUT_GENERAL;

    VkImageType imageType = VK_IMAGE_TYPE_2D;
    VkImageViewType imageViewType = VK_IMAGE_VIEW_TYPE_2D;
    if (dimensions == 1){
        imageType = VK_IMAGE_TYPE_1D;
        imageViewType = VK_IMAGE_VIEW_TYPE_1D;
    }
    if (dimensions == 3){
        imageType = VK_IMAGE_TYPE_3D;
        imageViewType = VK_IMAGE_VIEW_TYPE_3D;
    }

    createVkImage(m_format, tiling, usage, memoryProperties, imageType);
    if (data != nullptr && isDeviceLocal){
        vulB::VulBuffer stagingBuffer(m_vulDevice, imageSize, 1, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT); 
        stagingBuffer.map(imageSize);
        memcpy(stagingBuffer.getMappedMemory(), data, static_cast<size_t>(imageSize));
        stagingBuffer.unmap();

        transitionImageLayout(m_format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        copyBufferToImage(stagingBuffer.getBuffer());
        transitionImageLayout(m_format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_layout);
    }
    else if (data != nullptr && !isDeviceLocal){
        transitionImageLayout(m_format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

        vkMapMemory(m_vulDevice.device(), m_imageMemory, 0, imageSize, 0, &m_mappedMemory);
        memcpy(m_mappedMemory, data, (size_t)imageSize);

        if (!isStorageImage) transitionImageLayout(m_format, VK_IMAGE_LAYOUT_GENERAL, m_layout);
    }
    else transitionImageLayout(m_format, VK_IMAGE_LAYOUT_UNDEFINED, m_layout);
    
    createImageView(imageViewType);
    if (createSampler) createTextureSampler();

    if (m_data != nullptr) stbi_image_free(m_data);
}

void VulImage::addSampler(VkSampler sampler)
{
    if (m_hasSampler) throw std::runtime_error("Image already has a sampler");
    if (isStorageImage()) throw std::runtime_error("Cannot add a sampler to storage image");
    m_textureSampler = sampler;
    m_hasSampler = true;
}

void VulImage::modifyImage(void *data)
{
    VkDeviceSize imageSize = m_width * m_height * m_channels;
    if (m_isStorage) imageSize *= sizeof(float);

    if (m_isLocal){
        vulB::VulBuffer stagingBuffer(m_vulDevice, imageSize, 1, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT); 
        stagingBuffer.map(imageSize);
        memcpy(stagingBuffer.getMappedMemory(), data, static_cast<size_t>(imageSize));
        stagingBuffer.unmap();

        transitionImageLayout(m_format, m_layout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        copyBufferToImage(stagingBuffer.getBuffer());
        transitionImageLayout(m_format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_layout);
    }
    else {
        if (!m_isStorage) transitionImageLayout(m_format, m_layout, VK_IMAGE_LAYOUT_GENERAL);
        memcpy(m_mappedMemory, data, (size_t)imageSize);
        if (!m_isStorage) transitionImageLayout(m_format, VK_IMAGE_LAYOUT_GENERAL, m_layout);
    }
}

void VulImage::createImageView(VkImageViewType imageViewType)
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_image;
    viewInfo.viewType = imageViewType;
    viewInfo.format = m_format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(m_vulDevice.device(), &viewInfo, nullptr, &m_imageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture image view in vulkano_image.cpp");
    }
}

void VulImage::createTextureSampler()
{
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

    if (vkCreateSampler(m_vulDevice.device(), &samplerInfo, nullptr, &m_textureSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler in vul_texture_sampler.cpp");
    }
    m_hasSampler = true;
}

void VulImage::createVkImage(VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImageType imageType)
{
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = imageType;
    imageInfo.extent.width = m_width;
    imageInfo.extent.height = m_height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.flags = 0; // Optional

    if (vkCreateImage(m_vulDevice.device(), &imageInfo, nullptr, &m_image) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image in vulkano_image.cpp");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_vulDevice.device(), m_image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = m_vulDevice.findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(m_vulDevice.device(), &allocInfo, nullptr, &m_imageMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory in vulkano_image.cpp!");
    }

    vkBindImageMemory(m_vulDevice.device(), m_image, m_imageMemory, 0);
}

void VulImage::transitionImageLayout(VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{
    VkCommandBuffer commandBuffer = m_vulDevice.beginSingleTimeCommands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = m_image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED){
        barrier.srcAccessMask = 0;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL){
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_GENERAL){
        barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_HOST_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else throw std::invalid_argument("unsupported old layout transition in vulkano_image.cpp file!");

    if (newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (newLayout == VK_IMAGE_LAYOUT_GENERAL){
        barrier.dstAccessMask = VK_ACCESS_HOST_WRITE_BIT;
        destinationStage = VK_PIPELINE_STAGE_HOST_BIT; 
    } else throw std::invalid_argument("unsupported new layout transition in vulkano_image.cpp file!");

    vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    m_vulDevice.endSingleTimeCommands(commandBuffer);
}

void VulImage::copyBufferToImage(VkBuffer buffer) 
{
    VkCommandBuffer commandBuffer = m_vulDevice.beginSingleTimeCommands();

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = {0, 0, 0};
    region.imageExtent = {m_width, m_height, 1};

    vkCmdCopyBufferToImage(commandBuffer, buffer, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    m_vulDevice.endSingleTimeCommands(commandBuffer);
}


}
