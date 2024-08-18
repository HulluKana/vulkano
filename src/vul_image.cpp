#include <OpenEXR/ImfRgba.h>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>
#include <vul_image.hpp>
#include <vul_device.hpp>
#include <vul_buffer.hpp>
#include <vul_debug_tools.hpp>

#include <OpenEXR/ImfRgbaFile.h>
#include <ktx.h>
#include <vulkan/vulkan_core.h>
#include <stb_image.h>

#include <iostream>


namespace vul {

VulSampler::VulSampler(const VulDevice &vulDevice, VkFilter filter, VkSamplerAddressMode addressMode, float maxAnisotropy,
                VkBorderColor borderColor, VkSamplerMipmapMode mipMapMode, bool enableSamplerReduction,
                VkSamplerReductionMode samplerReductionMode, float mipLodBias, float mipMinLod, float mipMaxLod)
    : m_vulDevice{vulDevice}
{
    VkSamplerReductionModeCreateInfo reductionModeInfo{};
    reductionModeInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO;
    reductionModeInfo.reductionMode = samplerReductionMode;

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    if (enableSamplerReduction) samplerInfo.pNext = &reductionModeInfo;
    samplerInfo.magFilter = filter;
    samplerInfo.minFilter = filter;
    samplerInfo.addressModeU = addressMode;
    samplerInfo.addressModeV = addressMode;
    samplerInfo.addressModeW = addressMode;
    if (maxAnisotropy >= 1.0f) samplerInfo.anisotropyEnable = VK_TRUE;
    else samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = maxAnisotropy;
    samplerInfo.borderColor = borderColor;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = mipMapMode;
    samplerInfo.mipLodBias = mipLodBias;
    samplerInfo.minLod = mipMinLod;
    samplerInfo.maxLod = mipMaxLod;

    if (vkCreateSampler(m_vulDevice.device(), &samplerInfo, nullptr, &m_sampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler in vul_texture_sampler.cpp");
    }

    VUL_NAME_VK(m_sampler)
}

VulSampler::~VulSampler()
{
    if (m_sampler != VK_NULL_HANDLE) vkDestroySampler(m_vulDevice.device(), m_sampler, nullptr);
}

std::shared_ptr<VulSampler> VulSampler::createDefaultTexSampler(const vul::VulDevice &vulDevice, uint32_t mipLevels)
{
    std::shared_ptr<VulSampler> sampler;
    sampler.reset(new VulSampler{vulDevice, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT,
            vulDevice.properties.limits.maxSamplerAnisotropy, VK_BORDER_COLOR_INT_OPAQUE_BLACK,
            VK_SAMPLER_MIPMAP_MODE_LINEAR, false, VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE,
            0.0f, 0.0f, static_cast<float>(mipLevels)});
    return sampler;
}

std::shared_ptr<VulSampler> VulSampler::createCustomSampler(const VulDevice &vulDevice, VkFilter filter,
                VkSamplerAddressMode addressMode, float maxAnisotropy, VkBorderColor borderColor, VkSamplerMipmapMode mipMapMode,
                bool enableSamplerReduction, VkSamplerReductionMode samplerReductionMode,
                float mipLodBias, float mipMinLod, float mipMaxLod)
{
    std::shared_ptr<VulSampler> sampler;
    sampler.reset(new VulSampler{vulDevice, filter, addressMode, maxAnisotropy, borderColor,
            mipMapMode, enableSamplerReduction, samplerReductionMode, mipLodBias, mipMinLod, mipMaxLod});
    return sampler;
}





VulImage::VulImage(const VulDevice &vulDevice) : m_vulDevice{vulDevice}
{

}

VulImage::~VulImage()
{
    for (VkImageView imageView : m_mipImageViews) vkDestroyImageView(m_vulDevice.device(), imageView, nullptr);
    if (m_imageView != VK_NULL_HANDLE) vkDestroyImageView(m_vulDevice.device(), m_imageView, nullptr);
    if (m_image != VK_NULL_HANDLE && m_ownsImage) vkDestroyImage(m_vulDevice.device(), m_image, nullptr);
    if (m_imageMemory != VK_NULL_HANDLE) vkFreeMemory(m_vulDevice.device(), m_imageMemory, nullptr);
}

void VulImage::OldVkImageStuff::destoyImageStuff()
{
    for (VkImageView mipImageView : mipImageViews) vkDestroyImageView(device, mipImageView, nullptr);
    if (imageView != VK_NULL_HANDLE) vkDestroyImageView(device, imageView, nullptr);
    if (image != VK_NULL_HANDLE) vkDestroyImage(device, image, nullptr);
    if (imageMemory != VK_NULL_HANDLE) vkFreeMemory(device, imageMemory, nullptr);
}

void VulImage::loadCompressedKtxFromFile(const std::string &fileName, KtxCompressionFormat compressionFormat,
        uint32_t inputMipLevel, uint32_t mipLevelCount)
{
    if (fileName.length() == 0 || mipLevelCount == 0) return;

    KtxCompressionFormatProperties ktxFormatProperties = getKtxCompressionFormatProperties(compressionFormat);
    VkFormatProperties formatProperties = getVkFormatProperties(ktxFormatProperties.vkFormat);

    ktxTexture2 *origTexture;
    KTX_error_code result = ktxTexture2_CreateFromNamedFile(fileName.c_str(), KTX_TEXTURE_CREATE_NO_FLAGS, &origTexture); 
    if (result != KTX_SUCCESS) throw std::runtime_error("Failed to create ktxTexture. File: " + fileName + " Error code: " + std::to_string(result));

    ktxTextureCreateInfo createInfo{};
    createInfo.baseWidth = origTexture->baseWidth / std::pow(2, inputMipLevel);
    createInfo.baseHeight = origTexture->baseHeight / std::pow(2, inputMipLevel);
    createInfo.baseDepth = origTexture->baseDepth;
    createInfo.numLevels = origTexture->numLevels - inputMipLevel;
    createInfo.numFaces = origTexture->numFaces;
    createInfo.numLayers = origTexture->numLayers;
    createInfo.numDimensions = origTexture->numDimensions;
    createInfo.generateMipmaps = origTexture->generateMipmaps;
    createInfo.pDfd = origTexture->pDfd;
    ktxTexture2 *texture;
    ktxTexture2_Create(&createInfo, KTX_TEXTURE_CREATE_ALLOC_STORAGE, &texture);
    origTexture->dataSize = texture->dataSize;
    texture->vtbl->LoadImageData(ktxTexture(origTexture), texture->pData, texture->dataSize);
    mipLevelCount = std::min(texture->numLevels, mipLevelCount);

    result = ktxTexture2_TranscodeBasis(texture, ktxFormatProperties.transcodeFormat, 0);
    if (result != KTX_SUCCESS) throw std::runtime_error("Failed to transcode ktxTexture to format " +
            std::to_string(ktxFormatProperties.transcodeFormat) + " File: " + fileName + " Error code: " + std::to_string(result));

    const uint32_t baseWidth = alignUp(texture->baseWidth, formatProperties.sideLengthAlignment);
    const uint32_t baseHeight = alignUp(texture->baseHeight, formatProperties.sideLengthAlignment);
    const uint32_t baseDepth = texture->baseDepth;

    std::vector<std::vector<void *>> data(texture->numLevels);
    uint8_t *pCopySrc = texture->pData;
    for (int i = texture->numLevels - 1; i >= 0; i--) {
        const uint32_t width = std::max(baseWidth / static_cast<uint32_t>(std::pow(2, i)), 1u);
        const uint32_t height = std::max(baseHeight / static_cast<uint32_t>(std::pow(2, i)), 1u);
        const uint32_t depth = std::max(baseDepth / static_cast<uint32_t>(std::pow(2, i)), 1u);
        uint32_t layerSize = alignUp(width, formatProperties.sideLengthAlignment) *
            alignUp(height, formatProperties.sideLengthAlignment) * depth * formatProperties.bitsPerTexel / 8;

        data[i].resize(texture->numLayers);
        for (uint32_t j = 0; j < texture->numLayers; j++) {
            data[i][j] = pCopySrc;
            pCopySrc += layerSize;
        } 
    }
    data.resize(mipLevelCount);

    loadRawFromMemory(baseWidth, baseHeight, baseDepth, data, ktxFormatProperties.vkFormat); 
    ktxTexture_Destroy(ktxTexture(origTexture));
    ktxTexture_Destroy(ktxTexture(texture));
}

void VulImage::loadCubemapFromEXR(const std::string &filename)
{
    Imf::RgbaInputFile file(filename.c_str());
    Imath::Box2i dataWindow = file.dataWindow();
    uint32_t width = dataWindow.max.x - dataWindow.min.x + 1;
    uint32_t height = dataWindow.max.y - dataWindow.min.y + 1;
    assert(height % 6 == 0);
    height /= 6;

    std::unique_ptr<Imf::Rgba[]> pixels = std::unique_ptr<Imf::Rgba[]>(new Imf::Rgba[width * height * 6]());
    file.setFrameBuffer(pixels.get(), 1, width);
    file.readPixels(dataWindow.min.y, dataWindow.max.y);

    std::vector<void *> mipLevel(6);
    mipLevel[0] = pixels.get() + height * width * 0;
    mipLevel[1] = pixels.get() + height * width * 1;
    mipLevel[2] = pixels.get() + height * width * 2;
    mipLevel[3] = pixels.get() + height * width * 3;
    mipLevel[4] = pixels.get() + height * width * 5;
    mipLevel[5] = pixels.get() + height * width * 4;
    loadRawFromMemory(width, height, 1, {mipLevel}, VK_FORMAT_R16G16B16A16_SFLOAT);
}

void VulImage::loadUncompressedFromFile(const std::string &filename)
{
    assert(0 && "Not implemented");
}

void VulImage::loadRawFromMemory(uint32_t baseWidth, uint32_t baseHeight, uint32_t baseDepth,
        const std::vector<std::vector<void *>> &data, VkFormat format)
{
    assert(data.size() > 0 && data[0].size() > 0);

    keepEmpty(baseWidth, baseHeight, baseDepth, data.size(), data[0].size(), format);

    size_t offset = 0;
    for (size_t i = 0; i < data.size(); i++) {
        for (size_t j = 0; j < data[i].size(); j++) {
            m_mipLevels[i].containsData[j] = false;
            if (data[i][j] != nullptr) {
                memcpy(&m_data[offset], data[i][j], m_mipLevels[i].layerSize);
                m_mipLevels[i].containsData[j] = true;
            }
            m_mipLevels[i].layers[j] = offset;
            offset += m_mipLevels[i].layerSize;
        }
    }
}

void VulImage::keepEmpty(uint32_t baseWidth, uint32_t baseHeight, uint32_t baseDepth, uint32_t mipCount,
        uint32_t arrayCount, VkFormat format)
{
    assert(baseWidth > 0);
    assert(baseHeight > 0);
    assert(baseDepth > 0);
    assert(format == m_format || m_format == VK_FORMAT_UNDEFINED);

    VkFormatProperties formatProperties = getVkFormatProperties(format);
    m_baseWidth = baseWidth;
    m_baseHeight = baseHeight;
    m_baseDepth = baseDepth;
    m_format = format;
    m_bitsPerTexel = formatProperties.bitsPerTexel;

    size_t requiredSize = 0; 
    m_mipLevels.resize(mipCount);
    for (uint32_t i = 0; i < mipCount; i++) {
        const uint32_t width = std::max(baseWidth / static_cast<uint32_t>(std::pow(2, i)), 1u);
        const uint32_t height = std::max(baseHeight / static_cast<uint32_t>(std::pow(2, i)), 1u);
        const uint32_t depth = std::max(baseDepth / static_cast<uint32_t>(std::pow(2, i)), 1u);
        m_mipLevels[i].width = width;
        m_mipLevels[i].height = height;
        m_mipLevels[i].depth = depth;
        m_mipLevels[i].layerSize = alignUp(width, formatProperties.sideLengthAlignment)
            * alignUp(height, formatProperties.sideLengthAlignment) * depth * formatProperties.bitsPerTexel / 8;
        requiredSize += m_mipLevels[i].layerSize * arrayCount;
    }

    m_data.resize(requiredSize);
    for (uint32_t i = 0; i < mipCount; i++) {
        m_mipLevels[i].layers.resize(arrayCount);
        m_mipLevels[i].containsData.resize(arrayCount);
    }
}

void VulImage::addMipLevelsToStartFromCompressedKtxFile(const std::string &fileName,
        KtxCompressionFormat compressionFormat, uint32_t mipLevelCount)
{
    const std::vector<uint8_t> oldData = m_data;
    std::vector<MipLevel> oldMipLevels = m_mipLevels;
    m_mipLevels.clear();
    m_data.clear();
    loadCompressedKtxFromFile(fileName, compressionFormat, 0, mipLevelCount);
    for (MipLevel &mipLevel : oldMipLevels) {
        for (size_t &layer : mipLevel.layers) layer += m_data.size();
    }
    m_data.insert(m_data.end(), oldData.begin(), oldData.end());
    m_mipLevels.insert(m_mipLevels.end(), oldMipLevels.begin(), oldMipLevels.end());
}

std::unique_ptr<VulImage::OldVkImageStuff> VulImage::createCustomImage(VkImageViewType type, VkImageLayout layout, VkImageUsageFlags usage,
        VkMemoryPropertyFlags memoryProperties, VkImageTiling tiling, VkImageAspectFlags aspect, VkCommandBuffer cmdBuf)
{
    assert(m_mipLevels.size() > 0);
    if (type == VK_IMAGE_VIEW_TYPE_1D || type == VK_IMAGE_VIEW_TYPE_1D_ARRAY) m_imageType = VK_IMAGE_TYPE_1D;
    else if (type == VK_IMAGE_VIEW_TYPE_2D || type == VK_IMAGE_VIEW_TYPE_2D_ARRAY || type == VK_IMAGE_VIEW_TYPE_CUBE
            || type == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY) m_imageType = VK_IMAGE_TYPE_2D;
    else if (type == VK_IMAGE_VIEW_TYPE_3D) m_imageType = VK_IMAGE_TYPE_3D;
    m_ownsImage = true;
    m_imageViewType = type;
    m_usage = usage;
    m_memoryProperties = memoryProperties;
    m_tiling = tiling;
    m_aspect = aspect;
    m_arrayLayersCount = m_mipLevels[0].layers.size();
    for (uint32_t i = 1; i < m_mipLevels.size(); i++) assert(m_arrayLayersCount == m_mipLevels[i].layers.size());
    assert(m_arrayLayersCount >= 1);
    assert(!(m_arrayLayersCount > 1 && type == VK_IMAGE_VIEW_TYPE_3D));
    uint32_t maxSize = std::max(m_mipLevels[0].width, m_mipLevels[0].height);
    maxSize = std::max(maxSize, m_mipLevels[0].depth);
    m_mipLevels.resize(std::min(static_cast<uint32_t>(m_mipLevels.size()), static_cast<uint32_t>(std::log2(maxSize))));

    std::unique_ptr<OldVkImageStuff> oldVkImageStuff = std::make_unique<OldVkImageStuff>();
    oldVkImageStuff->image = m_image;
    oldVkImageStuff->imageMemory = m_imageMemory;
    oldVkImageStuff->imageView = m_imageView;
    oldVkImageStuff->mipImageViews = m_mipImageViews;
    oldVkImageStuff->device = m_vulDevice.device();
    m_stagingBuffers.clear();

    createVkImage();
    m_imageView = createImageView(0, m_mipLevels.size());

    const bool isDeviceLocal = memoryProperties & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    bool containsData = false;
    for (size_t i = 0; i < m_mipLevels.size(); i++) for (size_t j = 0; j < m_mipLevels[i].containsData.size(); j++)
        containsData = containsData || m_mipLevels[i].containsData[j];
    if (containsData && isDeviceLocal) {
        transitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, cmdBuf);
        for (size_t i = 0; i < m_mipLevels.size(); i++) {
            m_stagingBuffers.push_back(std::make_unique<vul::VulBuffer>(1, m_mipLevels[i].layerSize * m_arrayLayersCount, false, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, m_vulDevice));
            m_stagingBuffers[m_stagingBuffers.size() - 1]->writeData(&m_data[m_mipLevels[i].layers[0]], m_stagingBuffers[m_stagingBuffers.size() - 1]->getBufferSize(), 0, VK_NULL_HANDLE);
            copyBufferToImage(m_stagingBuffers[m_stagingBuffers.size() - 1]->getBuffer(), i, cmdBuf);

            VUL_NAME_VK(m_stagingBuffers[m_stagingBuffers.size() - 1]->getBuffer())
        }
        if (layout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) transitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, layout, cmdBuf);
    }
    else transitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, layout, cmdBuf);

    return oldVkImageStuff;
}

void VulImage::createFromVkImage(VkImage image, VkImageViewType type, VkFormat format, VkImageAspectFlags aspect,
        uint32_t mipLevelCount, uint32_t arrayLayerCount)
{
    m_ownsImage = false;
    m_image = image;
    m_imageViewType = type;
    m_format = format;
    m_arrayLayersCount = arrayLayerCount;
    m_mipLevels.resize(1);
    m_aspect = aspect;
    m_imageView = createImageView(0, m_mipLevels.size());
}

void VulImage::createImageViewsForMipMaps()
{
    m_mipImageViews.resize(m_mipLevels.size());
    for (uint32_t i = 0; i < m_mipImageViews.size(); i++) {
        m_mipImageViews[i] = createImageView(i, 1);
    }
}

VkRenderingAttachmentInfo VulImage::getAttachmentInfo(VkClearValue clearValue) const
{
    VUL_PROFILE_FUNC()
    VkAttachmentLoadOp loadOp = attachmentPreservePreviousContents ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_CLEAR;
    VkAttachmentStoreOp storeOp = attachmentStoreCurrentContents ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
    VkRenderingAttachmentInfo attachmentInfo{};
    attachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    attachmentInfo.imageView = m_imageView;
    attachmentInfo.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
    attachmentInfo.loadOp = loadOp;
    attachmentInfo.storeOp = storeOp;
    attachmentInfo.clearValue = clearValue;
    return attachmentInfo;
}

VkDescriptorImageInfo VulImage::getDescriptorInfo() const
{
    assert(m_imageView != VK_NULL_HANDLE);
    VkDescriptorImageInfo descInfo;
    descInfo.imageView = m_imageView;
    descInfo.imageLayout = m_layout;
    if (vulSampler != nullptr) descInfo.sampler = vulSampler->getSampler();
    else descInfo.sampler = VK_NULL_HANDLE;
    return descInfo;
}

VkDescriptorImageInfo VulImage::getMipDescriptorInfo(uint32_t mipLevel) const
{
    assert(m_mipImageViews.size() > mipLevel);
    VkDescriptorImageInfo descInfo;
    descInfo.imageView = m_mipImageViews[mipLevel];
    descInfo.imageLayout = m_layout;
    if (vulSampler != nullptr) descInfo.sampler = vulSampler->getSampler();
    else descInfo.sampler = VK_NULL_HANDLE;
    return descInfo;
}

void VulImage::transitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout, VkCommandBuffer cmdBuf)
{
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = m_image;
    barrier.subresourceRange.aspectMask = m_aspect;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = m_mipLevels.size();
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = m_arrayLayersCount;

    VkPipelineStageFlags sourceStage{};
    VkPipelineStageFlags destinationStage{};

    switch (oldLayout) {
        case VK_IMAGE_LAYOUT_UNDEFINED:
            barrier.srcAccessMask = 0;
            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            break;
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            break;
        case VK_IMAGE_LAYOUT_GENERAL:
            barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
            sourceStage = VK_PIPELINE_STAGE_HOST_BIT;
            break;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
            sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            break;
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            barrier.srcAccessMask = attachmentPreservePreviousContents ? VK_ACCESS_COLOR_ATTACHMENT_READ_BIT :
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            break;
        case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
            barrier.srcAccessMask = attachmentPreservePreviousContents ? VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT :
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            sourceStage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            break;
        default:
            std::invalid_argument("unsupported old layout transition in VulImage: " + std::to_string(oldLayout));
            break;
    }

    switch (newLayout) {
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            break;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            break;
        case VK_IMAGE_LAYOUT_GENERAL:
            barrier.dstAccessMask = VK_ACCESS_HOST_WRITE_BIT;
            destinationStage = VK_PIPELINE_STAGE_HOST_BIT; 
            break;
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            barrier.dstAccessMask = attachmentPreservePreviousContents ? VK_ACCESS_COLOR_ATTACHMENT_READ_BIT :
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            break;
        case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
            barrier.dstAccessMask = attachmentPreservePreviousContents ? VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT :
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            destinationStage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            break;
        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
            barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
            destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            break;
        default:
            throw std::invalid_argument("unsupported new layout transition in VulImage: " + std::to_string(newLayout));
            break;
    }

    vkCmdPipelineBarrier(cmdBuf, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    m_layout = newLayout;
}

void VulImage::transitionQueueFamily(uint32_t srcFamilyIdx, uint32_t dstFamilyIdx, VkPipelineStageFlags accessMask, VkCommandBuffer cmdBuf)
{
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = m_layout;
    barrier.newLayout = m_layout;
    barrier.srcQueueFamilyIndex = srcFamilyIdx;
    barrier.dstQueueFamilyIndex = dstFamilyIdx;
    barrier.image = m_image;
    barrier.subresourceRange.aspectMask = m_aspect;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = m_mipLevels.size();
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = m_arrayLayersCount;
    vkCmdPipelineBarrier(cmdBuf, accessMask, accessMask, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

std::unique_ptr<VulImage> VulImage::createDefaultWholeImageAllInOne(const vul::VulDevice &vulDevice, std::variant<std::string,
        RawImageData> data, std::variant<KtxCompressionFormat, VkFormat> format, bool addSampler,
        InputDataType dataType, ImageType imageType, VkCommandBuffer cmdBuf)
{
    std::unique_ptr<VulImage> img = std::make_unique<VulImage>(vulDevice);
    switch (dataType) {
        case InputDataType::compressedKtxFile:
            img->loadCompressedKtxFromFileWhole(std::get<std::string>(data), std::get<KtxCompressionFormat>(format));
            break;
        case InputDataType::exrFile:
            img->loadCubemapFromEXR(std::get<std::string>(data));
            break;
        case InputDataType::normieFile:
            img->loadUncompressedFromFile(std::get<std::string>(data));
            break;
        case InputDataType::rawData:
            RawImageData rawData = std::get<RawImageData>(data);
            img->loadRawFromMemory(rawData.baseWidth, rawData.baseHeight, rawData.baseDepth,
                    rawData.data, std::get<VkFormat>(format));
            break;
    }
    img->createDefaultImage(imageType, cmdBuf);
    if (addSampler) img->vulSampler = VulSampler::createDefaultTexSampler(vulDevice, img->getMipCount());
    return img;
}

void VulImage::loadCompressedKtxFromFileWhole(const std::string &fileName, KtxCompressionFormat compressionFormat)
{
    loadCompressedKtxFromFile(fileName, compressionFormat, 0, 69);
}

void VulImage::keepRegularRaw2d8bitRgbaEmpty(uint32_t width, uint32_t height)
{
    keepEmpty(width, height, 1, 1, 1, VK_FORMAT_R8G8B8A8_SRGB);
}

void VulImage::keepRegularRaw2d32bitRgbaEmpty(uint32_t width, uint32_t height)
{
    keepEmpty(width, height, 1, 1, 1, VK_FORMAT_R32G32B32A32_SFLOAT);
}

std::unique_ptr<VulImage::OldVkImageStuff> VulImage::createDefaultImage(ImageType type, VkCommandBuffer cmdBuf)
{
    assert(m_mipLevels.size() > 0);
    size_t arrayLayers = m_mipLevels[0].layers.size();
    switch (type) {
        case ImageType::texture1d:
            if (arrayLayers > 1) return createCustomImage(VK_IMAGE_VIEW_TYPE_1D_ARRAY, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT |
                    VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, cmdBuf);
            else return createCustomImage(VK_IMAGE_VIEW_TYPE_1D, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT
                    | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, cmdBuf);
            break;
        case ImageType::texture2d:
            if (arrayLayers > 1) return createCustomImage(VK_IMAGE_VIEW_TYPE_2D_ARRAY, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT
                    | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, cmdBuf);
            else return createCustomImage(VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT
                    | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, cmdBuf);
            break;
        case ImageType::texture3d:
            return createCustomImage(VK_IMAGE_VIEW_TYPE_3D, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT
                    | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, cmdBuf);
            break;
        case ImageType::textureCube:
            if (arrayLayers > 6) return createCustomImage(VK_IMAGE_VIEW_TYPE_CUBE_ARRAY, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT
                    | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, cmdBuf);
            else return createCustomImage(VK_IMAGE_VIEW_TYPE_CUBE, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT
                    | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, cmdBuf);
            break;
        case ImageType::storage1d:
            if (arrayLayers > 1) return createCustomImage(VK_IMAGE_VIEW_TYPE_1D_ARRAY, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_USAGE_STORAGE_BIT
                    | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_TILING_LINEAR, VK_IMAGE_ASPECT_COLOR_BIT, cmdBuf);
            else return createCustomImage(VK_IMAGE_VIEW_TYPE_1D, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_USAGE_STORAGE_BIT
                    | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_TILING_LINEAR, VK_IMAGE_ASPECT_COLOR_BIT, cmdBuf);
            break;
        case ImageType::storage2d:
            if (arrayLayers > 1) return createCustomImage(VK_IMAGE_VIEW_TYPE_2D_ARRAY, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_USAGE_STORAGE_BIT
                    | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_TILING_LINEAR, VK_IMAGE_ASPECT_COLOR_BIT, cmdBuf);
            else return createCustomImage(VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_USAGE_STORAGE_BIT
                    | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_TILING_LINEAR, VK_IMAGE_ASPECT_COLOR_BIT, cmdBuf);
            break;
        case ImageType::storage3d:
            return createCustomImage(VK_IMAGE_VIEW_TYPE_3D, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_USAGE_STORAGE_BIT
                    | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_TILING_LINEAR, VK_IMAGE_ASPECT_COLOR_BIT, cmdBuf);
            break;
        case ImageType::storageCube:
            if (arrayLayers > 6) return createCustomImage(VK_IMAGE_VIEW_TYPE_CUBE_ARRAY, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_USAGE_STORAGE_BIT
                    | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_TILING_LINEAR, VK_IMAGE_ASPECT_COLOR_BIT, cmdBuf);
            else return createCustomImage(VK_IMAGE_VIEW_TYPE_CUBE, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_USAGE_STORAGE_BIT
                    | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_TILING_LINEAR, VK_IMAGE_ASPECT_COLOR_BIT, cmdBuf);
            break;
        case ImageType::hdrCube:
            if (arrayLayers > 6) return createCustomImage(VK_IMAGE_VIEW_TYPE_CUBE_ARRAY, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT 
                    | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, cmdBuf);
            else return createCustomImage(VK_IMAGE_VIEW_TYPE_CUBE, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT
                    | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, cmdBuf);
            break;
        case ImageType::colorAttachment:
            return createCustomImage(VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, cmdBuf);
            break;
        case ImageType::depthAttachment:
            return createCustomImage(VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT, cmdBuf);
            break;
    }

    return {};
}

void VulImage::createVkImage()
{
    VkImageCreateFlags createFlags = 0;
    if (m_imageViewType == VK_IMAGE_VIEW_TYPE_2D_ARRAY) createFlags = VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT;
    else if (m_imageViewType == VK_IMAGE_VIEW_TYPE_CUBE || m_imageViewType == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY)
        createFlags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = m_imageType;
    imageInfo.extent.width = m_baseWidth;
    imageInfo.extent.height = m_baseHeight;
    imageInfo.extent.depth = m_baseDepth;
    imageInfo.mipLevels = m_mipLevels.size();
    imageInfo.arrayLayers = m_arrayLayersCount;
    imageInfo.format = m_format;
    imageInfo.tiling = m_tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = m_usage;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.flags = createFlags;

    if (vkCreateImage(m_vulDevice.device(), &imageInfo, nullptr, &m_image) != VK_SUCCESS)
        throw std::runtime_error("failed to create image in VulImage");

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_vulDevice.device(), m_image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = m_vulDevice.findMemoryType(memRequirements.memoryTypeBits, m_memoryProperties);

    if (vkAllocateMemory(m_vulDevice.device(), &allocInfo, nullptr, &m_imageMemory) != VK_SUCCESS)
        throw std::runtime_error("failed to allocate image memory in VulImage");
    if (vkBindImageMemory(m_vulDevice.device(), m_image, m_imageMemory, 0) != VK_SUCCESS)
        throw std::runtime_error("Failed to bind image memory in VulImage");

    VUL_NAME_VK(m_image)
    VUL_NAME_VK(m_imageMemory)
}

VkImageView VulImage::createImageView(uint32_t baseMipLevel, uint32_t mipLevelCount)
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_image;
    viewInfo.viewType = m_imageViewType;
    viewInfo.format = m_format;
    viewInfo.subresourceRange.aspectMask = m_aspect;
    viewInfo.subresourceRange.baseMipLevel = baseMipLevel;
    viewInfo.subresourceRange.levelCount = mipLevelCount;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = m_arrayLayersCount;

    VkImageView imageView;
    if (vkCreateImageView(m_vulDevice.device(), &viewInfo, nullptr, &imageView) != VK_SUCCESS)
        throw std::runtime_error("failed to create image view");

    VUL_NAME_VK(imageView);
    return imageView;
}

void VulImage::copyBufferToImage(VkBuffer buffer, uint32_t mipLevel, VkCommandBuffer cmdBuf) 
{
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = mipLevel;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = m_arrayLayersCount;

    region.imageOffset = {0, 0, 0};
    region.imageExtent = {m_mipLevels[mipLevel].width, m_mipLevels[mipLevel].height, m_mipLevels[mipLevel].depth};

    vkCmdCopyBufferToImage(cmdBuf, buffer, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

uint32_t VulImage::alignUp(uint32_t alignee, uint32_t aligner)
{
    return (alignee % aligner) ? alignee + (aligner - (alignee % aligner)) : alignee;
}

VulImage::KtxCompressionFormatProperties VulImage::getKtxCompressionFormatProperties(KtxCompressionFormat compressionFormat)
{
    KtxCompressionFormatProperties properties{};
    switch (compressionFormat) {
        case KtxCompressionFormat::uncompressedRgba32nonLinear:
            properties.transcodeFormat = KTX_TTF_RGBA32;
            properties.vkFormat = VK_FORMAT_R8G8B8A8_SRGB;
            break;
        case KtxCompressionFormat::uncompressedRgba32linear:
            properties.transcodeFormat = KTX_TTF_RGBA32;
            properties.vkFormat = VK_FORMAT_R8G8B8A8_UNORM;
            break;
        case KtxCompressionFormat::uncompressedRgba16linear:
            properties.transcodeFormat = KTX_TTF_RGBA4444;
            properties.vkFormat = VK_FORMAT_R4G4B4A4_UNORM_PACK16;
            break;
        case KtxCompressionFormat::uncompressedRgb565linear:
            properties.transcodeFormat = KTX_TTF_RGB565;
            properties.vkFormat = VK_FORMAT_R5G6B5_UNORM_PACK16;
            break;
        case KtxCompressionFormat::uncompressedBgr565linear:
            properties.transcodeFormat = KTX_TTF_BGR565;
            properties.vkFormat = VK_FORMAT_B5G6R5_UNORM_PACK16;
            break;
        case KtxCompressionFormat::bc1rgbNonLinear:
            properties.transcodeFormat = KTX_TTF_BC1_RGB;
            properties.vkFormat = VK_FORMAT_BC1_RGB_SRGB_BLOCK;
            break;
        case KtxCompressionFormat::bc1rgbLinear:
            properties.transcodeFormat = KTX_TTF_BC1_RGB;
            properties.vkFormat = VK_FORMAT_BC1_RGB_UNORM_BLOCK;
            break;
        case KtxCompressionFormat::bc3rgbaNonLinear:
            properties.transcodeFormat = KTX_TTF_BC3_RGBA;
            properties.vkFormat = VK_FORMAT_BC3_SRGB_BLOCK;
            break;
        case KtxCompressionFormat::bc3rgbaLinear:
            properties.transcodeFormat = KTX_TTF_BC3_RGBA;
            properties.vkFormat = VK_FORMAT_BC3_UNORM_BLOCK;
            break;
        case KtxCompressionFormat::bc4rUnsigned:
            properties.transcodeFormat = KTX_TTF_BC4_R;
            properties.vkFormat = VK_FORMAT_BC4_UNORM_BLOCK;
            break;
        case KtxCompressionFormat::bc4rSigned:
            properties.transcodeFormat = KTX_TTF_BC4_R;
            properties.vkFormat = VK_FORMAT_BC4_SNORM_BLOCK;
            break;
        case KtxCompressionFormat::bc5rgUnsigned:
            properties.transcodeFormat = KTX_TTF_BC5_RG;
            properties.vkFormat = VK_FORMAT_BC5_UNORM_BLOCK;
            break;
        case KtxCompressionFormat::bc5rgSigned:
            properties.transcodeFormat = KTX_TTF_BC5_RG;
            properties.vkFormat = VK_FORMAT_BC5_SNORM_BLOCK;
            break;
        case KtxCompressionFormat::bc7rgbaNonLinear:
            properties.transcodeFormat = KTX_TTF_BC7_RGBA;
            properties.vkFormat = VK_FORMAT_BC7_SRGB_BLOCK;
            break;
        case KtxCompressionFormat::bc7rgbaLinear:
            properties.transcodeFormat = KTX_TTF_BC7_RGBA;
            properties.vkFormat = VK_FORMAT_BC7_UNORM_BLOCK;
            break;
    }
    return properties;
}

VulImage::VkFormatProperties VulImage::getVkFormatProperties(VkFormat format)
{
    VkFormatProperties properties{};
    switch (format) {
        case VK_FORMAT_R8G8B8A8_SRGB:
            properties.bitsPerTexel = 32;
            properties.sideLengthAlignment = 1;
            break;
        case VK_FORMAT_R8G8B8A8_UNORM:
            properties.bitsPerTexel = 32;
            properties.sideLengthAlignment = 1;
            break;
        case VK_FORMAT_R4G4B4A4_UNORM_PACK16:
            properties.bitsPerTexel = 16;
            properties.sideLengthAlignment = 1;
            break;
        case VK_FORMAT_R5G6B5_UNORM_PACK16:
            properties.bitsPerTexel = 16;
            properties.sideLengthAlignment = 1;
            break;
        case VK_FORMAT_B5G6R5_UNORM_PACK16:
            properties.bitsPerTexel = 16;
            properties.sideLengthAlignment = 1;
            break;
        case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
            properties.bitsPerTexel = 4;
            properties.sideLengthAlignment = 4;
            break;
        case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
            properties.bitsPerTexel = 4;
            properties.sideLengthAlignment = 4;
            break;
        case VK_FORMAT_BC3_SRGB_BLOCK:
            properties.bitsPerTexel = 8;
            properties.sideLengthAlignment = 4;
            break;
        case VK_FORMAT_BC3_UNORM_BLOCK:
            properties.bitsPerTexel = 8;
            properties.sideLengthAlignment = 4;
            break;
        case VK_FORMAT_BC4_UNORM_BLOCK:
            properties.bitsPerTexel = 4;
            properties.sideLengthAlignment = 4;
            break;
        case VK_FORMAT_BC4_SNORM_BLOCK:
            properties.bitsPerTexel = 4;
            properties.sideLengthAlignment = 4;
            break;
        case VK_FORMAT_BC5_UNORM_BLOCK:
            properties.bitsPerTexel = 8;
            properties.sideLengthAlignment = 4;
            break;
        case VK_FORMAT_BC5_SNORM_BLOCK:
            properties.bitsPerTexel = 8;
            properties.sideLengthAlignment = 4;
            break;
        case VK_FORMAT_BC7_SRGB_BLOCK:
            properties.bitsPerTexel = 8;
            properties.sideLengthAlignment = 4;
            break;
        case VK_FORMAT_BC7_UNORM_BLOCK:
            properties.bitsPerTexel = 8;
            properties.sideLengthAlignment = 4;
            break;
        case VK_FORMAT_R8_UNORM:
            properties.bitsPerTexel = 8;
            properties.sideLengthAlignment = 1;
            break;
        case VK_FORMAT_R32_UINT:
            properties.bitsPerTexel = 32;
            properties.sideLengthAlignment = 1;
            break;
        case VK_FORMAT_R32G32B32A32_SFLOAT:
            properties.bitsPerTexel = 128;
            properties.sideLengthAlignment = 1;
            break;
        case VK_FORMAT_R16G16B16A16_SFLOAT:
            properties.bitsPerTexel = 64;
            properties.sideLengthAlignment = 1;
            break;
        case VK_FORMAT_D32_SFLOAT:
            properties.bitsPerTexel = 32;
            properties.sideLengthAlignment = 1;
            break;
        default:
            throw std::invalid_argument("Unsupported VkFormat: " + std::to_string(format));
    }
    return properties;

}

}
