#pragma once

#include"vul_device.hpp"

#include <memory>
#include <vulkan/vulkan_core.h>

namespace vulB {

class VulBuffer {
    public:
        VulBuffer(VulDevice &vulDevice);
        ~VulBuffer();

        VulBuffer(const VulBuffer &) = delete;
        VulBuffer &operator=(const VulBuffer &) = delete;
        VulBuffer(VulBuffer &&) = default;
        VulBuffer &operator=(VulBuffer &&) = default;

        template<typename T>
        void loadVector(const std::vector<T> &vector) {loadData(vector.data(), sizeof(T), static_cast<uint32_t>(vector.size()));}
        void loadData(const void *data, uint32_t elementSize, uint32_t elementCount);
        void keepEmpty(uint32_t elementSize, uint32_t elementCount);

        enum Usage : VkBufferUsageFlags {
            usage_none = 0,
            usage_transferSrc = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            usage_transferDst = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            usage_ssbo = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            usage_ubo = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            usage_indexBuffer = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            usage_vertexBuffer = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
        };
        VkResult createBuffer(bool isLocal, Usage usage);

        template<typename T>
        VkResult writeVector(const std::vector<T> &vector, VkDeviceSize offset) {return writeData(vector.data(), sizeof(T) * vector.size(), sizeof(T) * offset);}
        VkResult writeData(const void *data, VkDeviceSize size, VkDeviceSize offset);

        void copyDataFromBufferSingleTime(VulBuffer &srcBuffer, VkDeviceSize size, VkDeviceSize srcOffset, VkDeviceSize dstOffset);
        void copyDataFromBuffer(VulBuffer &srcBuffer, VkDeviceSize size, VkDeviceSize srcOffset, VkDeviceSize dstOffset, VkCommandBuffer cmdBuf);

        VkResult mapAll() {return map(m_bufferSize, 0);}
        VkResult map(VkDeviceSize size, VkDeviceSize offset);
        void unmap();

        VkResult addStagingBuffer();
        void deleteStagingBuffer() {delete m_stagingBuffer.release();}

        VkBuffer getBuffer() const { return m_buffer; }
        bool hasStagingBuffer() const {return m_stagingBuffer.get() != nullptr;}
        void* getMappedMemory() const { return m_mapped; }
        VkBufferUsageFlags getUsageFlags() const { return m_usageFlags; }
        VkMemoryPropertyFlags getMemoryPropertyFlags() const { return m_memoryPropertyFlags; }
        VkDeviceSize getBufferSize() const { return m_bufferSize; }
        VkDescriptorBufferInfo getDescriptorInfo() const {return VkDescriptorBufferInfo{m_buffer, 0, m_bufferSize};}
    private:
        VulDevice &m_vulDevice; 

        const void *m_inputData = nullptr;
        uint32_t m_elementSize = 0;
        uint32_t m_elementCount = 0;
        bool m_creationPreparationDone = false;

        void* m_mapped = nullptr;
        VkBuffer m_buffer = VK_NULL_HANDLE;
        VkDeviceMemory m_memory = VK_NULL_HANDLE;
        std::unique_ptr<VulBuffer> m_stagingBuffer = nullptr;

        VkDeviceSize m_bufferSize;
        VkBufferUsageFlags m_usageFlags;
        VkMemoryPropertyFlags m_memoryPropertyFlags;
};
}
