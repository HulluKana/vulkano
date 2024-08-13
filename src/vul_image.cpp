#include <OpenEXR/ImfRgba.h>
#include <cassert>
#include <cmath>
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
    if (m_imageView != VK_NULL_HANDLE) vkDestroyImageView(m_vulDevice.device(), m_imageView, nullptr);
    if (m_image != VK_NULL_HANDLE && m_ownsImage) vkDestroyImage(m_vulDevice.device(), m_image, nullptr);
    if (m_imageMemory != VK_NULL_HANDLE) vkFreeMemory(m_vulDevice.device(), m_imageMemory, nullptr);
    for (VkImageView imageView : m_mipImageViews) vkDestroyImageView(m_vulDevice.device(), imageView, nullptr);
}

void VulImage::loadCompressedKtxFromFile(const std::string &fileName, KtxCompressionFormat compressionFormat,
        uint32_t baseInputMipLevel, uint32_t baseOutputMipLevel, uint32_t mipLevelCount,
        uint32_t baseInputArrayLayer, uint32_t baseOutputArrayLayer, uint32_t arrayLayerCount)
{
    if (fileName.length() == 0 || mipLevelCount == 0 || arrayLayerCount == 0) return;

    KtxCompressionFormatProperties ktxFormatProperties = getKtxCompressionFormatProperties(compressionFormat);
    VkFormatProperties formatProperties = getVkFormatProperties(ktxFormatProperties.vkFormat);

    static double k = 0.0;
    static double l = 0.0;

    double m = glfwGetTime();
    ktxTexture2 *origTexture;
    KTX_error_code result = ktxTexture2_CreateFromNamedFile(fileName.c_str(), KTX_TEXTURE_CREATE_NO_FLAGS, &origTexture); 
    if (result != KTX_SUCCESS) throw std::runtime_error("Failed to create ktxTexture. File: " + fileName + " Error code: " + std::to_string(result));
    k += glfwGetTime() - m;

    ktxTextureCreateInfo createInfo{};
    createInfo.baseWidth = origTexture->baseWidth / 64;
    createInfo.baseHeight = origTexture->baseHeight / 64;
    createInfo.baseDepth = origTexture->baseDepth;
    createInfo.numLevels = origTexture->numLevels - 6;
    createInfo.numFaces = origTexture->numFaces;
    createInfo.numLayers = origTexture->numLayers;
    createInfo.numDimensions = origTexture->numDimensions;
    createInfo.generateMipmaps = origTexture->generateMipmaps;
    createInfo.pDfd = origTexture->pDfd;
    ktxTexture2 *texture;
    ktxTexture2_Create(&createInfo, KTX_TEXTURE_CREATE_ALLOC_STORAGE, &texture);
    origTexture->dataSize = texture->dataSize;
    texture->vtbl->LoadImageData(ktxTexture(origTexture), texture->pData, texture->dataSize);

    double c = glfwGetTime();
    result = ktxTexture2_TranscodeBasis(texture, ktxFormatProperties.transcodeFormat, 0);
    if (result != KTX_SUCCESS) throw std::runtime_error("Failed to transcode ktxTexture to format " +
            std::to_string(ktxFormatProperties.transcodeFormat) + " File: " + fileName + " Error code: " + std::to_string(result));
    l += glfwGetTime() - c;

    std::cout << k << " " << l << "\n";

    const uint32_t baseWidth = alignUp(texture->baseWidth * std::pow(2, baseOutputMipLevel), formatProperties.sideLengthAlignment);
    const uint32_t baseHeight = alignUp(texture->baseHeight * std::pow(2, baseOutputMipLevel), formatProperties.sideLengthAlignment);
    const uint32_t baseDepth = texture->baseDepth * std::pow(2, baseOutputMipLevel);

    uint32_t mipLevelCopyCount = std::min(texture->numLevels - baseInputMipLevel, mipLevelCount);
    uint32_t arrayLayerCopyCount = std::min(texture->numLayers - baseInputArrayLayer, arrayLayerCount);
    std::vector<std::vector<void *>> data(mipLevelCopyCount);
    uint8_t *pCopySrc = texture->pData;
    for (int i = mipLevelCopyCount - 1; i >= 0; i--) {
        const uint32_t width = std::max(baseWidth / static_cast<uint32_t>(std::pow(2, i + baseOutputMipLevel)), 1u);
        const uint32_t height = std::max(baseHeight / static_cast<uint32_t>(std::pow(2, i + baseOutputMipLevel)), 1u);
        const uint32_t depth = std::max(baseDepth / static_cast<uint32_t>(std::pow(2, i + baseOutputMipLevel)), 1u);
        uint32_t layerSize = alignUp(width, formatProperties.sideLengthAlignment) *
            alignUp(height, formatProperties.sideLengthAlignment) * depth * formatProperties.bitsPerTexel / 8;

        pCopySrc += layerSize * baseInputArrayLayer;
        data[i].resize(arrayLayerCopyCount);
        for (uint32_t j = 0; j < arrayLayerCopyCount; j++) {
            data[i][j] = pCopySrc;
            pCopySrc += layerSize;
        } 

        pCopySrc += layerSize * (texture->numLayers - baseInputArrayLayer - arrayLayerCopyCount);
    }

    loadRawFromMemory(baseWidth, baseHeight, baseDepth, data, ktxFormatProperties.vkFormat, baseOutputMipLevel, baseOutputArrayLayer); 
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

    std::unique_ptr<Imf::Rgba> pixels(new Imf::Rgba[width * height * 6]());
    file.setFrameBuffer(pixels.get(), 1, width);
    file.readPixels(dataWindow.min.y, dataWindow.max.y);

    std::vector<void *> mipLevel(6);
        mipLevel[0] = pixels.get() + height * width * 0;
        mipLevel[1] = pixels.get() + height * width * 1;
        mipLevel[2] = pixels.get() + height * width * 2;
        mipLevel[3] = pixels.get() + height * width * 3;
        mipLevel[4] = pixels.get() + height * width * 5;
        mipLevel[5] = pixels.get() + height * width * 4;
    loadRawFromMemory(width, height, 1, {mipLevel}, VK_FORMAT_R16G16B16A16_SFLOAT, 0, 0);
}

void VulImage::loadRawFromMemory(uint32_t baseWidth, uint32_t baseHeight, uint32_t baseDepth,
        const std::vector<std::vector<void *>> &data, VkFormat format, uint32_t baseOutputMipLevel, uint32_t baseOutputArrayLayer)
{
    if (data.size() == 0) return;
    size_t arrayCount = data[0].size();
    uint32_t firstMip = 0;
    for (size_t i = 0; i < data.size(); i++) {
        size_t nextArrayCount = data[std::min(i + 1, data.size())].size();
        if (arrayCount != nextArrayCount || i >= data.size() - 1) {
            keepEmpty(baseWidth, baseHeight, baseDepth, i - firstMip + 1, arrayCount, format, baseOutputMipLevel + firstMip, baseOutputArrayLayer);
            arrayCount = nextArrayCount;
            firstMip = i + 1;
        }
    }

    size_t offset = 0;
    for (size_t i = 0; i < data.size(); i++) {
        uint32_t m_i = i + baseOutputMipLevel;
        offset += m_mipLevels[m_i].layerSize * baseOutputArrayLayer;
        for (size_t j = 0; j < data[i].size(); j++) {
            m_mipLevels[m_i].containsData[j] = false;
            if (data[i][j] != nullptr) {
                memcpy(m_data.get() + offset, data[i][j], m_mipLevels[m_i].layerSize);
                m_mipLevels[m_i].containsData[j] = true;
            }
            m_mipLevels[m_i].layers[j + baseOutputArrayLayer] = m_data.get() + offset; 
            offset += m_mipLevels[m_i].layerSize;
        }
    }
}

void VulImage::keepEmpty(uint32_t baseWidth, uint32_t baseHeight, uint32_t baseDepth, uint32_t mipCount,
        uint32_t arrayCount, VkFormat format, uint32_t baseOutputMipLevel, uint32_t baseOutputArrayLayer)
{
    assert(baseWidth > 0);
    assert(baseHeight > 0);
    assert(baseDepth > 0);
    assert(m_baseWidth == 0 || m_baseWidth == baseWidth);
    assert(m_baseHeight == 0 || m_baseHeight == baseHeight);
    assert(m_baseDepth == 0 || m_baseDepth == baseDepth);
    assert(format == m_format || m_format == VK_FORMAT_UNDEFINED);
    assert(baseOutputMipLevel <= m_mipLevels.size());
    if (m_mipLevels.size() > 0)
        for (size_t i = 0; i < m_mipLevels.size(); i++) assert(baseOutputArrayLayer <= m_mipLevels[i].layers.size());
    else assert(baseOutputArrayLayer == 0);

    VkFormatProperties formatProperties = getVkFormatProperties(format);
    m_baseWidth = baseWidth;
    m_baseHeight = baseHeight;
    m_baseDepth = baseDepth;
    m_format = format;
    m_bitsPerTexel = formatProperties.bitsPerTexel;

    size_t requiredSize = 0; 
    for (uint32_t i = 0; i < baseOutputMipLevel; i++) {
        assert(m_mipLevels[i].width > 0);
        assert(m_mipLevels[i].height > 0);
        assert(m_mipLevels[i].depth > 0);
        assert(m_mipLevels[i].layers.size() > 0);
        requiredSize += m_mipLevels[i].width * m_mipLevels[i].height * m_mipLevels[i].depth *
            formatProperties.bitsPerTexel / 8 * m_mipLevels[i].layers.size();
    }

    m_mipLevels.resize(std::max(mipCount + baseOutputMipLevel, static_cast<uint32_t>(m_mipLevels.size())));
    for (uint32_t i = 0; i < mipCount; i++) {
        const uint32_t width = std::max(baseWidth / static_cast<uint32_t>(std::pow(2, i + baseOutputMipLevel)), 1u);
        const uint32_t height = std::max(baseHeight / static_cast<uint32_t>(std::pow(2, i + baseOutputMipLevel)), 1u);
        const uint32_t depth = std::max(baseDepth / static_cast<uint32_t>(std::pow(2, i + baseOutputMipLevel)), 1u);
        if (i + baseOutputMipLevel <= m_mipLevels.size() && baseOutputArrayLayer > 0) {
            assert(m_mipLevels[i + baseOutputMipLevel].width > 0);
            assert(m_mipLevels[i + baseOutputMipLevel].height > 0);
            assert(m_mipLevels[i + baseOutputMipLevel].depth > 0);
            assert(m_mipLevels[i + baseOutputMipLevel].layers.size() > 0);
            assert(m_mipLevels[i + baseOutputMipLevel].width == width);
            assert(m_mipLevels[i + baseOutputMipLevel].height == height);
            assert(m_mipLevels[i + baseOutputMipLevel].depth == depth);
        }
        m_mipLevels[i + baseOutputMipLevel].width = width;
        m_mipLevels[i + baseOutputMipLevel].height = height;
        m_mipLevels[i + baseOutputMipLevel].depth = depth;
        m_mipLevels[i + baseOutputMipLevel].layerSize = alignUp(width, formatProperties.sideLengthAlignment)
            * alignUp(height, formatProperties.sideLengthAlignment) * depth * formatProperties.bitsPerTexel / 8;
        requiredSize += m_mipLevels[i + baseOutputMipLevel].layerSize * (baseOutputArrayLayer + arrayCount);
    }
    m_dataSize = requiredSize;

    uint8_t *oldData = nullptr;
    if (m_data != nullptr) oldData =  m_data.release();
    if (oldData == nullptr) {
        assert(baseOutputMipLevel == 0);
        assert(baseOutputArrayLayer == 0);
    }
    m_data = std::unique_ptr<uint8_t>(new uint8_t[m_dataSize]());
    assert(m_data != nullptr);
   
    for (uint32_t i = 0; i < baseOutputMipLevel; i++) {
        for (size_t j = 0; j < m_mipLevels[i].layers.size(); j++) {
            if (m_mipLevels[i].layers[j] == nullptr) continue;
            size_t offset = static_cast<uint8_t *>(m_mipLevels[i].layers[j]) - oldData;
            memcpy(m_data.get() + offset, oldData + offset, m_mipLevels[i].layerSize);
            m_mipLevels[i].layers[j] = m_data.get() + m_mipLevels[i].layerSize * j;
        }
    }
    size_t offset = 0;
    for (uint32_t i = baseOutputMipLevel; i < baseOutputMipLevel + mipCount; i++) {
        m_mipLevels[i].layers.resize(std::max(arrayCount + baseOutputArrayLayer, static_cast<uint32_t>(m_mipLevels[i].layers.size())));
        m_mipLevels[i].containsData.resize(std::max(arrayCount + baseOutputArrayLayer, static_cast<uint32_t>(m_mipLevels[i].layers.size())));
        for (uint32_t j = 0; j < baseOutputArrayLayer; j++) {
            m_mipLevels[i].containsData[j] = false;
            if (m_mipLevels[i].layers[j] != nullptr) {
                size_t oldOffset = static_cast<uint8_t *>(m_mipLevels[i].layers[j]) - oldData;
                memcpy(m_data.get() + offset, oldData + oldOffset, m_mipLevels[i].layerSize);
                m_mipLevels[i].containsData[j] = true;
            }
            m_mipLevels[i].layers[j] = m_data.get() + offset;
            offset += m_mipLevels[i].layerSize;
        }
        offset += m_mipLevels[i].layerSize * mipCount;
    }

    if (oldData != nullptr) delete[] oldData; 
}

void VulImage::createCustomImage(VkImageViewType type, VkImageLayout layout, VkImageUsageFlags usage,
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
    for (uint32_t i = 1; i < m_mipLevelsCount; i++) assert(m_arrayLayersCount == m_mipLevels[i].layers.size());
    assert(m_arrayLayersCount >= 1);
    assert(!(m_arrayLayersCount > 1 && type == VK_IMAGE_VIEW_TYPE_3D));
    uint32_t maxSize = std::max(m_mipLevels[0].width, m_mipLevels[0].height);
    maxSize = std::max(maxSize, m_mipLevels[0].depth);
    m_mipLevelsCount = std::min(static_cast<uint32_t>(m_mipLevels.size()), static_cast<uint32_t>(std::log2(maxSize)));

    createVkImage();
    m_imageView = createImageView(0, m_mipLevelsCount);

    const bool isDeviceLocal = memoryProperties & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    bool containsData = false;
    for (uint32_t i = 0; i < m_mipLevelsCount; i++) for (size_t j = 0; j < m_mipLevels[i].containsData.size(); j++)
        containsData = containsData || m_mipLevels[i].containsData[j];
    if (containsData && isDeviceLocal) {
        transitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, cmdBuf);
        for (size_t i = 0; i < m_mipLevelsCount; i++) {
            m_stagingBuffers.push_back(std::make_unique<vul::VulBuffer>(1, m_mipLevels[i].layerSize * m_arrayLayersCount, false, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, m_vulDevice));
            m_stagingBuffers[m_stagingBuffers.size() - 1]->writeData(m_mipLevels[i].layers[0], m_stagingBuffers[m_stagingBuffers.size() - 1]->getBufferSize(), 0, VK_NULL_HANDLE);
            copyBufferToImage(m_stagingBuffers[m_stagingBuffers.size() - 1]->getBuffer(), i, cmdBuf);

            VUL_NAME_VK(m_stagingBuffers[m_stagingBuffers.size() - 1]->getBuffer())
        }
        transitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, layout, cmdBuf);
    }
    else transitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, layout, cmdBuf);
}

void VulImage::createFromVkImage(VkImage image, VkImageViewType type, VkFormat format, VkImageAspectFlags aspect,
        uint32_t mipLevelCount, uint32_t arrayLayerCount)
{
    m_ownsImage = false;
    m_image = image;
    m_imageViewType = type;
    m_format = format;
    m_mipLevelsCount = mipLevelCount;
    m_arrayLayersCount = arrayLayerCount;
    m_aspect = aspect;
    m_imageView = createImageView(0, m_mipLevelsCount);
}

void VulImage::createImageViewsForMipMaps()
{
    m_mipImageViews.resize(m_mipLevelsCount);
    for (uint32_t i = 0; i < m_mipLevelsCount; i++) {
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
    barrier.subresourceRange.levelCount = m_mipLevelsCount;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = m_arrayLayersCount;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

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
            img->loadUncompressedFromFileWhole(std::get<std::string>(data));
            break;
        case InputDataType::rawData:
            RawImageData rawData = std::get<RawImageData>(data);
            img->loadRawFromMemoryWhole(rawData.baseWidth, rawData.baseHeight, rawData.baseDepth,
                    rawData.data, std::get<VkFormat>(format));
            break;
    }
    img->createDefaultImage(imageType, cmdBuf);
    if (addSampler) img->vulSampler = VulSampler::createDefaultTexSampler(vulDevice, img->getMipCount());
    return img;
}

void VulImage::loadCompressedKtxFromFileWhole(const std::string &fileName, KtxCompressionFormat compressionFormat)
{
    loadCompressedKtxFromFile(fileName, compressionFormat, 0, 0, 69, 0, 0, 69);
}

void VulImage::loadUncompressedFromFileWhole(const std::string &filename)
{
    assert(0 && "Not implemented");
}

void VulImage::loadRawFromMemoryWhole(uint32_t baseWidth, uint32_t baseHeight, uint32_t baseDepth,
        const std::vector<std::vector<void *>> &data, VkFormat format)
{
    loadRawFromMemory(baseWidth, baseHeight, baseDepth, data, format, 0, 0);
}

void VulImage::keepRegularRaw2d8bitRgbaEmpty(uint32_t width, uint32_t height)
{
    keepEmpty(width, height, 1, 1, 1, VK_FORMAT_R8G8B8A8_SRGB, 0, 0);
}

void VulImage::keepRegularRaw2d32bitRgbaEmpty(uint32_t width, uint32_t height)
{
    keepEmpty(width, height, 1, 1, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 0, 0);
}

void VulImage::createDefaultImage(ImageType type, VkCommandBuffer cmdBuf)
{
    assert(m_mipLevels.size() > 0);
    size_t arrayLayers = m_mipLevels[0].layers.size();
    switch (type) {
        case ImageType::texture1d:
            if (arrayLayers > 1) createCustomImage(VK_IMAGE_VIEW_TYPE_1D_ARRAY, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT |
                    VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, cmdBuf);
            else createCustomImage(VK_IMAGE_VIEW_TYPE_1D, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT
                    | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, cmdBuf);
            break;
        case ImageType::texture2d:
            if (arrayLayers > 1) createCustomImage(VK_IMAGE_VIEW_TYPE_2D_ARRAY, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT
                    | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, cmdBuf);
            else createCustomImage(VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT
                    | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, cmdBuf);
            break;
        case ImageType::texture3d:
            createCustomImage(VK_IMAGE_VIEW_TYPE_3D, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT
                    | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, cmdBuf);
            break;
        case ImageType::textureCube:
            if (arrayLayers > 6) createCustomImage(VK_IMAGE_VIEW_TYPE_CUBE_ARRAY, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT
                    | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, cmdBuf);
            else createCustomImage(VK_IMAGE_VIEW_TYPE_CUBE, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT
                    | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, cmdBuf);
            break;
        case ImageType::storage1d:
            if (arrayLayers > 1) createCustomImage(VK_IMAGE_VIEW_TYPE_1D_ARRAY, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_USAGE_STORAGE_BIT
                    | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_TILING_LINEAR, VK_IMAGE_ASPECT_COLOR_BIT, cmdBuf);
            else createCustomImage(VK_IMAGE_VIEW_TYPE_1D, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_USAGE_STORAGE_BIT
                    | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_TILING_LINEAR, VK_IMAGE_ASPECT_COLOR_BIT, cmdBuf);
            break;
        case ImageType::storage2d:
            if (arrayLayers > 1) createCustomImage(VK_IMAGE_VIEW_TYPE_2D_ARRAY, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_USAGE_STORAGE_BIT
                    | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_TILING_LINEAR, VK_IMAGE_ASPECT_COLOR_BIT, cmdBuf);
            else createCustomImage(VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_USAGE_STORAGE_BIT
                    | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_TILING_LINEAR, VK_IMAGE_ASPECT_COLOR_BIT, cmdBuf);
            break;
        case ImageType::storage3d:
            createCustomImage(VK_IMAGE_VIEW_TYPE_3D, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_USAGE_STORAGE_BIT
                    | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_TILING_LINEAR, VK_IMAGE_ASPECT_COLOR_BIT, cmdBuf);
            break;
        case ImageType::storageCube:
            if (arrayLayers > 6) createCustomImage(VK_IMAGE_VIEW_TYPE_CUBE_ARRAY, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_USAGE_STORAGE_BIT
                    | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_TILING_LINEAR, VK_IMAGE_ASPECT_COLOR_BIT, cmdBuf);
            else createCustomImage(VK_IMAGE_VIEW_TYPE_CUBE, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_USAGE_STORAGE_BIT
                    | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_TILING_LINEAR, VK_IMAGE_ASPECT_COLOR_BIT, cmdBuf);
            break;
        case ImageType::hdrCube:
            if (arrayLayers > 6) createCustomImage(VK_IMAGE_VIEW_TYPE_CUBE_ARRAY, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT 
                    | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, cmdBuf);
            else createCustomImage(VK_IMAGE_VIEW_TYPE_CUBE, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT
                    | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, cmdBuf);
            break;
        case ImageType::colorAttachment:
            createCustomImage(VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, cmdBuf);
            break;
        case ImageType::depthAttachment:
            createCustomImage(VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT, cmdBuf);
            break;
    }
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
    imageInfo.mipLevels = m_mipLevelsCount;
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
    vkBindImageMemory(m_vulDevice.device(), m_image, m_imageMemory, 0);

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
