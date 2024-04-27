#include "vul_buffer.hpp"
#include "vul_debug_tools.hpp"
#include "vul_device.hpp"
#include "vul_gltf_loader.hpp"
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

}

void VulAs::loadScene(const Scene &scene)
{
    std::vector<BlasInput> blasInputs;
    for (const GltfLoader::GltfNode &node : scene.nodes) {
        blasInputs.emplace_back(gltfNodeToBlasInput(scene, node)); 
    }

    buildBlases(blasInputs, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);
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

    for (size_t i = 0; i < blasInputs.size(); i++) {
        buildDatas[i].buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        buildDatas[i].buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        buildDatas[i].buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        buildDatas[i].buildInfo.flags = blasInputs[i].flags | flags;
        buildDatas[i].buildInfo.geometryCount = static_cast<uint32_t>(blasInputs[i].asGeometries.size());
        buildDatas[i].buildInfo.pGeometries = blasInputs[i].asGeometries.data();

        buildDatas[i].rangeInfo = blasInputs[i].asBuildOffsetInfos.data();
        buildDatas[i].sizeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

        std::vector<uint32_t> maxPrimCounts(blasInputs[i].asBuildOffsetInfos.size());
        for (const VkAccelerationStructureBuildRangeInfoKHR &buildOffsetInfo : blasInputs[i].asBuildOffsetInfos)
            maxPrimCounts.push_back(buildOffsetInfo.primitiveCount);
        vkGetAccelerationStructureBuildSizesKHR(m_vulDevice.device(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                &buildDatas[i].buildInfo, maxPrimCounts.data(), &buildDatas[i].sizeInfo);

        maxScratchSize = std::max(maxScratchSize, buildDatas[i].sizeInfo.buildScratchSize);
        totalBlasesSize += buildDatas[i].sizeInfo.accelerationStructureSize;
    }

    VulBuffer scratchBuffer(m_vulDevice);
    scratchBuffer.keepEmpty(1, maxScratchSize);
    scratchBuffer.createBuffer(true, static_cast<VulBuffer::Usage>(VulBuffer::usage_ssbo | VulBuffer::usage_getAddress));
    
    std::vector<size_t> indices;
    VkDeviceSize batchSize{};
    constexpr VkDeviceSize batchLimit = 256'000'000;
    for (size_t i = 0; i < buildDatas.size(); i++) {
        indices.push_back(i);
        batchSize += buildDatas[i].sizeInfo.accelerationStructureSize;

        if (batchSize >= batchLimit || i == buildDatas.size() - 1) {
            std::cout << "Batch size: " << batchSize << "\n";

            VkCommandBuffer cmdBuf = m_vulDevice.beginSingleTimeCommands();
            for (size_t idx : indices) {
                As blas{};
                blas.buffer = std::make_shared<VulBuffer>(m_vulDevice);
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

                m_blases.push_back(blas);

                VUL_NAME_VK_IDX(m_blases[idx].as, idx)
                VUL_NAME_VK_IDX(m_blases[idx].buffer->getBuffer(), idx)
            }
            m_vulDevice.endSingleTimeCommands(cmdBuf);
            batchSize = 0;
            indices.clear();
        }
    }
}

VulAs::BlasInput VulAs::gltfNodeToBlasInput(const Scene &scene, const GltfLoader::GltfNode &node)
{
    const GltfLoader::GltfPrimMesh &mesh = scene.meshes[node.primMesh];

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
