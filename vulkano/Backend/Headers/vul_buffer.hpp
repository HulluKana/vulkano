#pragma once

#include "vul_device.hpp"
#include <memory>
#include <stdexcept>
#include <iostream>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace vulB {

class VulBuffer {
    public:
        template<typename T>
        static inline std::unique_ptr<VulBuffer> createLocalBufferFromData(VulDevice &vulDevice, std::vector<T> &data, VkBufferUsageFlags usage, bool enableAddressGetting = false)
        {
            VulBuffer stagingBuffer(vulDevice, sizeof(T), data.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            if (stagingBuffer.map() != VK_SUCCESS) throw std::runtime_error("Failed to map memory to staging buffer in createLocalBufferFromData function");
            stagingBuffer.writeToBuffer(data.data());

            VkMemoryAllocateFlags allocFlags = 0;
            if (enableAddressGetting) allocFlags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
            std::unique_ptr<VulBuffer> resultBuffer = std::make_unique<VulBuffer>(vulDevice, sizeof(T), data.size(), usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, allocFlags);
            vulDevice.copyBuffer(stagingBuffer.getBuffer(), resultBuffer->getBuffer(), sizeof(T) * data.size());

            return resultBuffer;
        }


        VulBuffer(
                VulDevice& device,
                VkDeviceSize instanceSize,
                uint32_t instanceCount,
                VkBufferUsageFlags usageFlags,
                VkMemoryPropertyFlags memoryPropertyFlags,
                VkMemoryAllocateFlags allocateFlags = 0,
                VkDeviceSize minOffsetAlignment = 1);
        ~VulBuffer();

        VulBuffer(const VulBuffer&) = delete;
        VulBuffer& operator=(const VulBuffer&) = delete;


        VkResult map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
        void unmap();

        void writeToBuffer(void* data, VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
        VkResult flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
        VkDescriptorBufferInfo descriptorInfo(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) const;
        VkResult invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);

        void writeToIndex(void* data, int index);
        VkResult flushIndex(int index);
        VkDescriptorBufferInfo descriptorInfoForIndex(int index);
        VkResult invalidateIndex(int index);

        VkBuffer getBuffer() const { return buffer; }
        void* getMappedMemory() const { return mapped; }
        uint32_t getInstanceCount() const { return instanceCount; }
        VkDeviceSize getInstanceSize() const { return instanceSize; }
        VkDeviceSize getAlignmentSize() const { return instanceSize; }
        VkBufferUsageFlags getUsageFlags() const { return usageFlags; }
        VkMemoryPropertyFlags getMemoryPropertyFlags() const { return memoryPropertyFlags; }
        VkDeviceSize getBufferSize() const { return bufferSize; }

    private:
        static VkDeviceSize getAlignment(VkDeviceSize instanceSize, VkDeviceSize minOffsetAlignment);

        VulDevice& vulDevice;
        void* mapped = nullptr;
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;

        VkDeviceSize bufferSize;
        VkDeviceSize instanceSize;
        uint32_t instanceCount;
        VkDeviceSize alignmentSize;
        VkBufferUsageFlags usageFlags;
        VkMemoryPropertyFlags memoryPropertyFlags;
};

}
