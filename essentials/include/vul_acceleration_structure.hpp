#include <map>
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
        void loadScene(const Scene &scene);
        void loadAabbs(const std::vector<Aabb> &aabbs, const std::vector<uint32_t> &customBlasIndices, bool allowUpdating);

        struct BlasTransform {
            uint32_t blasIdx;
            glm::mat4 transform;
        };
        void updateBlasTransforms(const std::vector<BlasTransform> &blasTransforms);

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

        VkAccelerationStructureInstanceKHR blasToAsInstance(uint32_t index, const As &blas);
        BlasInput gltfNodesToBlasInput(const Scene &scene, uint32_t firstNode, uint32_t nodeCount, const std::unique_ptr<vulB::VulBuffer> &transformsBuffer);
        BlasInput aabbsToBlasInput(vulB::VulBuffer &aabbBuf, VkDeviceSize maxAabbCount, VkDeviceSize aabbOffset);
        std::unique_ptr<vulB::VulBuffer> createTransformsBuffer(const Scene &scene);

        As createAs(VkAccelerationStructureCreateInfoKHR &createInfo);

        As m_tlas;
        std::vector<As> m_blases;
        std::unique_ptr<vulB::VulBuffer> m_transformsBuffer;

        VkBuildAccelerationStructureFlagsKHR m_tlasBuildFlags;

        std::map<uint32_t, VkAccelerationStructureInstanceKHR> m_instancesMap;

        const vulB::VulDevice &m_vulDevice;
};

}
