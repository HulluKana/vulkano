#pragma once

#include"vul_device.hpp"

#include <memory>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

namespace vul {

class VulBuffer {
    public:
        VulBuffer(uint32_t elementSize, uint32_t elementCount, bool isLocal, VkBufferUsageFlags usage, const VulDevice &vulDevice);
        VulBuffer(const VulDevice &vulDevice);
        ~VulBuffer();

        VulBuffer(const VulBuffer &) = delete;
        VulBuffer &operator=(const VulBuffer &) = delete;
        VulBuffer(VulBuffer &&) = default;

        VkResult createBuffer(uint32_t elementSize, uint32_t elementCount, bool isLocal, VkBufferUsageFlags usage);

        VkResult writeData(const void *data, VkDeviceSize size, VkDeviceSize offset, VkCommandBuffer cmdBuf);
        template<typename T> VkResult writeVector(const std::vector<T> &vector, VkDeviceSize offset, VkCommandBuffer cmdBuf) {return writeData(vector.data(), sizeof(T) * vector.size(), sizeof(T) * offset, cmdBuf);}
        
        VkResult readData(void *data, VkDeviceSize size, VkDeviceSize offset, VkCommandBuffer cmdBuf);
        template<typename T> VkResult readVector(std::vector<T> &vector, size_t elementCount, VkDeviceSize offset, VkCommandBuffer cmdBuf)
        {
            if (sizeof(T) != m_elementSize) throw std::runtime_error("Size of the element in the read destination vector must be the same as the size of the element in the buffer");
            size_t oldSizeInBytes = vector.size();
            vector.resize(vector.size() + elementCount);
            return readData(&vector[oldSizeInBytes], elementCount * sizeof(T), offset, cmdBuf);
        }
        template<typename T> VkResult readVectorAll(std::vector<T> &vector) {return readVector(vector, m_elementCount, 0);}

        void copyDataFromBuffer(VulBuffer &srcBuffer, VkDeviceSize size, VkDeviceSize srcOffset, VkDeviceSize dstOffset, VkCommandBuffer cmdBuf);

        VkResult mapAll() {return map(m_bufferSize, 0);}
        VkResult map(VkDeviceSize size, VkDeviceSize offset);
        void unmap();

        VkResult resizeBufferWithData(const void *data, uint32_t elementSize, uint32_t elementCount, VkCommandBuffer commandBuffer);
        template<typename T> VkResult resizeBufferWithVector(const std::vector<T> &vector, VkCommandBuffer commandBuffer) {return resizeBufferWithData(vector.data(), sizeof(T), static_cast<uint32_t>(vector.size()), commandBuffer);}
        VkResult resizeBufferAsEmpty(uint32_t elementSize, uint32_t elementCount) {return resizeBufferWithData(nullptr, elementSize, elementCount, VK_NULL_HANDLE);}

        VkResult reallocElsewhere(bool isLocal, VkCommandBuffer commandBuffer);

        VkResult appendData(const void *data, uint32_t elementCount, VkCommandBuffer commandBuffer);
        template<typename T> VkResult appendVector(const std::vector<T> &vector, VkCommandBuffer commandBuffer) {return appendData(vector.data(), static_cast<uint32_t>(vector.size()), commandBuffer);}
        VkResult appendEmpty(uint32_t elementCount) {return appendData(nullptr, elementCount, VK_NULL_HANDLE);}

        VkResult addStagingBuffer();
        void deleteStagingBuffer() {m_stagingBuffer.reset(nullptr);}

        VkBuffer getBuffer() const { return m_buffer; }
        VkDeviceMemory getMemory() const {return m_memory; }
        bool hasStagingBuffer() const {return m_stagingBuffer.get() != nullptr;}
        const std::unique_ptr<VulBuffer> &getStagingBuffer() const {return m_stagingBuffer;}
        void* getMappedMemory() const { return m_mapped; }
        VkBufferUsageFlags getUsageFlags() const { return m_usageFlags; }
        VkMemoryPropertyFlags getMemoryPropertyFlags() const { return m_memoryPropertyFlags; }
        VkDeviceSize getBufferSize() const { return m_bufferSize; }

        VkDescriptorBufferInfo getDescriptorInfo() const {return VkDescriptorBufferInfo{m_buffer, 0, m_bufferSize};}
        VkDeviceAddress getBufferAddress() const
        {
            VkBufferDeviceAddressInfo addressInfo{VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, nullptr, m_buffer};
            return vkGetBufferDeviceAddress(m_vulDevice.device(), &addressInfo);
        }
    private:
        const VulDevice &m_vulDevice; 

        uint32_t m_elementSize = 0;
        uint32_t m_elementCount = 0;

        void* m_mapped = nullptr;
        VkBuffer m_buffer = VK_NULL_HANDLE;
        VkDeviceMemory m_memory = VK_NULL_HANDLE;
        std::unique_ptr<VulBuffer> m_stagingBuffer = nullptr;

        VkDeviceSize m_bufferSize;
        VkBufferUsageFlags m_usageFlags;
        VkMemoryPropertyFlags m_memoryPropertyFlags;
        bool m_isDeviceLocal;
};
}
