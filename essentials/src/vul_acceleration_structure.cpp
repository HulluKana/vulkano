#include <cstdio>
#include <stdexcept>
#include <vul_debug_tools.hpp>
#include <vul_transform.hpp>
#include <vul_acceleration_structure.hpp>
#include <iostream>
#include <vulkan/vulkan_core.h>

using namespace vulB;
namespace vul {

VulAs::VulAs(VulDevice &vulDevice, const Scene &scene) : m_vulDevice{vulDevice}
{
    m_transformsBuffer = createTransformsBuffer(scene);

    std::vector<BlasInput> blasInputs;
    blasInputs.emplace_back(gltfNodesToBlasInput(scene, 0, static_cast<uint32_t>(scene.nodes.size()), m_transformsBuffer));

    buildBlases(blasInputs, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR);

    std::vector<VkAccelerationStructureInstanceKHR> asInsts;
    for (size_t i = 0; i < m_blases.size(); i++) asInsts.emplace_back(blasToAsInstance(i, m_blases[i], 0xff));

    buildTlas(asInsts, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);
}

VulAs::~VulAs()
{
    vkDestroyAccelerationStructureKHR(m_vulDevice.device(), m_tlas.as, nullptr);
    for (As &as : m_blases) vkDestroyAccelerationStructureKHR(m_vulDevice.device(), as.as, nullptr);
}

void VulAs::buildTlas(const std::vector<VkAccelerationStructureInstanceKHR> &asInsts, VkBuildAccelerationStructureFlagsKHR flags)
{
    VulBuffer instsBuf(m_vulDevice); 
    instsBuf.loadVector(asInsts);
    instsBuf.createBuffer(true, static_cast<VulBuffer::Usage>(VulBuffer::usage_getAddress | VulBuffer::usage_accelerationStructureBuildRead
                | VulBuffer::usage_transferDst));

    VkCommandBuffer cmdBuf = m_vulDevice.beginSingleTimeCommands();
    VkMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
    vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1, &barrier, 0, nullptr, 0, nullptr);

    VkAccelerationStructureGeometryInstancesDataKHR instsData{};
    instsData.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    instsData.data.deviceAddress = instsBuf.getBufferAddress();

    VkAccelerationStructureGeometryKHR tlasGeom{};
    tlasGeom.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    tlasGeom.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    tlasGeom.geometry.instances = instsData;

    VkAccelerationStructureBuildGeometryInfoKHR buildInfo{};
    buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    buildInfo.flags = flags;
    buildInfo.geometryCount = 1;
    buildInfo.pGeometries = &tlasGeom;

    VkAccelerationStructureBuildSizesInfoKHR sizeInfo{};
    sizeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

    uint32_t primitiveCount = static_cast<uint32_t>(asInsts.size());
    vkGetAccelerationStructureBuildSizesKHR(m_vulDevice.device(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo, &primitiveCount, &sizeInfo);

    VkAccelerationStructureCreateInfoKHR creatInf{};
    creatInf.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    creatInf.size = sizeInfo.accelerationStructureSize;
    m_tlas = createAs(creatInf);

    VulBuffer scratchBuffer(m_vulDevice);
    scratchBuffer.keepEmpty(1, sizeInfo.buildScratchSize);
    scratchBuffer.createBuffer(true, static_cast<VulBuffer::Usage>(VulBuffer::usage_ssbo | VulBuffer::usage_getAddress));

    buildInfo.dstAccelerationStructure = m_tlas.as;
    buildInfo.scratchData.deviceAddress = scratchBuffer.getBufferAddress();

    VkAccelerationStructureBuildRangeInfoKHR buildRangeInf{};
    buildRangeInf.primitiveCount = primitiveCount;
    buildRangeInf.primitiveOffset = 0;
    buildRangeInf.firstVertex = 0;
    buildRangeInf.transformOffset = 0;
    const VkAccelerationStructureBuildRangeInfoKHR *pBuildRangeInf = &buildRangeInf;

    vkCmdBuildAccelerationStructuresKHR(cmdBuf, 1, &buildInfo, &pBuildRangeInf);
    m_vulDevice.endSingleTimeCommands(cmdBuf);
}

void VulAs::buildBlases(const std::vector<BlasInput> &blasInputs, VkBuildAccelerationStructureFlagsKHR flags)
{
    std::vector<BlasBuildData> buildDatas(blasInputs.size());
    VkDeviceSize maxScratchSize{};
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
            if (queryPool) vkResetQueryPool(m_vulDevice.device(), queryPool, 0, static_cast<uint32_t>(indices.size()));
            uint32_t queryCount{};
            VkCommandBuffer cmdBuf = m_vulDevice.beginSingleTimeCommands();
            for (size_t idx : indices) {
                VkAccelerationStructureCreateInfoKHR createInfo{};
                createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
                createInfo.size = buildDatas[idx].sizeInfo.accelerationStructureSize;
                As blas = createAs(createInfo);

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

                for (size_t idx : indices) {
                    VkAccelerationStructureCreateInfoKHR assCreate{};
                    assCreate.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
                    assCreate.size = compactSizes[queryCnt++];
                    As blas = createAs(assCreate);

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

VkAccelerationStructureInstanceKHR VulAs::blasToAsInstance(uint32_t index, const As &blas, uint32_t mask)
{
    transform3D defaultTransform{};
    defaultTransform.pos = glm::vec3(0.0f);
    defaultTransform.rot = glm::vec3(0.0f);
    defaultTransform.scale = glm::vec3(1.0f);
    glm::mat4 trasposedTransformMat = glm::transpose(defaultTransform.transformMat());

    VkAccelerationStructureDeviceAddressInfoKHR blasAddrInfo{};
    blasAddrInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
    blasAddrInfo.accelerationStructure = blas.as;

    VkAccelerationStructureInstanceKHR asInst{};
    memcpy(&asInst.transform, &trasposedTransformMat, sizeof(VkTransformMatrixKHR));
    asInst.instanceCustomIndex = index;
    asInst.accelerationStructureReference = vkGetAccelerationStructureDeviceAddressKHR(m_vulDevice.device(), &blasAddrInfo);
    asInst.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
    asInst.mask = mask;
    asInst.instanceShaderBindingTableRecordOffset = 0;
    
    return asInst;
}

VulAs::BlasInput VulAs::gltfNodesToBlasInput(const Scene &scene, uint32_t firstNode, uint32_t nodeCount, const std::unique_ptr<vulB::VulBuffer> &transformsBuffer)
{
    BlasInput blasInput{};
    for (uint32_t i = firstNode; i < firstNode + nodeCount; i++) {
        const GltfLoader::GltfPrimMesh &mesh = scene.meshes[scene.nodes[i].primMesh];

        VkAccelerationStructureGeometryTrianglesDataKHR triangles{};
        triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
        triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
        triangles.vertexData.deviceAddress = scene.vertexBuffer->getBufferAddress();
        triangles.vertexStride = sizeof(glm::vec3);
        triangles.maxVertex = mesh.vertexCount - 1;
        triangles.indexType = VK_INDEX_TYPE_UINT32;
        triangles.indexData.deviceAddress = scene.indexBuffer->getBufferAddress();
        triangles.transformData.deviceAddress = transformsBuffer->getBufferAddress();

        VkAccelerationStructureGeometryKHR asGeom{};
        asGeom.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
        asGeom.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
        if (scene.materials[mesh.materialIndex].alphaMode == GltfLoader::GltfAlphaMode::opaque) asGeom.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
        else asGeom.flags = VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR;
        asGeom.geometry.triangles = triangles;

        VkAccelerationStructureBuildRangeInfoKHR offsetInfo{};
        offsetInfo.firstVertex = mesh.vertexOffset;
        offsetInfo.primitiveCount = mesh.indexCount / 3;
        offsetInfo.primitiveOffset = mesh.firstIndex * sizeof(uint32_t);
        offsetInfo.transformOffset = i * sizeof(VkTransformMatrixKHR);

        blasInput.asGeometries.push_back(asGeom);
        blasInput.asBuildOffsetInfos.push_back(offsetInfo);
    }

    return blasInput;
}

std::unique_ptr<VulBuffer> VulAs::createTransformsBuffer(const Scene &scene)
{
    std::vector<VkTransformMatrixKHR> transforms(scene.nodes.size());
    for (size_t i = 0; i < scene.nodes.size(); i++) {
        glm::mat4 trasposedTransformMat = glm::transpose(scene.nodes[i].worldMatrix);
        memcpy(&transforms[i], &trasposedTransformMat, sizeof(VkTransformMatrixKHR));
    }

    std::unique_ptr<vulB::VulBuffer> transformsBuffer = std::make_unique<VulBuffer>(m_vulDevice);
    transformsBuffer->loadVector(transforms);
    transformsBuffer->createBuffer(true, static_cast<VulBuffer::Usage>(VulBuffer::usage_ssbo | VulBuffer::usage_transferDst | VulBuffer::usage_getAddress | VulBuffer::usage_accelerationStructureBuildRead));
    return transformsBuffer;
}

VulAs::As VulAs::createAs(VkAccelerationStructureCreateInfoKHR &createInfo)
{
    As as{};
    as.buffer = std::make_unique<VulBuffer>(m_vulDevice);
    as.buffer->keepEmpty(1, createInfo.size);
    VkResult result = as.buffer->createBuffer(true, static_cast<VulBuffer::Usage>(VulBuffer::usage_accelerationStructureBuffer | VulBuffer::usage_getAddress));
    if (result != VK_SUCCESS) {
        if (result == VK_ERROR_OUT_OF_DEVICE_MEMORY) {
            fprintf(stderr, "Failed to create acceleration structure buffer due to running out of device memory. Tried to allocate %lu bytes. Trying to allocate buffer to host memory", createInfo.size);
            as.buffer = std::make_unique<VulBuffer>(m_vulDevice);
            as.buffer->keepEmpty(1, createInfo.size);
            result = as.buffer->createBuffer(false, static_cast<VulBuffer::Usage>(VulBuffer::usage_accelerationStructureBuffer | VulBuffer::usage_getAddress));
        }
        if (result != VK_SUCCESS) throw std::runtime_error("Failed to create acceleration structure buffer: " + std::to_string(result));
    } 

    createInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    createInfo.buffer = as.buffer->getBuffer();

    vkCreateAccelerationStructureKHR(m_vulDevice.device(), &createInfo, nullptr, &as.as);
    return as;
}

}
