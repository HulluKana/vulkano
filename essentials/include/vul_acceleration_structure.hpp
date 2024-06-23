#include "vul_gltf_loader.hpp"
#include <iterator>
#include <vul_device.hpp>
#include <vul_scene.hpp>
#include <vulkan/vulkan_core.h>

#pragma once

namespace vul {

class VulAs {
    public:
        VulAs(const vulB::VulDevice &vulDevice);
        ~VulAs();

        VulAs(const VulAs &) = delete;
        VulAs &operator=(const VulAs &) = delete;
        VulAs(VulAs &&) = default;

        struct Aabb {
            glm::vec3 minPos;
            glm::vec3 maxPos;
            uint32_t blasIndex;
        };
        struct AsNode {
            uint32_t nodeIndex;
            uint32_t blasIndex;
        };
        struct InstanceInfo {
            uint32_t blasIdx;
            uint32_t customIndex;
            uint32_t shaderBindingTableRecordOffset;
            glm::mat4 transform;
        };
        void loadScene(const Scene &scene, const std::vector<AsNode> &nodes, const std::vector<InstanceInfo> &instanceInfos, bool allowUpdating);
        void loadAabbs(const std::vector<Aabb> &aabbs, const std::vector<InstanceInfo> &instanceInfos, bool allowUpdating);

        struct InstanceTransform {
            uint32_t instanceIdx;
            glm::mat4 transform;
        };
        void updateInstanceTransforms(const std::vector<InstanceTransform> &instanceTransforms);

        const VkAccelerationStructureKHR *getPTlas() const {return &m_tlas.as;}
    private:
        struct As {
            VkAccelerationStructureKHR as;
            std::unique_ptr<vulB::VulBuffer> buffer;
        };
        struct BlasInput {
            std::vector<VkAccelerationStructureGeometryKHR> asGeometries;
            std::vector<VkAccelerationStructureBuildRangeInfoKHR> asBuildOffsetInfos;
            VkBuildAccelerationStructureFlagsKHR flags{};
        };
        struct BlasBuildData {
            VkAccelerationStructureBuildGeometryInfoKHR buildInfo{};
            VkAccelerationStructureBuildSizesInfoKHR sizeInfo{};
            const VkAccelerationStructureBuildRangeInfoKHR* rangeInfo;
        };

        void buildTlas(const std::vector<VkAccelerationStructureInstanceKHR> &asInsts, VkBuildAccelerationStructureFlagsKHR flags, bool update);

        void buildBlases(const std::vector<BlasInput> &blasInputs, VkBuildAccelerationStructureFlagsKHR flags);
        As buildBlas(BlasBuildData &buildData, VkDeviceAddress scratchBufferAddress, VkQueryPool queryPool, uint32_t queryIndex, VkCommandBuffer cmdBuf);

        VkAccelerationStructureInstanceKHR blasToAsInstance(uint32_t index, uint32_t sbtOffset, const glm::mat4 &transform, const As &blas);
        BlasInput gltfNodesToBlasInput(const Scene &scene, const std::vector<uint32_t> &orderedNodesIndices, uint32_t startIdx, uint32_t count, const std::unique_ptr<vulB::VulBuffer> &transformsBuffer);
        BlasInput aabbsToBlasInput(vulB::VulBuffer &aabbBuf, VkDeviceSize maxAabbCount, VkDeviceSize aabbOffset);
        std::unique_ptr<vulB::VulBuffer> createTransformsBuffer(const Scene &scene, const std::vector<uint32_t> &orderedNodesIndices);

        As createAs(VkAccelerationStructureCreateInfoKHR &createInfo, bool deviceLocal);

        As m_tlas;
        std::vector<As> m_blases;

        VkBuildAccelerationStructureFlagsKHR m_tlasBuildFlags;

        std::vector<VkAccelerationStructureInstanceKHR> m_instances;

        const vulB::VulDevice &m_vulDevice;
};

}
