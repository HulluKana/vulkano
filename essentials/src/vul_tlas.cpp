#include "vul_buffer.hpp"
#include "vul_debug_tools.hpp"
#include "vul_device.hpp"
#include "vul_gltf_loader.hpp"
#include <memory>
#include <stdexcept>
#include <vul_tlas.hpp>
#include <vulkan/vulkan_core.h>
#include <iostream>

using namespace vulB;
namespace vul {

VulAs::VulAs(VulDevice &vulDevice) : m_vulDevice{vulDevice}
{

}

VulAs::~VulAs()
{
    for (As &as : m_blases) vkDestroyAccelerationStructureKHR(m_vulDevice.device(), as.as, nullptr);
}

void VulAs::loadScene(const Scene &scene)
{
    std::vector<BlasInput> blasInputs;
    for (const GltfLoader::GltfPrimMesh &mesh : scene.meshes) {
        blasInputs.emplace_back(gltfNodeToBlasInput(scene, mesh)); 
    }

    buildBlases(blasInputs, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR);
}

void VulAs::buildBlases(const std::vector<BlasInput> &blasInputs, VkBuildAccelerationStructureFlagsKHR flags)
{
    struct BlasBuildData {
        VkAccelerationStructureBuildGeometryInfoKHR buildInfo{};
        VkAccelerationStructureBuildSizesInfoKHR sizeInfo{};
        const VkAccelerationStructureBuildRangeInfoKHR* rangeInfo;
    };
    std::vector<BlasBuildData> buildDatas(blasInputs.size());
    VkDeviceSize maxScratchSize{};
    VkDeviceSize totalBlasesSize{};
    uint32_t compactionsCount{};

    for (size_t i = 0; i < blasInputs.size(); i++) {
        buildDatas[i].buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        buildDatas[i].buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        buildDatas[i].buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        buildDatas[i].buildInfo.flags = blasInputs[i].flags | flags;
        buildDatas[i].buildInfo.geometryCount = static_cast<uint32_t>(blasInputs[i].asGeometries.size());
        buildDatas[i].buildInfo.pGeometries = blasInputs[i].asGeometries.data();

        buildDatas[i].rangeInfo = blasInputs[i].asBuildOffsetInfos.data();
        buildDatas[i].sizeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

        std::vector<uint32_t> maxPrimCounts;
        for (const VkAccelerationStructureBuildRangeInfoKHR &buildOffsetInfo : blasInputs[i].asBuildOffsetInfos)
            maxPrimCounts.push_back(buildOffsetInfo.primitiveCount);
        vkGetAccelerationStructureBuildSizesKHR(m_vulDevice.device(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                &buildDatas[i].buildInfo, maxPrimCounts.data(), &buildDatas[i].sizeInfo);

        maxScratchSize = std::max(maxScratchSize, buildDatas[i].sizeInfo.buildScratchSize);
        totalBlasesSize += buildDatas[i].sizeInfo.accelerationStructureSize;
        if ((buildDatas[i].buildInfo.flags & VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR)
                == VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR) compactionsCount++;
    }

    VulBuffer scratchBuffer(m_vulDevice);
    scratchBuffer.keepEmpty(1, maxScratchSize);
    scratchBuffer.createBuffer(true, static_cast<VulBuffer::Usage>(VulBuffer::usage_ssbo | VulBuffer::usage_getAddress));

    VkQueryPool queryPool = VK_NULL_HANDLE;
    if (compactionsCount > 0) {
        if (compactionsCount != static_cast<uint32_t>(blasInputs.size())) throw std::runtime_error("A mix of compacted and non-compacted blases is not allowed");
        VkQueryPoolCreateInfo quePoCreatI{};
        quePoCreatI.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        quePoCreatI.queryType = VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR;
        quePoCreatI.queryCount = compactionsCount;
        vkCreateQueryPool(m_vulDevice.device(), &quePoCreatI, nullptr, &queryPool);
    }
    
    std::vector<size_t> indices;
    VkDeviceSize batchSize{};
    constexpr VkDeviceSize batchLimit = 256'000'000;
    for (size_t i = 0; i < blasInputs.size(); i++) {
        indices.push_back(i);
        batchSize += buildDatas[i].sizeInfo.accelerationStructureSize;

        if (batchSize >= batchLimit || i == blasInputs.size() - 1) {
            std::cout << "Batch size: " << batchSize << "\n";

            if (queryPool) vkResetQueryPool(m_vulDevice.device(), queryPool, 0, static_cast<uint32_t>(indices.size()));
            uint32_t queryCount{};
            VkCommandBuffer cmdBuf = m_vulDevice.beginSingleTimeCommands();
            for (size_t idx : indices) {
                As blas{};
                blas.buffer = std::make_unique<VulBuffer>(m_vulDevice);
                blas.buffer->keepEmpty(1, buildDatas[idx].sizeInfo.accelerationStructureSize);
                blas.buffer->createBuffer(true, static_cast<VulBuffer::Usage>(VulBuffer::usage_accelerationStructureBuffer | VulBuffer::usage_getAddress));

                VkAccelerationStructureCreateInfoKHR createInfo{};
                createInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
                createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
                createInfo.size = blas.buffer->getBufferSize();
                createInfo.buffer = blas.buffer->getBuffer();

                vkCreateAccelerationStructureKHR(m_vulDevice.device(), &createInfo, nullptr, &blas.as);

                buildDatas[idx].buildInfo.dstAccelerationStructure = blas.as;
                buildDatas[idx].buildInfo.scratchData.deviceAddress = scratchBuffer.getBufferAddress();

                vkCmdBuildAccelerationStructuresKHR(cmdBuf, 1, &buildDatas[idx].buildInfo, &buildDatas[idx].rangeInfo);

                VkMemoryBarrier barrier{};
                barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
                barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
                barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
                vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                        0, 1, &barrier, 0, nullptr, 0, nullptr);

                if (queryPool) vkCmdWriteAccelerationStructuresPropertiesKHR(cmdBuf, 1, &buildDatas[idx].buildInfo.dstAccelerationStructure,
                        VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR, queryPool, queryCount++);

                m_blases.push_back(std::move(blas));

                VUL_NAME_VK_IDX(m_blases[idx].as, idx)
                VUL_NAME_VK_IDX(m_blases[idx].buffer->getBuffer(), idx)
            }
            m_vulDevice.endSingleTimeCommands(cmdBuf);

            if (queryPool) {
                std::vector<VkDeviceSize> compactSizes(indices.size());
                vkGetQueryPoolResults(m_vulDevice.device(), queryPool, 0, static_cast<uint32_t>(compactSizes.size()), compactSizes.size() * sizeof(VkDeviceSize),
                        compactSizes.data(), sizeof(VkDeviceSize), VK_QUERY_RESULT_WAIT_BIT);
                std::vector<As> bigAses;
                uint32_t queryCnt{};
                VkCommandBuffer cmdBuf = m_vulDevice.beginSingleTimeCommands();

                VkDeviceSize compactBatchSize{};
                for (VkDeviceSize compactSize : compactSizes) compactBatchSize += compactSize;
                std::cout << "Compact batch size: " << compactBatchSize << "\n";
                for (size_t idx : indices) {
                    As blas{};
                    blas.buffer = std::make_unique<VulBuffer>(m_vulDevice);
                    blas.buffer->keepEmpty(1, compactSizes[queryCnt++]);
                    blas.buffer->createBuffer(true, static_cast<VulBuffer::Usage>(VulBuffer::usage_accelerationStructureBuffer | VulBuffer::usage_getAddress));

                    VkAccelerationStructureCreateInfoKHR assCreate{};
                    assCreate.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
                    assCreate.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
                    assCreate.size = blas.buffer->getBufferSize();
                    assCreate.buffer = blas.buffer->getBuffer();

                    vkCreateAccelerationStructureKHR(m_vulDevice.device(), &assCreate, nullptr, &blas.as);

                    // I am sane
                    VkCopyAccelerationStructureInfoKHR cloneI{};
                    cloneI.sType = VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_INFO_KHR;
                    cloneI.src = m_blases[idx].as;
                    cloneI.dst = blas.as;
                    cloneI.mode = VK_COPY_ACCELERATION_STRUCTURE_MODE_COMPACT_KHR;
                    vkCmdCopyAccelerationStructureKHR(cmdBuf, &cloneI);

                    bigAses.push_back(std::move(m_blases[idx]));
                    m_blases[idx] = std::move(blas);
 
                    VUL_NAME_VK_IDX(m_blases[idx].as, idx)
                    VUL_NAME_VK_IDX(m_blases[idx].buffer->getBuffer(), idx)
                }
                m_vulDevice.endSingleTimeCommands(cmdBuf);
                for (As &as : bigAses) vkDestroyAccelerationStructureKHR(m_vulDevice.device(), as.as, nullptr);
            }

            batchSize = 0;
            indices.clear();
        }
    }

    if (queryPool) vkDestroyQueryPool(m_vulDevice.device(), queryPool, nullptr);
}

VulAs::BlasInput VulAs::gltfNodeToBlasInput(const Scene &scene, const GltfLoader::GltfPrimMesh &mesh)
{
    VkAccelerationStructureGeometryTrianglesDataKHR triangles{};
    triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
    triangles.vertexData.deviceAddress = scene.vertexBuffer->getBufferAddress();
    triangles.vertexStride = sizeof(glm::vec3);
    triangles.maxVertex = mesh.vertexCount - 1;
    triangles.indexType = VK_INDEX_TYPE_UINT32;
    triangles.indexData.deviceAddress = scene.indexBuffer->getBufferAddress();

    VkAccelerationStructureGeometryKHR asGeom{};
    asGeom.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    asGeom.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    asGeom.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
    asGeom.geometry.triangles = triangles;

    VkAccelerationStructureBuildRangeInfoKHR offsetInfo{};
    offsetInfo.firstVertex = mesh.vertexOffset;
    offsetInfo.primitiveCount = mesh.indexCount / 3;
    offsetInfo.primitiveOffset = mesh.firstIndex * sizeof(uint32_t);
    offsetInfo.transformOffset = 0;

    BlasInput blasInput{};
    blasInput.asGeometries.push_back(asGeom);
    blasInput.asBuildOffsetInfos.push_back(offsetInfo);

    return blasInput;
}

}
