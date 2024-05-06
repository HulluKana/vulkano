#include <cassert>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <vul_new_image.hpp>
#include <vul_device.hpp>
#include <vul_buffer.hpp>
#include <vul_debug_tools.hpp>

#include <ktx.h>
#include <vulkan/vulkan_core.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <functional>
#include <iostream>

using namespace vulB;
namespace vul {

VulSampler::VulSampler(VulDevice &vulDevice, VkFilter filter, VkSamplerAddressMode addressMode, float maxAnisotropy,
        VkBorderColor borderColor, VkSamplerMipmapMode mipMapMode, float mipLodBias, float mipMinLod, float mipMaxLod)
    : m_vulDevice{vulDevice}
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
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

std::shared_ptr<VulSampler> VulSampler::createDefaultTexSampler(vulB::VulDevice &vulDevice, uint32_t mipLevels)
{
    std::shared_ptr<VulSampler> sampler;
    sampler.reset(new VulSampler{vulDevice, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT,
            vulDevice.properties.limits.maxSamplerAnisotropy, VK_BORDER_COLOR_INT_OPAQUE_BLACK,
            VK_SAMPLER_MIPMAP_MODE_LINEAR, 0.0f, 0.0f, static_cast<float>(mipLevels)});
    return sampler;
}

std::shared_ptr<VulSampler> VulSampler::createCustomSampler(vulB::VulDevice &vulDevice, VkFilter filter,
        VkSamplerAddressMode addressMode, float maxAnisotropy, VkBorderColor borderColor,
        VkSamplerMipmapMode mipMapMode, float mipLodBias, float mipMinLod, float mipMaxLod)
{
    std::shared_ptr<VulSampler> sampler;
    sampler.reset(new VulSampler{vulDevice, filter, addressMode, maxAnisotropy,
            borderColor, mipMapMode, mipLodBias, mipMinLod, mipMaxLod});
    return sampler;
}





VulNewImage::VulNewImage(VulDevice &vulDevice) : m_vulDevice{vulDevice}
{

}

VulNewImage::~VulNewImage()
{
    if (m_image != VK_NULL_HANDLE) vkDestroyImage(m_vulDevice.device(), m_image, nullptr);
    if (m_imageView != VK_NULL_HANDLE) vkDestroyImageView(m_vulDevice.device(), m_imageView, nullptr);
    if (m_imageMemory != VK_NULL_HANDLE) vkFreeMemory(m_vulDevice.device(), m_imageMemory, nullptr);
}

void VulNewImage::loadCompressedKtxFromFileWhole(const std::string &fileName, KtxCompressionFormat compressionFormat)
{
    loadCompressedKtxFromFile(fileName, compressionFormat, 0, 0, 69, 0, 0, 69);
}

void VulNewImage::loadCompressedKtxFromFile(const std::string &fileName, KtxCompressionFormat compressionFormat,
        uint32_t baseInputMipLevel, uint32_t baseOutputMipLevel, uint32_t mipLevelCount,
        uint32_t baseInputArrayLayer, uint32_t baseOutputArrayLayer, uint32_t arrayLayerCount)
{
    if (fileName.length() == 0 || mipLevelCount == 0 || arrayLayerCount == 0) return;

    KtxCompressionFormatProperties ktxFormatProperties = getKtxCompressionFormatProperties(compressionFormat);
    VkFormatProperties formatProperties = getVkFormatProperties(ktxFormatProperties.vkFormat);

    ktxTexture2 *ktxTexture;
    KTX_error_code result = ktxTexture2_CreateFromNamedFile(fileName.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTexture); 
    if (result != KTX_SUCCESS) throw std::runtime_error("Failed to create ktxTexture: " + std::to_string(result));

    result = ktxTexture2_TranscodeBasis(ktxTexture, ktxFormatProperties.transcodeFormat, 0);
    if (result != KTX_SUCCESS) throw std::runtime_error("Failed to transcode ktxTexture to format " +
            std::to_string(ktxFormatProperties.transcodeFormat) + ": " + std::to_string(result));

    uint32_t width = ktxTexture->baseWidth;
    uint32_t height = ktxTexture->baseHeight;
    uint32_t layerSize = alignUp(width, formatProperties.sideLengthAlignment) *
        alignUp(height, formatProperties.sideLengthAlignment) * formatProperties.bitsPerTexel / 8;
    uint8_t *pCopySrc = ktxTexture->pData + ktxTexture->dataSize - layerSize * ktxTexture->numLayers;
    for (uint32_t i = 0; i < baseInputMipLevel; i++) {
        width = std::max(width / 2, 1u);
        height = std::max(height / 2, 1u);
        pCopySrc -= layerSize * ktxTexture->numLayers;
    }

    uint32_t mipLevelCopyCount = std::min(ktxTexture->numLevels - baseInputMipLevel, mipLevelCount);
    uint32_t arrayLayerCopyCount = std::min(ktxTexture->numLayers - baseInputArrayLayer, arrayLayerCount);
    std::vector<MipLevel> levels(mipLevelCopyCount);
    for (uint32_t i = 0; i < mipLevelCopyCount; i++) {
        levels[i].width = alignUp(width, formatProperties.sideLengthAlignment);
        levels[i].height = alignUp(height, formatProperties.sideLengthAlignment);
        levels[i].depth = 1;

        pCopySrc -= layerSize * baseInputArrayLayer;
        levels[i].layers.reserve(arrayLayerCopyCount);
        for (uint32_t j = 0; j < arrayLayerCopyCount; j++) {
            levels[i].layers[j] = pCopySrc;
            pCopySrc -= layerSize;
        } 

        width = std::max(width / 2, 1u);
        height = std::max(height / 2, 1u);
        pCopySrc -= layerSize * (ktxTexture->numLayers - baseInputArrayLayer - arrayLayerCopyCount);
    }

    loadRawFromMemory(levels, ktxFormatProperties.vkFormat, baseOutputMipLevel, baseOutputArrayLayer); 
    free(ktxTexture);
}

void VulNewImage::loadRawFromMemoryWhole(const std::vector<MipLevel> &mipLevels, VkFormat format)
{
    loadRawFromMemory(mipLevels, format, 0, 0);
}

void VulNewImage::loadRawFromMemory(const std::vector<MipLevel> &mipLevels, VkFormat format, uint32_t baseOutputMipLevel,
        uint32_t baseOutputArrayLayer)
{
    if (mipLevels.size() == 0) return;
    assert(format == m_format || m_format == VK_FORMAT_UNDEFINED);
    assert(baseOutputMipLevel <= m_mipLevels.size());
    if (m_mipLevels.size() > 0)
        for (size_t i = 0; i < m_mipLevels.size(); i++) assert(baseOutputArrayLayer <= m_mipLevels[i].layers.size());
    else assert(baseOutputArrayLayer == 0);

    VkFormatProperties formatProperties = getVkFormatProperties(format);
    size_t requiredSize = 0; 
    for (uint32_t i = 0; i < baseOutputMipLevel; i++) {
        assert(m_mipLevels[i].width > 0);
        assert(m_mipLevels[i].height > 0);
        assert(m_mipLevels[i].depth > 0);
        assert(m_mipLevels[i].layers.size() > 0);
        requiredSize += m_mipLevels[i].width * m_mipLevels[i].height * m_mipLevels[i].depth *
            formatProperties.bitsPerTexel / 8 * m_mipLevels[i].layers.size();
    }

    m_mipLevels.reserve(mipLevels.size());
    for (size_t i = 0; i < mipLevels.size(); i++) {
        assert(mipLevels[i].width > 0);
        assert(mipLevels[i].height > 0);
        assert(mipLevels[i].depth > 0);
        if (i + baseOutputMipLevel <= m_mipLevels.size() && baseOutputArrayLayer > 0) {
            assert(m_mipLevels[i + baseOutputMipLevel].width > 0);
            assert(m_mipLevels[i + baseOutputMipLevel].height > 0);
            assert(m_mipLevels[i + baseOutputMipLevel].depth > 0);
            assert(m_mipLevels[i + baseOutputMipLevel].layers.size() > 0);
            assert(m_mipLevels[i + baseOutputMipLevel].width == mipLevels[i].width);
            assert(m_mipLevels[i + baseOutputMipLevel].height == mipLevels[i].height);
            assert(m_mipLevels[i + baseOutputMipLevel].depth == mipLevels[i].depth);
        }
        m_mipLevels[i + baseOutputMipLevel].width = alignUp(mipLevels[i].width, formatProperties.sideLengthAlignment);
        m_mipLevels[i + baseOutputMipLevel].height = alignUp(mipLevels[i].height, formatProperties.sideLengthAlignment);
        m_mipLevels[i + baseOutputMipLevel].depth = alignUp(mipLevels[i].depth, formatProperties.sideLengthAlignment);
        requiredSize += m_mipLevels[i + baseOutputMipLevel].width * m_mipLevels[i + baseOutputMipLevel].height *
            m_mipLevels[i + baseOutputMipLevel].depth * formatProperties.bitsPerTexel / 8 *
            (baseOutputArrayLayer + mipLevels[i].layers.size());
    }

    uint8_t *oldData = nullptr;
    if (m_data != nullptr) oldData =  m_data.release();
    if (oldData == nullptr) {
        assert(baseOutputMipLevel == 0);
        assert(baseOutputArrayLayer == 0);
    }
    m_data = std::unique_ptr<uint8_t>(new uint8_t[requiredSize]());
    assert(m_data != nullptr);
    m_dataSize = requiredSize;
   
    for (uint32_t i = 0; i < baseOutputMipLevel; i++) {
        size_t layerSize = m_mipLevels[i].width * m_mipLevels[i].height * m_mipLevels[i].depth * formatProperties.bitsPerTexel / 8;
        for (size_t j = 0; j < m_mipLevels[i].layers.size(); j++) {
            if (m_mipLevels[i].layers[j] == nullptr) continue;
            size_t offset = static_cast<uint8_t *>(m_mipLevels[i].layers[j]) - oldData;
            memcpy(m_data.get() + offset, oldData + offset, layerSize);
            m_mipLevels[i].layers[j] = m_data.get() + layerSize * j;
        }
    }
    size_t offset = 0;
    for (size_t i = 0; i < mipLevels.size(); i++) {
        uint32_t m_i = i + baseOutputMipLevel;
        size_t layerSize = m_mipLevels[m_i].width * m_mipLevels[m_i].height * m_mipLevels[m_i].depth * formatProperties.bitsPerTexel / 8;
        for (uint32_t j = 0; j < baseOutputArrayLayer; j++) {
            if (m_mipLevels[m_i].layers[j] != nullptr) {
                size_t oldOffset = static_cast<uint8_t *>(m_mipLevels[m_i].layers[j]) - oldData;
                memcpy(m_data.get() + offset, oldData + oldOffset, layerSize);
                m_mipLevels[m_i].layers[j] = m_data.get() + offset;
            }
            offset += layerSize;
        }
        for (size_t j = 0; j < mipLevels[i].layers.size(); j++) {
            if (mipLevels[m_i].layers[j] != nullptr) {
                memcpy(m_data.get() + offset, mipLevels[m_i].layers[j], layerSize);
                m_mipLevels[m_i].layers[j + baseOutputArrayLayer] = m_data.get() + offset; 
            }
            offset += layerSize;
        }
    }
}

uint32_t VulNewImage::alignUp(uint32_t alignee, uint32_t aligner)
{
    return (alignee + aligner - 1) & ~(alignee - 1);
}

VulNewImage::KtxCompressionFormatProperties VulNewImage::getKtxCompressionFormatProperties(KtxCompressionFormat compressionFormat)
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

VulNewImage::VkFormatProperties VulNewImage::getVkFormatProperties(VkFormat format)
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
        default:
            assert(0 && ("Unsupported VkFormat: " + std::to_string(format)).c_str());
    }
    return properties;

}

}
