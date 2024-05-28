#include "vul_buffer.hpp"
#include "vul_gltf_loader.hpp"
#include <vul_device.hpp>
#include <vul_scene.hpp>
#include <vulkan/vulkan_core.h>

#pragma once

namespace vul {

class VulAs {
    public:
        VulAs(vulB::VulDevice &vulDevice, const Scene &scene);
        ~VulAs();

        VulAs(const VulAs &) = delete;
        VulAs &operator=(const VulAs &) = delete;
        VulAs(VulAs &&) = default;

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

        void buildTlas(const std::vector<VkAccelerationStructureInstanceKHR> &asInsts, VkBuildAccelerationStructureFlagsKHR flags);

        void buildBlases(const std::vector<BlasInput> &blasInputs, VkBuildAccelerationStructureFlagsKHR flags);
        As buildBlas(BlasBuildData &buildData, VkDeviceAddress scratchBufferAddress, VkQueryPool queryPool, uint32_t queryIndex, VkCommandBuffer cmdBuf);

        VkAccelerationStructureInstanceKHR blasToAsInstance(uint32_t index, const As &blas, uint32_t mask);
        BlasInput gltfNodesToBlasInput(const Scene &scene, uint32_t firstNode, uint32_t nodeCount, const std::unique_ptr<vulB::VulBuffer> &transformsBuffer);
        std::unique_ptr<vulB::VulBuffer> createTransformsBuffer(const Scene &scene);

        As createAs(VkAccelerationStructureCreateInfoKHR &createInfo);

        As m_tlas;
        std::vector<As> m_blases;
        std::unique_ptr<vulB::VulBuffer> m_transformsBuffer;

        vulB::VulDevice &m_vulDevice;
};

}
