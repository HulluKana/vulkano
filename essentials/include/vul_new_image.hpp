#pragma once

#include "vul_device.hpp"
#include <vul_buffer.hpp>
#include <vulkan/vulkan_core.h>

/*
REQUIRED FEATURES
    Load raw data from memory
 X  Load uncompressed from regular file
 X  Load compressed from ktx

    Create mipmaps from several uncompressed files
 X  Create mipmaps from compressed ktx file
    Create mipmaps from memory
    Create mipmaps from several memory
    Create mipmaps from base image at runtime for raw images

    Support 1D, 2D, 3D and cubemap images
 X  Support 1, 2 and 4 channels for raw images 
    Support multiple layers for all except 3D
    Support mipmaps for all sampled variants, including their layered variants

    Support sampled format for all image types that vulkan supports sampled for 
    Support storage format for all image types that vulkan supports storage for 
    Support all bc compression formats except bc2 for sampled images
 X  Support 8bit unorm and srgb for sampled images
    Support 16bit sfloat for sampled images
 X  Support 32bit sfloat and uint for storage images

 X  Create device local images
 X  Create host visible and coherent images

 X  Modify device local images
 X  Modify host visible images
    Modify individual mipmap levels
    Modify individual array layers, including those in mipmaps

    Read data from device local images
    Read data from host visible images
    Read data from individual mipmap levels
    Read data from individual array layers, including those in mipmaps

 X  Allow creating few different good types of images using higher level and easier to use function
 X  Allow creating all types of supported images using lower level and harder to use function
    Allow passing own command buffer to both low and high level functions
 X  Allow not having to pass own command buffer to both low and high level functions
 X  Allow passing image data at the creation time
    Allow not passing image data at the creation time

OPTIONAL FEATURES
    Load uncompressed from memory
    Load compressed from memory
    Load uncompressed from ktx

    Create mipmaps from several compressed files

    Support 3 channels for raw images

    Support bc2 compression format for sampled images
    Support all 8, 16 and 32 bits for sampled images
    Support all 8, 16 and 32 bits for storage images
    Support single channel 64bit uint and sint for storage images

 X  Support sharing sampler
    Support sharing image source

    Create host visible and host cached, but not host coherent images
    Create host visible, host cached and host coherent images

    Allow creating all types of supported images using higher level and easier to use function

    Allow using image as attachment image

THE FOLLOWING MUST BE SPECIFIED BY DATA LOADER
Mip level count
Width, height and depth of each mip level
Pointer to the start of each mip levels data, or nullptr to signal lack of data
Array layer count
Amount of dimensions
If image has 2 dimensions and array layer count is multiple of 6, whether to interpret it as cube map or not
Data format
Size of each texel
Alignment of each texel
Total data size
Pointer to start of the data, or nullptr to signal lack of data
Whether the data can be freed or not
*/

namespace vul {

class VulNewImage {
    public:
        VulNewImage(vulB::VulDevice &vulDevice);
        ~VulNewImage();

        VulNewImage(const VulNewImage &) = delete;
        VulNewImage &operator=(const VulNewImage &) = delete;
        VulNewImage(VulNewImage &&) = default;

        enum class KtxCompressionFormat {
            uncompressedRgba32nonLinear,
            uncompressedRgba32linear,
            uncompressedRgba16linear,
            uncompressedRgb565linear,
            uncompressedBgr565linear,
            bc1rgbNonLinear,
            bc1rgbLinear,
            bc3rgbaNonLinear,
            bc3rgbaLinear,
            bc4rUnsigned,
            bc4rSigned,
            bc5rgUnsigned,
            bc5rgSigned,
            bc7rgbaNonLinear,
            bc7rgbaLinear,
            bc1rgbIfNoAlphaElseBc3rgbaNonLinear,
            bc1rgbIfNoAlphaElseBc3rgbaLinear
        };
        struct MipLevel {
            std::vector<void *> layers;
            uint32_t width;
            uint32_t height;
            uint32_t depth;
        };

        void loadCompressedKtxFromFileWhole(const std::string &fileName, KtxCompressionFormat compressionFormat);
        void loadCompressedKtxFromFile(const std::string &fileName, KtxCompressionFormat compressionFormat,
                uint32_t baseInputMipLevel, uint32_t baseOutputMipLevel, uint32_t mipLevelCount,
                uint32_t baseInputArrayLayer, uint32_t baseOutputArrayLayer, uint32_t arrayLayerCount);

        void loadUncompressedFromFileWhole(const std::string &fileName);
        void loadUncompressedFromFile(const std::string &fileName, uint32_t baseInputMipLevel, uint32_t baseOutputMipLevel,
                uint32_t mipLevelCount, uint32_t baseInputArrayLayer, uint32_t baseOutputArrayLayer, uint32_t arrayLayerCount);

        void loadRegularRaw2d8bitRgbaFromMemoryWhole(uint32_t width, uint32_t height);
        void loadRegularRaw2d32bitRgbaFromMemoryWhole(uint32_t width, uint32_t height);
        void loadRawFromMemoryWhole(const std::vector<MipLevel> &mipLevels, uint32_t dimensions, VkFormat format);
        void loadRawFromMemory(const std::vector<MipLevel> &mipLevels, uint32_t dimensions, VkFormat format,
                uint32_t baseOutputMipLevel, uint32_t baseOutputArrayLayer);

        void keepRegularRaw2d8bitRgbaEmpty(uint32_t width, uint32_t height);
        void keepRegularRaw2d32bitRgbaEmpty(uint32_t width, uint32_t height);
        void keepEmpty(uint32_t width, uint32_t height, uint32_t depth, uint32_t dimensions, uint32_t mipLevels,
                uint32_t arrayLayers, VkFormat format);
    private:
        std::vector<MipLevel> m_mipLevels;
        void *m_data;
        size_t m_dataSize;

        vulB::VulDevice &m_vulDevice;
};

}
