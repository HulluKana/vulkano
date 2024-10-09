#include "vul_command_pool.hpp"
#include <vul_debug_tools.hpp>
#include<vul_buffer.hpp>

#include <cstring>
#include <stdexcept>
#include <iostream>
#include <vulkan/vulkan_core.h>
#include <cassert>

namespace vul {

VulBuffer::VulBuffer(uint32_t elementSize, uint32_t elementCount, bool isLocal, VkBufferUsageFlags usage, const VulDevice &vulDevice) : m_vulDevice{vulDevice}
{
    VkResult result = createBuffer(elementSize, elementCount, isLocal, usage);
    assert(result == VK_SUCCESS);
}

VulBuffer::VulBuffer(const VulDevice &vulDevice) : m_vulDevice{vulDevice}
{
}

VulBuffer::~VulBuffer()
{
    unmap();
    vkDestroyBuffer(m_vulDevice.device(), m_buffer, nullptr);
    vkFreeMemory(m_vulDevice.device(), m_memory, nullptr);
}

VkResult VulBuffer::createBuffer(uint32_t elementSize, uint32_t elementCount, bool isLocal, VkBufferUsageFlags usage)
{
    VUL_PROFILE_FUNC()

    if ((usage & (VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)) == (VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT))
        throw std::runtime_error("Buffer can't be both storage and uniform buffer");
    if ((usage & (VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)) == (VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT))
        throw std::runtime_error("Buffer can't be both index and vertex buffer");
    m_usageFlags = usage; 
    m_memoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    if (!isLocal) m_memoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    m_isDeviceLocal = isLocal;
    m_elementSize = elementSize;
    m_elementCount = elementCount;

    VkDeviceSize minOffsetAlignment = 1;
    if (usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT) minOffsetAlignment = m_vulDevice.properties.limits.minUniformBufferOffsetAlignment;
    VkDeviceSize alignment = (m_elementSize + minOffsetAlignment - 1) & ~(minOffsetAlignment - 1);
    m_bufferSize = alignment * m_elementCount;

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = m_bufferSize;
    bufferInfo.usage = m_usageFlags;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (usage & VK_BUFFER_USAGE_VIDEO_DECODE_SRC_BIT_KHR) bufferInfo.flags = VK_BUFFER_CREATE_VIDEO_PROFILE_INDEPENDENT_BIT_KHR;

    VkResult result = vkCreateBuffer(m_vulDevice.device(), &bufferInfo, nullptr, &m_buffer);
    if (result != VK_SUCCESS) return result;

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_vulDevice.device(), m_buffer, &memRequirements);

    VkMemoryAllocateFlagsInfo memAllocFlagsInfo{};
    memAllocFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
    if ((usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) == VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) memAllocFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.pNext = &memAllocFlagsInfo;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = m_vulDevice.findMemoryType(memRequirements.memoryTypeBits, m_memoryPropertyFlags);
    
    result = vkAllocateMemory(m_vulDevice.device(), &allocInfo, nullptr, &m_memory);
    if (result != VK_SUCCESS) return result;
    result = vkBindBufferMemory(m_vulDevice.device(), m_buffer, m_memory, 0);
    if (result != VK_SUCCESS) return result;

    VUL_NAME_VK(m_buffer)
    VUL_NAME_VK(m_memory)

    return VK_SUCCESS;

}

VkResult VulBuffer::writeData(const void *data, VkDeviceSize size, VkDeviceSize offset, VkCommandBuffer commandBuffer)
{
    VUL_PROFILE_FUNC()

    if (data == nullptr) return VK_SUCCESS;

    if (m_buffer == nullptr) throw std::runtime_error("Tried to write to buffer before it was created");
    if (m_isDeviceLocal) {
        VkResult result = addStagingBuffer();
        if (result != VK_SUCCESS) return result;
        result = m_stagingBuffer->writeData(data, size, offset, commandBuffer);
        if (result != VK_SUCCESS) return result;
        copyDataFromBuffer(*m_stagingBuffer, size, offset, offset, commandBuffer);
    }
    else {
        bool needUnmapping = false;
        if (m_mapped == nullptr) {
            needUnmapping = true;
            VkResult result = map(size, offset);
            if (result != VK_SUCCESS) return result;
        }
        VkDeviceSize neededOffset = (m_mapped == nullptr) ? offset : 0;
        memcpy(reinterpret_cast<char *>(m_mapped) + neededOffset, data, size);
        if (needUnmapping) unmap();
    }
    return VK_SUCCESS;
}

VkResult VulBuffer::readData(void *data, VkDeviceSize size, VkDeviceSize offset, VulCmdPool &cmdPool)
{
    if (size + offset > m_bufferSize) throw std::runtime_error("Size + offset of the read data must be at most equal to the size of the buffer");
    if (size == 0) return VK_SUCCESS;

    if (m_isDeviceLocal) {
        VkResult result = addStagingBuffer();
        if (result != VK_SUCCESS) return result;
        VkCommandBuffer cmdBuf = cmdPool.getPrimaryCommandBuffer();
        m_stagingBuffer->copyDataFromBuffer(*this, size, offset, 0, cmdBuf);
        cmdPool.submit(cmdBuf, true);
        memcpy(data, m_stagingBuffer->getMappedMemory(), size);
    } else {
        VkResult result = map(size, offset);
        if (result != VK_SUCCESS) return result;
        memcpy(data, m_mapped, size);
    }
    return VK_SUCCESS;
}

void VulBuffer::copyDataFromBuffer(VulBuffer &srcBuffer, VkDeviceSize size, VkDeviceSize srcOffset, VkDeviceSize dstOffset, VkCommandBuffer cmdBuf)
{
    VUL_PROFILE_FUNC()

    if (m_buffer == nullptr) throw std::runtime_error("Tried to copy to buffer before it was created");
    if (srcBuffer.getBuffer() == nullptr) throw std::runtime_error("Tried to copy from buffer before it was created");
    if (!(m_usageFlags & VK_BUFFER_USAGE_TRANSFER_DST_BIT)) throw std::runtime_error("Copy destination buffer needs transfer dst usage flag");
    if (!(srcBuffer.getUsageFlags() & VK_BUFFER_USAGE_TRANSFER_SRC_BIT)) throw std::runtime_error("Copy source buffer needs transfer src usage flag");
    if (size + srcOffset > srcBuffer.getBufferSize()) throw std::runtime_error("Source buffer is too small for copying");
    if (size + dstOffset > m_bufferSize) throw std::runtime_error("Destination buffer is too small for copying");

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = srcOffset;
    copyRegion.dstOffset = dstOffset;
    copyRegion.size = size;
    vkCmdCopyBuffer(cmdBuf, srcBuffer.getBuffer(), m_buffer, 1, &copyRegion);
}

VkResult VulBuffer::map(VkDeviceSize size, VkDeviceSize offset)
{
    VUL_PROFILE_FUNC()

    if (m_mapped) return VK_SUCCESS;

    if (m_buffer == nullptr) throw std::runtime_error("Tried to map buffer before it was created");
    if (m_isDeviceLocal) throw std::runtime_error("Cannot map device local buffer");
    return vkMapMemory(m_vulDevice.device(), m_memory, offset, size, 0, &m_mapped);
}

void VulBuffer::unmap()
{
    VUL_PROFILE_FUNC()

    if (m_mapped){
        vkUnmapMemory(m_vulDevice.device(), m_memory);
        m_mapped = nullptr;
    }
}

VkResult VulBuffer::resizeBufferWithData(const void *data, uint32_t elementSize, uint32_t elementCount, VkCommandBuffer commandBuffer)
{
    VUL_PROFILE_FUNC()

    unmap();
    vkDestroyBuffer(m_vulDevice.device(), m_buffer, nullptr);
    vkFreeMemory(m_vulDevice.device(), m_memory, nullptr);

    m_elementSize = elementSize;
    m_elementCount = elementCount;

    VkResult result = createBuffer(elementSize, elementCount, m_isDeviceLocal, m_usageFlags);
    if (result != VK_SUCCESS) return result;
    return writeData(data, m_bufferSize, 0, commandBuffer);
}

VkResult VulBuffer::reallocElsewhere(bool isLocal, VkCommandBuffer commandBuffer)
{
    VUL_PROFILE_FUNC()

    VkResult result = addStagingBuffer();
    if (result != VK_SUCCESS) return result;
    copyDataFromBuffer(*this, m_bufferSize, 0, 0, commandBuffer);

    m_isDeviceLocal = isLocal;
    result = resizeBufferWithData(nullptr, m_elementSize, m_elementCount, commandBuffer);
    if (result != VK_SUCCESS) return result;

    copyDataFromBuffer(*m_stagingBuffer.get(), m_bufferSize, 0, 0, commandBuffer);
    return VK_SUCCESS;
}

VkResult VulBuffer::appendData(const void *data, uint32_t elementCount, VulCmdPool &cmdPool)
{
    VUL_PROFILE_FUNC()

    if (elementCount == 0) return VK_SUCCESS;

    std::vector<uint8_t> newData(m_bufferSize + m_elementSize * elementCount);

    if (hasStagingBuffer()) {
        VkResult result = m_stagingBuffer->resizeBufferAsEmpty(m_elementSize, m_elementCount + elementCount);
        if (result != VK_SUCCESS) return result;
        result = m_stagingBuffer->mapAll();
        if (result != VK_SUCCESS) return result;
    }

    VkResult result = readData(newData.data(), m_bufferSize, 0, cmdPool);
    if (result != VK_SUCCESS) return result;
    if (data != nullptr) memcpy(newData.data() + m_bufferSize, data, m_elementSize * elementCount);

    VkCommandBuffer cmdBuf = cmdPool.getPrimaryCommandBuffer();
    result = resizeBufferWithData(newData.data(), m_elementSize, elementCount + m_elementCount, cmdBuf);
    cmdPool.submit(cmdBuf, true);
    return result;
}

VkResult VulBuffer::addStagingBuffer()
{
    VUL_PROFILE_FUNC()

    if (m_stagingBuffer.get()) return VK_SUCCESS;

    m_stagingBuffer = std::make_unique<VulBuffer>(m_vulDevice);
    VkResult result = m_stagingBuffer->createBuffer(m_elementSize, m_elementCount, false, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    if (result != VK_SUCCESS) return result;
    return m_stagingBuffer->mapAll();
}

}
