#include "vul_debug_tools.hpp"
#include <cmath>
#include <memory>
#include<vul_old_image.hpp>
#include<vul_buffer.hpp>
#include <cstring>
#include <vulkan/vulkan_core.h>

#include <ktx.h>
#include<stb_image.h>

#include<stdexcept>
#include <iostream>

namespace vul{

VulOldImage::VulOldImage(vulB::VulDevice &vulDevice) : m_vulDevice{vulDevice}
{

}

VulOldImage::~VulOldImage()
{
    vkDestroyImageView(m_vulDevice.device(), m_imageView, nullptr);
    vkDestroyImage(m_vulDevice.device(), m_image, nullptr);
    if (m_hasSampler) vkDestroySampler(m_vulDevice.device(), m_textureSampler, nullptr);
    vkFreeMemory(m_vulDevice.device(), m_imageMemory, nullptr);
}

void VulOldImage::loadUncompressedFromFile(const std::string &fileName)
{
    m_data = stbi_load(fileName.c_str(), (int *)&m_width, (int *)&m_height, (int *)&m_channels, STBI_rgb_alpha);
    if (!m_data) throw std::runtime_error("failed to load image!");
    m_dataSize = m_width * m_height * m_channels;
}

void VulOldImage::loadCompressedFromKtxFile(const std::string &fileName, CompressedFromat format)
{
    ktx_transcode_fmt_e ktxFormat{};
    switch (format) {
        case CompressedFromat::bc7Srgb: 
            ktxFormat = KTX_TTF_BC7_RGBA;
            m_format = VK_FORMAT_BC7_SRGB_BLOCK;
            break;
        case CompressedFromat::bc7Unorm: 
            ktxFormat = KTX_TTF_BC7_RGBA;
            m_format = VK_FORMAT_BC7_UNORM_BLOCK;
            break;
    }

    ktxTexture2 *ktxTexture;
    KTX_error_code result = ktxTexture2_CreateFromNamedFile(fileName.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTexture); 
    if (result != KTX_SUCCESS) throw std::runtime_error("Failed to create ktxTexture: " + std::to_string(result));

    result = ktxTexture2_TranscodeBasis(ktxTexture, ktxFormat, 0);
    if (result != KTX_SUCCESS)
        throw std::runtime_error("Failed to transcode ktxTexture to format " + std::to_string(ktxFormat) + ": " + std::to_string(result));

    m_data = malloc(ktxTexture->dataSize);
    memcpy(m_data, ktxTexture->pData, ktxTexture->dataSize);

    keepEmpty(ktxTexture->baseWidth, ktxTexture->baseHeight, 4, 1);
    m_mipLevels = ktxTexture->numLevels;
    free(ktxTexture);
}

void VulOldImage::loadData(const void *data, uint32_t width, uint32_t height, uint32_t channels, uint32_t bytesPerPixel)
{
    m_constData = data;
    keepEmpty(width, height, channels, bytesPerPixel);
}

void VulOldImage::keepEmpty(uint32_t width, uint32_t height, uint32_t channels, uint32_t bytesPerPixel)
{
    if (channels != 1 && channels != 2 && channels != 4) throw std::runtime_error("The amount of channels in an image must be 1, 2 or 4, but it is " + std::to_string(channels));
    m_width = width;
    m_height = height;
    m_channels = channels;
    m_bytesPerPixel = bytesPerPixel;
}

void VulOldImage::createImageSingleTime(bool createSampler, bool isDeviceLocal, ImageType type, int dimensions)
{
    VkCommandBuffer cmdBuf = m_vulDevice.beginSingleTimeCommands();
    createImage(createSampler, isDeviceLocal, type, dimensions, cmdBuf);
    m_vulDevice.endSingleTimeCommands(cmdBuf);
    deleteCpuResources();
}

void VulOldImage::createImage(bool createSampler, bool isDeviceLocal, ImageType type, int dimensions, VkCommandBuffer cmdBuf)
{
    if (dimensions < 1 || dimensions > 2) throw std::runtime_error("Vul image must have between 1 and 2 dimensions");
    if (createSampler && type != ImageType::texture) throw std::runtime_error("Cannot create a sampler for a storage image");
    if (m_format != VK_FORMAT_UNDEFINED) throw std::runtime_error("Get fucked lol");
    m_isLocal = isDeviceLocal;
    m_type = type;

    m_dataSize = m_width * m_height * m_channels;
    if (m_format == VK_FORMAT_UNDEFINED) {
        if (type != ImageType::texture) m_dataSize *= sizeof(float);

        if (type == ImageType::storageFloat){
            if (m_channels == 1) m_format = VK_FORMAT_R32_SFLOAT;
            else if (m_channels == 2) m_format = VK_FORMAT_R32G32_SFLOAT;
            else if (m_channels == 4) m_format = VK_FORMAT_R32G32B32A32_SFLOAT;
        } else if (type == ImageType::storageUint) {
            if (m_channels == 1) m_format = VK_FORMAT_R32_UINT;
            else if (m_channels == 2) m_format = VK_FORMAT_R32G32_UINT;
            else if (m_channels == 4) m_format = VK_FORMAT_R32G32B32A32_UINT;
        } else{
            if (m_channels == 1) m_format = VK_FORMAT_R8_SRGB;
            else if (m_channels == 2) m_format = VK_FORMAT_R8G8_SRGB;
            else if (m_channels == 4) m_format = VK_FORMAT_R8G8B8A8_SRGB;
        }
    }

    VkImageUsageFlags usage = 0;
    if (type == ImageType::texture) usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
    else usage |= VK_IMAGE_USAGE_STORAGE_BIT;
    if (isDeviceLocal) usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    VkMemoryPropertyFlags memoryProperties;
    if (isDeviceLocal) memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    else memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    VkImageTiling tiling = VK_IMAGE_TILING_LINEAR;
    if (isDeviceLocal && type == ImageType::texture) tiling = VK_IMAGE_TILING_OPTIMAL; 

    m_layout = VK_IMAGE_LAYOUT_GENERAL;
    if (type == ImageType::texture) m_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkImageType imageType = VK_IMAGE_TYPE_2D;
    if (dimensions == 1) imageType = VK_IMAGE_TYPE_1D;

    createImageLowLevel(createSampler, m_format, imageType, m_layout, usage, memoryProperties, tiling, cmdBuf);
}

void VulOldImage::createImageLowLevel(bool createSampler, VkFormat format, VkImageType type, VkImageLayout layout, VkImageUsageFlags usage, VkMemoryPropertyFlags memoryProperties, VkImageTiling tiling, VkCommandBuffer cmdBuf)
{
    m_format = format;
    m_layout = layout;
    bool isDeviceLocal = memoryProperties & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    VkImageViewType imageViewType{};
    if (type == VK_IMAGE_TYPE_1D) imageViewType = VK_IMAGE_VIEW_TYPE_1D;
    else if (type == VK_IMAGE_TYPE_2D) imageViewType = VK_IMAGE_VIEW_TYPE_2D;

    const void *data = nullptr;
    if (m_constData != nullptr) data = m_constData;
    else if (m_data != nullptr) data = m_data;

    createVkImage(m_format, tiling, usage, memoryProperties, type);
    if (data != nullptr && isDeviceLocal){
        transitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, cmdBuf);
        uint32_t usedSize = 0;
        for (uint32_t i = 0; i < m_mipLevels; i++) {
            uint32_t width = m_width / static_cast<uint32_t>(pow(2, m_mipLevels - i - 1));
            uint32_t height = m_height / static_cast<uint32_t>(pow(2, m_mipLevels - i - 1));
            bool skipThisOne = width == 0 || height == 0;
            width = (std::max(width, 1u) + 3) & ~3;
            height = (std::max(height, 1u) + 3) & ~3;
            uint32_t size = width * height * m_bytesPerPixel;
            usedSize += size;
            m_stagingBuffers.push_back(std::make_unique<vulB::VulBuffer>(m_vulDevice));
            if (skipThisOne) continue;

            m_stagingBuffers[i]->loadData(reinterpret_cast<void *>(reinterpret_cast<size_t>(data) + usedSize - size), 1, size);
            m_stagingBuffers[i]->createBuffer(false, vulB::VulBuffer::usage_transferSrc);
            copyBufferToImage(m_stagingBuffers[i]->getBuffer(), m_mipLevels - i - 1, cmdBuf);
        }
        transitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_layout, cmdBuf);
    }
    else if (data != nullptr && !isDeviceLocal){
        transitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, cmdBuf);

        vkMapMemory(m_vulDevice.device(), m_imageMemory, 0, m_dataSize, 0, &m_mappedMemory);
        memcpy(m_mappedMemory, data, m_dataSize);

        if (m_layout != VK_IMAGE_LAYOUT_GENERAL) transitionImageLayout(VK_IMAGE_LAYOUT_GENERAL, m_layout, cmdBuf);
    }
    else transitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, m_layout, cmdBuf);
    
    createImageView(imageViewType);
    if (createSampler) createTextureSampler();
}

void VulOldImage::addSampler(VkSampler sampler)
{
    if (m_hasSampler) throw std::runtime_error("Image already has a sampler");
    if (m_type != ImageType::texture) throw std::runtime_error("Cannot add a sampler to storage image");
    m_textureSampler = sampler;
    m_hasSampler = true;
}

void VulOldImage::modifyImage(void *data)
{
    VkDeviceSize imageSize = m_width * m_height * m_channels;
    if (m_type != ImageType::texture) imageSize *= sizeof(float);

    VkCommandBuffer cmdBuf = m_vulDevice.beginSingleTimeCommands();
    if (m_isLocal){
        vulB::VulBuffer stagingBuffer(m_vulDevice);
        stagingBuffer.loadData(data, 1, imageSize);
        stagingBuffer.createBuffer(false, vulB::VulBuffer::usage_transferSrc);

        transitionImageLayout(m_layout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, cmdBuf);
        copyBufferToImage(stagingBuffer.getBuffer(), 0, cmdBuf);
        transitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_layout, cmdBuf);
    }
    else {
        if (m_type != ImageType::texture) transitionImageLayout(m_layout, VK_IMAGE_LAYOUT_GENERAL, cmdBuf);
        memcpy(m_mappedMemory, data, (size_t)imageSize);
        if (m_type != ImageType::texture) transitionImageLayout(VK_IMAGE_LAYOUT_GENERAL, m_layout, cmdBuf);
    }
    m_vulDevice.endSingleTimeCommands(cmdBuf);
}

void VulOldImage::deleteCpuResources()
{
    if (m_data != nullptr) {
        free(m_data);
        m_data = nullptr;
    }
    m_constData = nullptr;

    m_stagingBuffers.clear();
}

void VulOldImage::createImageView(VkImageViewType imageViewType)
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_image;
    viewInfo.viewType = imageViewType;
    viewInfo.format = m_format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = m_mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(m_vulDevice.device(), &viewInfo, nullptr, &m_imageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture image view in vulkano_image.cpp");
    }

    VUL_NAME_VK(m_imageView)
}

void VulOldImage::createTextureSampler()
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
    samplerInfo.maxLod = static_cast<float>(m_mipLevels);

    if (vkCreateSampler(m_vulDevice.device(), &samplerInfo, nullptr, &m_textureSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler in vul_texture_sampler.cpp");
    }
    m_hasSampler = true;

    VUL_NAME_VK(m_textureSampler)
}

void VulOldImage::createVkImage(VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImageType imageType)
{
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = imageType;
    imageInfo.extent.width = m_width;
    imageInfo.extent.height = m_height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = m_mipLevels;
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

    VUL_NAME_VK(m_image)
    VUL_NAME_VK(m_imageMemory)
}

void VulOldImage::transitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout, VkCommandBuffer cmdBuf)
{
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = m_image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = m_mipLevels;
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

    vkCmdPipelineBarrier(cmdBuf, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

void VulOldImage::copyBufferToImage(VkBuffer buffer, uint32_t mipLevel, VkCommandBuffer cmdBuf) 
{
    uint32_t width = m_width / static_cast<uint32_t>(pow(2, mipLevel));
    uint32_t height = m_height / static_cast<uint32_t>(pow(2, mipLevel));

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = mipLevel;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};

    vkCmdCopyBufferToImage(cmdBuf, buffer, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}


}
