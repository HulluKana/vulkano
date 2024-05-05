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
    KtxCompressionFormatProperties formatProperties = getKtxCompressionFormatProperties(compressionFormat);
    ktxTexture2 *ktxTexture;
    KTX_error_code result = ktxTexture2_CreateFromNamedFile(fileName.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTexture); 
    if (result != KTX_SUCCESS) throw std::runtime_error("Failed to create ktxTexture: " + std::to_string(result));

    result = ktxTexture2_TranscodeBasis(ktxTexture, formatProperties.transcodeFormat, 0);
    if (result != KTX_SUCCESS) throw std::runtime_error("Failed to transcode ktxTexture to format " +
            std::to_string(formatProperties.transcodeFormat) + ": " + std::to_string(result));

    std::function<uint32_t(uint32_t, uint32_t)> alignUp = [](uint32_t alignee, uint32_t aligner)
        {return (alignee + aligner - 1) & ~(alignee - 1);};

    uint32_t width = ktxTexture->baseWidth;
    uint32_t height = ktxTexture->baseHeight;
    uint32_t layerSize = alignUp(width, formatProperties.sideLengthAlignment) *
        alignUp(height, formatProperties.sideLengthAlignment) * formatProperties.bitsPerTexel / 8;
    uint8_t *pCopySrc = ktxTexture->pData + ktxTexture->dataSize - layerSize * ktxTexture->numLayers;
    for (uint32_t i = 0; i < baseInputMipLevel; i++) {
        width = std::min(width / 2, 1u);
        height = std::min(height / 2, 1u);
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

        width = std::min(width / 2, 1u);
        height = std::min(height / 2, 1u);
        pCopySrc -= layerSize * (ktxTexture->numLayers - baseInputArrayLayer - arrayLayerCopyCount);
    }

    loadRawFromMemory(levels, formatProperties.vkFormat, baseOutputMipLevel, baseOutputArrayLayer); 
    free(ktxTexture);
}

void VulNewImage::loadRawFromMemory(const std::vector<MipLevel> &mipLevels, VkFormat format, uint32_t baseOutputMipLevel,
        uint32_t baseOutputArrayLayer)
{

}

VulNewImage::KtxCompressionFormatProperties VulNewImage::getKtxCompressionFormatProperties(KtxCompressionFormat compressionFormat)
{
    KtxCompressionFormatProperties properties{};
    switch (compressionFormat) {
        case KtxCompressionFormat::uncompressedRgba32nonLinear:
            properties.transcodeFormat = KTX_TTF_RGBA32;
            properties.vkFormat = VK_FORMAT_R8G8B8A8_SRGB;
            properties.bitsPerTexel = 32;
            properties.sideLengthAlignment = 1;
            break;
        case KtxCompressionFormat::uncompressedRgba32linear:
            properties.transcodeFormat = KTX_TTF_RGBA32;
            properties.vkFormat = VK_FORMAT_R8G8B8A8_UNORM;
            properties.bitsPerTexel = 32;
            properties.sideLengthAlignment = 1;
            break;
        case KtxCompressionFormat::uncompressedRgba16linear:
            properties.transcodeFormat = KTX_TTF_RGBA4444;
            properties.vkFormat = VK_FORMAT_R4G4B4A4_UNORM_PACK16;
            properties.bitsPerTexel = 16;
            properties.sideLengthAlignment = 1;
            break;
        case KtxCompressionFormat::uncompressedRgb565linear:
            properties.transcodeFormat = KTX_TTF_RGB565;
            properties.vkFormat = VK_FORMAT_R5G6B5_UNORM_PACK16;
            properties.bitsPerTexel = 16;
            properties.sideLengthAlignment = 1;
            break;
        case KtxCompressionFormat::uncompressedBgr565linear:
            properties.transcodeFormat = KTX_TTF_BGR565;
            properties.vkFormat = VK_FORMAT_B5G6R5_UNORM_PACK16;
            properties.bitsPerTexel = 16;
            properties.sideLengthAlignment = 1;
            break;
        case KtxCompressionFormat::bc1rgbNonLinear:
            properties.transcodeFormat = KTX_TTF_BC1_RGB;
            properties.vkFormat = VK_FORMAT_BC1_RGB_SRGB_BLOCK;
            properties.bitsPerTexel = 4;
            properties.sideLengthAlignment = 4;
            break;
        case KtxCompressionFormat::bc1rgbLinear:
            properties.transcodeFormat = KTX_TTF_BC1_RGB;
            properties.vkFormat = VK_FORMAT_BC1_RGB_UNORM_BLOCK;
            properties.bitsPerTexel = 4;
            properties.sideLengthAlignment = 4;
            break;
        case KtxCompressionFormat::bc3rgbaNonLinear:
            properties.transcodeFormat = KTX_TTF_BC3_RGBA;
            properties.vkFormat = VK_FORMAT_BC3_SRGB_BLOCK;
            properties.bitsPerTexel = 8;
            properties.sideLengthAlignment = 4;
            break;
        case KtxCompressionFormat::bc3rgbaLinear:
            properties.transcodeFormat = KTX_TTF_BC3_RGBA;
            properties.vkFormat = VK_FORMAT_BC3_UNORM_BLOCK;
            properties.bitsPerTexel = 8;
            properties.sideLengthAlignment = 4;
            break;
        case KtxCompressionFormat::bc4rUnsigned:
            properties.transcodeFormat = KTX_TTF_BC4_R;
            properties.vkFormat = VK_FORMAT_BC4_UNORM_BLOCK;
            properties.bitsPerTexel = 4;
            properties.sideLengthAlignment = 4;
            break;
        case KtxCompressionFormat::bc4rSigned:
            properties.transcodeFormat = KTX_TTF_BC4_R;
            properties.vkFormat = VK_FORMAT_BC4_SNORM_BLOCK;
            properties.bitsPerTexel = 4;
            properties.sideLengthAlignment = 4;
            break;
        case KtxCompressionFormat::bc5rgUnsigned:
            properties.transcodeFormat = KTX_TTF_BC5_RG;
            properties.vkFormat = VK_FORMAT_BC5_UNORM_BLOCK;
            properties.bitsPerTexel = 8;
            properties.sideLengthAlignment = 4;
            break;
        case KtxCompressionFormat::bc5rgSigned:
            properties.transcodeFormat = KTX_TTF_BC5_RG;
            properties.vkFormat = VK_FORMAT_BC5_SNORM_BLOCK;
            properties.bitsPerTexel = 8;
            properties.sideLengthAlignment = 4;
            break;
        case KtxCompressionFormat::bc7rgbaNonLinear:
            properties.transcodeFormat = KTX_TTF_BC7_RGBA;
            properties.vkFormat = VK_FORMAT_BC7_SRGB_BLOCK;
            properties.bitsPerTexel = 8;
            properties.sideLengthAlignment = 4;
            break;
        case KtxCompressionFormat::bc7rgbaLinear:
            properties.transcodeFormat = KTX_TTF_BC7_RGBA;
            properties.vkFormat = VK_FORMAT_BC7_UNORM_BLOCK;
            properties.bitsPerTexel = 8;
            properties.sideLengthAlignment = 4;
            break;
    }
    return properties;
}

}
