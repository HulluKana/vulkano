#pragma once

#include "vul_device.hpp"
#include <ktx.h>
#include <variant>
#include <vul_buffer.hpp>
#include <vulkan/vulkan_core.h>

/*
REQUIRED FEATURES
Y   Load raw data from memory
YX  Load uncompressed from regular file
YX  Load compressed from ktx

Y   Create mipmaps from several uncompressed files
YX  Create mipmaps from compressed ktx file
Y   Create mipmaps from memory
Y   Create mipmaps from several memory
Y   Create mipmaps from base image at runtime for raw images

Y   Support 1D, 2D, 3D and cubemap images
YX  Support 1, 2 and 4 channels for raw images 
Y   Support multiple layers for all except 3D
Y   Support mipmaps for all sampled variants, including their layered variants

Y   Support sampled format for all image types that vulkan supports sampled for 
Y   Support storage format for all image types that vulkan supports storage for 
    Support all bc compression formats except bc2 for sampled images
YX  Support 8bit unorm and srgb for sampled images
Y   Support 16bit sfloat for sampled images
YX  Support 32bit sfloat and uint for storage images

YX  Create device local images
YX  Create host visible and coherent images

YX  Modify device local images
YX  Modify host visible images
Y   Modify individual mipmap levels
Y   Modify individual array layers, including those in mipmaps

Y   Read data from device local images
Y   Read data from host visible images
Y   Read data from individual mipmap levels
Y   Read data from individual array layers, including those in mipmaps

YX  Allow creating few different good types of images using higher level and easier to use function
YX  Allow creating all types of supported images using lower level and harder to use function
Y   Allow creating few different types good types of images using a single higher level all-in-ones static function
Y   Allow passing own command buffer to both low and high level functions
YX  Allow not having to pass own command buffer to both low and high level functions
YX  Allow passing image data at the creation time
Y   Allow not passing image data at the creation time
Y   Allow creating a good high quality texture sampler with higher level and easier to use function
Y   Allow creating a custom texture sampler with lover level and easier to use function
Y   Allow passing in a pre-existing sampler

OPTIONAL FEATURES
    Load uncompressed from memory
    Load compressed from memory
    Load uncompressed from ktx

Y   Create mipmaps from several compressed files

    Support 3 channels for raw images

    Support bc2 compression format for sampled images
Y   Support all 8, 16 and 32 bits for sampled images
Y   Support all 8, 16 and 32 bits for storage images
Y   Support single channel 64bit uint and sint for storage images

YX  Support sharing sampler
    Support sharing image source

Y   Create host visible and host cached, but not host coherent images
Y   Create host visible, host cached and host coherent images

    Allow creating all types of supported images using higher level and easier to use function
Y   Allow creating all types of supported images using a single static lower-level all-in-one function
    Allow creating all types of supported images using a single static higher level all-in-one function

    Allow using image as attachment image

THE FOLLOWING MUST BE SPECIFIED BY DATA LOADER
Mip level count
Width, height and depth of each mip level
Pointer to the start of each mip levels data, or nullptr to signal lack of data
Data format
Size of each texel
Total data size
Pointer to start of the data, or nullptr to signal lack of data
*/

namespace vul {

class VulSampler {
    public:
        ~VulSampler();

        VulSampler(const VulSampler &) = delete;
        VulSampler &operator=(const VulSampler &) = delete;
        VulSampler(VulSampler &&) = default;

        static std::shared_ptr<VulSampler> createDefaultTexSampler(vulB::VulDevice &vulDevice, uint32_t mipLevels);
        static std::shared_ptr<VulSampler> createCustomSampler(vulB::VulDevice &vulDevice, VkFilter filter,
                VkSamplerAddressMode addressMode, float maxAnisotropy, VkBorderColor borderColor,
                VkSamplerMipmapMode mipMapMode,float mipLodBias, float mipMinLod, float mipMaxLod);

        VkSampler getSampler() const {return m_sampler;}
    private:
        VulSampler(vulB::VulDevice &vulDevice, VkFilter filter, VkSamplerAddressMode addressMode, float maxAnisotropy,
                VkBorderColor borderColor, VkSamplerMipmapMode mipMapMode, float mipLodBias, float mipMinLod, float mipMaxLod);
        vulB::VulDevice &m_vulDevice;
        VkSampler m_sampler = VK_NULL_HANDLE;
};



class VulImage {
    public:
        VulImage(vulB::VulDevice &vulDevice);
        ~VulImage();

        VulImage(const VulImage &) = delete;
        VulImage &operator=(const VulImage &) = delete;
        VulImage(VulImage &&) = default;

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
        };
        enum class ImageType {
            texture1d, 
            texture2d,
            texture3d,
            textureCube,
            storage1d,
            storage2d,
            storage3d,
            storageCube
        };
        enum class InputDataType {
            compressedKtxFile,
            uncompressedNonKtxFile,
            rawData
        };
        struct DataSection {
            void *data;
            size_t dataSize;
            uint32_t startingX;
            uint32_t startingY;
            uint32_t startingZ;
            uint32_t mipLevel;
            uint32_t arrayLayer;
        };
        struct RawImageData {
            uint32_t baseWidth;
            uint32_t baseHeight;
            uint32_t baseDepth;
            std::vector<std::vector<void *>> data;
        };

        static VulImage createDefaultWholeImageAllInOneSingleTime(vulB::VulDevice &vulDevice, std::variant<const std::string &,
                const RawImageData &> data, std::variant<KtxCompressionFormat, VkFormat> format, bool addSampler,
                InputDataType dataType, ImageType imageType);
        static VulImage createDefaultWholeImageAllInOne(vulB::VulDevice &vulDevice, std::variant<const std::string &,
                const RawImageData &> data, std::variant<KtxCompressionFormat, VkFormat> format, bool addSampler,
                InputDataType dataType, ImageType imageType, VkCommandBuffer cmdBuf);
        static VulImage createSamplerlessCustomImageAllInOneSingleTime(vulB::VulDevice &vulDevice,
                std::variant<const std::string &, const RawImageData &> data, std::variant<KtxCompressionFormat,
                VkFormat> format, InputDataType dataType, VkImageViewType type, VkImageLayout layout, VkImageUsageFlags usage,
                VkMemoryPropertyFlags memoryProperties, VkImageTiling tiling);
        static VulImage createSamplerlessCustomImageAllInOne(vulB::VulDevice &vulDevice, std::variant<const std::string &,
                const RawImageData &> data, std::variant<KtxCompressionFormat, VkFormat> format, InputDataType dataType,
                VkImageViewType type, VkImageLayout layout, VkImageUsageFlags usage, VkMemoryPropertyFlags memoryProperties,
                VkImageTiling tiling, VkCommandBuffer cmdBuf);
        static VulImage createCustomImageAllInOneSingleTime(vulB::VulDevice &vulDevice,
                std::variant<const std::string &, const RawImageData &> data, std::variant<KtxCompressionFormat,
                VkFormat> format, InputDataType dataType, VkImageViewType type, VkImageLayout layout, VkImageUsageFlags usage,
                VkMemoryPropertyFlags memoryProperties, VkImageTiling tiling, VkFilter filter, VkSamplerAddressMode addressMode,
                float maxAnisotropy, VkBorderColor borderColor, float mipLodBias, float mipMinLod, float mipMaxLod);
        static VulImage createCustomImageAllInOne(vulB::VulDevice &vulDevice, std::variant<const std::string &,
                const RawImageData &> data, std::variant<KtxCompressionFormat, VkFormat> format, InputDataType dataType,
                VkImageViewType type, VkImageLayout layout, VkImageUsageFlags usage, VkMemoryPropertyFlags memoryProperties,
                VkImageTiling tiling, VkFilter filter, VkSamplerAddressMode addressMode, float maxAnisotropy,
                VkBorderColor borderColor, float mipLodBias, float mipMinLod, float mipMaxLod, VkCommandBuffer cmdBuf);

        void loadCompressedKtxFromFileWhole(const std::string &fileName, KtxCompressionFormat compressionFormat);
        void loadCompressedKtxFromFile(const std::string &fileName, KtxCompressionFormat compressionFormat,
                uint32_t baseInputMipLevel, uint32_t baseOutputMipLevel, uint32_t mipLevelCount,
                uint32_t baseInputArrayLayer, uint32_t baseOutputArrayLayer, uint32_t arrayLayerCount);

        void loadUncompressedFromFileWhole(const std::string &fileName);
        void loadUncompressedFromFile(const std::string &fileName, uint32_t baseInputMipLevel, uint32_t baseOutputMipLevel,
                uint32_t mipLevelCount, uint32_t baseInputArrayLayer, uint32_t baseOutputArrayLayer, uint32_t arrayLayerCount);

        void loadRegularRaw2d8bitRgbaFromMemoryWhole(const void *data, uint32_t width, uint32_t height);
        void loadRegularRaw2d32bitRgbaFromMemoryWhole(const void *data, uint32_t width, uint32_t height);
        void loadRawFromMemoryWhole(uint32_t baseWidth, uint32_t baseHeight, uint32_t baseDepth,
                const std::vector<std::vector<void *>> &data, VkFormat format);
        void loadRawFromMemory(uint32_t baseWidth, uint32_t baseHeight, uint32_t baseDepth, const std::vector<std::vector<void *>>
                &data, VkFormat format, uint32_t baseOutputMipLevel, uint32_t baseOutputArrayLayer);

        void keepRegularRaw2d8bitRgbaEmpty(uint32_t width, uint32_t height);
        void keepRegularRaw2d32bitRgbaEmpty(uint32_t width, uint32_t height);
        void keepEmpty(uint32_t baseWidth, uint32_t baseHeight, uint32_t baseDepth, uint32_t mipCount, uint32_t arrayCount,
                VkFormat format, uint32_t baseOutputMipLevel, uint32_t baseOutputArrayLayer);

        void createMipMapsSingleTime();
        void createMipMaps(VkCommandBuffer cmdBuf);

        void createDefaultImageSingleTime(ImageType type);
        void createDefaultImage(ImageType type, VkCommandBuffer cmdBuf);
        void createCustomImageSingleTime(VkImageViewType type, VkImageLayout layout, VkImageUsageFlags usage,
                VkMemoryPropertyFlags memoryProperties, VkImageTiling tiling);
        void createCustomImage(VkImageViewType type, VkImageLayout layout, VkImageUsageFlags usage,
                VkMemoryPropertyFlags memoryProperties, VkImageTiling tiling, VkCommandBuffer cmdBuf);

        void modifyImageSingleTime(const std::vector<DataSection> &modificationSections);
        void modifyImage(const std::vector<DataSection> &modificationSections, VkCommandBuffer cmdBuf);

        void readImageSingleTime(std::vector<DataSection> &readSections);
        void readImage(std::vector<DataSection> &readSections, VkCommandBuffer cmdBuf);

        void deleteStagingResources() {m_stagingBuffers.clear();}
        void deleteCpuData() {delete m_data.release(); m_dataSize = 0;}

        uint32_t getBaseWidth() const {return m_baseWidth;}
        uint32_t getBaseHeight() const {return m_baseHeight;}
        uint32_t getBaseDepth() const {return m_baseDepth;}
        uint32_t getMipCount() const {return static_cast<uint32_t>(m_mipLevels.size());}
        VkExtent3D getMipSize(uint32_t mipLevel) const;
        uint32_t getArrayCount() const {return m_arrayLayers;}
        uint32_t getBitsPerTexel() const {return m_bitsPerTexel;}
        size_t getDataSize() const {return m_dataSize;}

        VkFormat getFormat() const {return m_format;}
        VkMemoryPropertyFlags getMemoryProperties() const {return m_memoryProperties;}
        VkImageUsageFlags getImageUsages() const {return m_usage;}
        VkImageLayout getLayout() const {return m_layout;}
        VkImageTiling getTiling() const {return m_tiling;}
        VkImageType getImageType() const {return m_imageType;}
        VkImageViewType getImageViewType() const {return m_imageViewType;}

        VkImage getImage() const {return m_image;}
        VkImageView getImageView() const {return m_imageView;}
        VkDeviceMemory getMemory() const {return m_imageMemory;}
        
        std::shared_ptr<VulSampler> vulSampler = nullptr;
    private:
        struct KtxCompressionFormatProperties {
            ktx_transcode_fmt_e transcodeFormat;
            VkFormat vkFormat;
        };
        struct VkFormatProperties {
            uint32_t bitsPerTexel;
            uint32_t sideLengthAlignment;
        };
        struct MipLevel {
            std::vector<void *> layers;
            uint32_t width;
            uint32_t height;
            uint32_t depth;
            size_t layerSize;
        };

        uint32_t alignUp(uint32_t alignee, uint32_t aligner);
        void transitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout, VkCommandBuffer cmdBuf);
        void copyBufferToImage(VkBuffer buffer, uint32_t mipLevel, VkCommandBuffer cmdBuf);
        KtxCompressionFormatProperties getKtxCompressionFormatProperties(KtxCompressionFormat compressionFormat);
        VkFormatProperties getVkFormatProperties(VkFormat format);

        VkFormat m_format = VK_FORMAT_UNDEFINED;
        uint32_t m_bitsPerTexel = 0;
        uint32_t m_arrayLayers = 0;
        VkMemoryPropertyFlags m_memoryProperties = 0;
        VkImageUsageFlags m_usage = 0;
        VkImageLayout m_layout = VK_IMAGE_LAYOUT_UNDEFINED;
        VkImageTiling m_tiling = VK_IMAGE_TILING_OPTIMAL;
        VkImageType m_imageType = VK_IMAGE_TYPE_1D;
        VkImageViewType m_imageViewType = VK_IMAGE_VIEW_TYPE_1D;

        std::vector<MipLevel> m_mipLevels;
        std::unique_ptr<uint8_t> m_data;
        size_t m_dataSize = 0;
        uint32_t m_baseWidth = 0;
        uint32_t m_baseHeight = 0;
        uint32_t m_baseDepth = 0;
        std::vector<std::unique_ptr<vulB::VulBuffer>> m_stagingBuffers;

        VkImage m_image = VK_NULL_HANDLE;
        VkImageView m_imageView = VK_NULL_HANDLE;
        VkDeviceMemory m_imageMemory = VK_NULL_HANDLE;

        vulB::VulDevice &m_vulDevice;
};

}
