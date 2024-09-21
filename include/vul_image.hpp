#pragma once

#include "vul_device.hpp"
#include <cassert>
#include <ktx.h>
#include <variant>
#include <vul_buffer.hpp>
#include <vulkan/vulkan_core.h>

namespace vul {

class VulSampler {
    public:
        ~VulSampler();

        VulSampler(const VulSampler &) = delete;
        VulSampler &operator=(const VulSampler &) = delete;
        VulSampler(VulSampler &&) = default;

        static std::shared_ptr<VulSampler> createDefaultTexSampler(const VulDevice &vulDevice);
        static std::shared_ptr<VulSampler> createCustomSampler(const VulDevice &vulDevice, VkFilter filter,
                VkSamplerAddressMode addressMode, float maxAnisotropy, VkBorderColor borderColor,
                VkSamplerMipmapMode mipMapMode, bool enableSamplerReduction, VkSamplerReductionMode samplerReductionMode,
                float mipLodBias, float mipMinLod, float mipMaxLod);

        VkSampler getSampler() const {return m_sampler;}
    private:
        VulSampler(const VulDevice &vulDevice, VkFilter filter, VkSamplerAddressMode addressMode, float maxAnisotropy,
                VkBorderColor borderColor, VkSamplerMipmapMode mipMapMode, bool enableSamplerReduction,
                VkSamplerReductionMode samplerReductionMode, float mipLodBias, float mipMinLod, float mipMaxLod);
        const VulDevice &m_vulDevice;
        VkSampler m_sampler = VK_NULL_HANDLE;
};



class VulImage {
    public:
        VulImage(const VulDevice &vulDevice);
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
            storageCube,
            hdrCube,
            colorAttachment,
            depthAttachment
        };
        enum class InputDataType {
            compressedKtxFile,
            exrFile,
            normieFile,
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
        struct OldVkImageStuff {
            VkImage image = VK_NULL_HANDLE;
            VkDeviceMemory imageMemory = VK_NULL_HANDLE;
            VkImageView imageView = VK_NULL_HANDLE;
            std::vector<VkImageView> mipImageViews;
            VkDevice device = VK_NULL_HANDLE;

            void destoyImageStuff();
            ~OldVkImageStuff() {destoyImageStuff();}
        };

        static std::unique_ptr<VulImage> createDefaultWholeImageAllInOne(const VulDevice &vulDevice, std::variant<std::string,
                RawImageData> data, std::variant<KtxCompressionFormat, VkFormat> format, bool addSampler,
                InputDataType dataType, ImageType imageType, VkCommandBuffer cmdBuf);
        static std::unique_ptr<VulImage> createSamplerlessCustomImageAllInOne(const VulDevice &vulDevice, std::variant<const std::string &,
                const RawImageData &> data, std::variant<KtxCompressionFormat, VkFormat> format, InputDataType dataType,
                VkImageViewType type, VkImageLayout layout, VkImageUsageFlags usage, VkMemoryPropertyFlags memoryProperties,
                VkImageTiling tiling, VkCommandBuffer cmdBuf);
        static std::unique_ptr<VulImage> createCustomImageAllInOne(const VulDevice &vulDevice, std::variant<const std::string &,
                const RawImageData &> data, std::variant<KtxCompressionFormat, VkFormat> format, InputDataType dataType,
                VkImageViewType type, VkImageLayout layout, VkImageUsageFlags usage, VkMemoryPropertyFlags memoryProperties,
                VkImageTiling tiling, VkFilter filter, VkSamplerAddressMode addressMode, float maxAnisotropy,
                VkBorderColor borderColor, float mipLodBias, float mipMinLod, float mipMaxLod, VkCommandBuffer cmdBuf);

        void loadCompressedKtxFromFileWhole(const std::string &fileName, KtxCompressionFormat compressionFormat);
        void loadCompressedKtxFromFile(const std::string &fileName, KtxCompressionFormat compressionFormat,
                uint32_t inputMipLevel, uint32_t mipLevelCount);
        void addMipLevelsToStartFromCompressedKtxFile(const std::string &fileName,
                KtxCompressionFormat compressionFormat, uint32_t mipLevelCount);

        void loadUncompressedFromFile(const std::string &fileName);
        void loadCubemapFromEXR(const std::string &filename);

        void loadRegularRaw2d8bitRgbaFromMemory(const void *data, uint32_t width, uint32_t height);
        void loadRegularRaw2d32bitRgbaFromMemory(const void *data, uint32_t width, uint32_t height);
        void loadRawFromMemory(uint32_t baseWidth, uint32_t baseHeight, uint32_t baseDepth,
                const std::vector<std::vector<void *>> &data, VkFormat format);

        void keepRegularRaw2d8bitRgbaEmpty(uint32_t width, uint32_t height);
        void keepRegularRaw2d32bitRgbaEmpty(uint32_t width, uint32_t height);
        void keepEmpty(uint32_t baseWidth, uint32_t baseHeight, uint32_t baseDepth, uint32_t mipCount, uint32_t arrayCount, VkFormat format);

        std::unique_ptr<OldVkImageStuff> createDefaultImage(ImageType type, VkCommandBuffer cmdBuf);
        std::unique_ptr<OldVkImageStuff> createCustomImage(VkImageViewType type, VkImageLayout layout, VkImageUsageFlags usage,
                VkMemoryPropertyFlags memoryProperties, VkImageTiling tiling, VkImageAspectFlags aspect, VkCommandBuffer cmdBuf);
        std::unique_ptr<OldVkImageStuff> createCustomImageSparse(VkImageViewType type, VkImageLayout layout, VkImageUsageFlags usage,
                VkMemoryPropertyFlags memoryProperties, VkImageAspectFlags aspect, VkCommandBuffer cmdBuf);

        void createFromVkImage(VkImage image, VkImageViewType type, VkFormat format, VkImageAspectFlags aspect,
                uint32_t mipLevelCount, uint32_t arrayLayerCount);

        void createMipMaps(VkCommandBuffer cmdBuf);
        void createImageViewsForMipMaps();

        void modifyImage(const std::vector<DataSection> &modificationSections, VkCommandBuffer cmdBuf);
        void readImage(std::vector<DataSection> &readSections, VkCommandBuffer cmdBuf);

        void transitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout, VkCommandBuffer cmdBuf);
        void transitionQueueFamily(uint32_t srcFamilyIdx, uint32_t dstFamilyIdx, VkPipelineStageFlags accessMask, VkCommandBuffer cmdBuf);

        void deleteStagingResources() {m_stagingBuffer.reset(nullptr);}
        void deleteCpuData() {m_data.resize(0);}

        VkRenderingAttachmentInfo getAttachmentInfo(VkClearValue clearValue) const;
        VkDescriptorImageInfo getDescriptorInfo() const;
        VkDescriptorImageInfo getMipDescriptorInfo(uint32_t mipLevel) const;

        uint32_t getBaseWidth() const {return m_baseWidth;}
        uint32_t getBaseHeight() const {return m_baseHeight;}
        uint32_t getBaseDepth() const {return m_baseDepth;}
        uint32_t getMipCount() const {return m_mipLevels.size();}
        VkExtent3D getMipSize(uint32_t mip)const{return{m_mipLevels[mip].width,m_mipLevels[mip].height,m_mipLevels[mip].depth};}
        uint32_t getArrayCount() const {return m_arrayLayersCount;}
        uint32_t getBitsPerTexel() const {return m_bitsPerTexel;}
        size_t getDataSize() const {return m_data.size();}

        VkFormat getFormat() const {return m_format;}
        VkMemoryPropertyFlags getMemoryProperties() const {return m_memoryProperties;}
        VkImageUsageFlags getImageUsages() const {return m_usage;}
        VkImageLayout getLayout() const {return m_layout;}
        VkImageTiling getTiling() const {return m_tiling;}
        VkImageAspectFlags getAspect() const {return m_aspect;}
        VkImageType getImageType() const {return m_imageType;}
        VkImageViewType getImageViewType() const {return m_imageViewType;}

        VkImage getImage() const {return m_image;}
        VkImageView getImageView() const {return m_imageView;}
        VkImageView getImageViewForMipLevel(uint32_t mipLevel) const {assert(m_mipImageViews.size() > mipLevel); return m_mipImageViews[mipLevel];}
        VkDeviceMemory getMemory() const {return m_imageMemory;}
        
        std::shared_ptr<VulSampler> vulSampler = nullptr;
        bool attachmentPreservePreviousContents = false;
        bool attachmentStoreCurrentContents = true;
        std::string name;
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
            std::vector<size_t> layers;
            std::vector<bool> containsData;
            uint32_t width;
            uint32_t height;
            uint32_t depth;
            size_t layerSize;
        };

        std::unique_ptr<OldVkImageStuff> prepareImageCreation(VkImageViewType type, VkImageUsageFlags usage,
                VkMemoryPropertyFlags memoryProperties, VkImageTiling tiling, VkImageAspectFlags aspect);
        void createVkImage(VkImageCreateFlags flags);
        void allocateVkMemory();
        VkImageView createImageView(uint32_t baseMipLevel, uint32_t mipLevelCount);
        void copyBufferToImage(VkBuffer buffer, VkDeviceSize offset, uint32_t mipLevel, VkCommandBuffer cmdBuf);

        uint32_t alignUp(uint32_t alignee, uint32_t aligner);

        KtxCompressionFormatProperties getKtxCompressionFormatProperties(KtxCompressionFormat compressionFormat);
        VkFormatProperties getVkFormatProperties(VkFormat format);

        bool m_ownsImage = false;
        VkFormat m_format = VK_FORMAT_UNDEFINED;
        uint32_t m_bitsPerTexel = 0;
        uint32_t m_arrayLayersCount = 0;
        VkMemoryPropertyFlags m_memoryProperties = 0;
        VkImageUsageFlags m_usage = 0;
        VkImageLayout m_layout = VK_IMAGE_LAYOUT_UNDEFINED;
        VkImageTiling m_tiling = VK_IMAGE_TILING_OPTIMAL;
        VkImageAspectFlags m_aspect = 0;
        VkImageType m_imageType = VK_IMAGE_TYPE_1D;
        VkImageViewType m_imageViewType = VK_IMAGE_VIEW_TYPE_1D;

        std::vector<MipLevel> m_mipLevels;
        std::vector<uint8_t> m_data;
        uint32_t m_baseWidth = 0;
        uint32_t m_baseHeight = 0;
        uint32_t m_baseDepth = 0;
        std::unique_ptr<VulBuffer> m_stagingBuffer;

        VkImage m_image = VK_NULL_HANDLE;
        VkImageView m_imageView = VK_NULL_HANDLE;
        std::vector<VkImageView> m_mipImageViews;
        VkDeviceMemory m_imageMemory = VK_NULL_HANDLE;
        std::vector<VkDeviceMemory> m_sparseMemoryRegions;

        const VulDevice &m_vulDevice;
};

}
