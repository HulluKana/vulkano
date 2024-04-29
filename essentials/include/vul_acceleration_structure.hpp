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
        VkAccelerationStructureInstanceKHR gltfNodeToAsInstance(const Scene &scene, uint32_t nodeIdx, const As &blas);

        void buildBlases(const std::vector<BlasInput> &blasInputs, VkBuildAccelerationStructureFlagsKHR flags);
        As buildBlas(BlasBuildData &buildData, VkDeviceAddress scratchBufferAddress, VkQueryPool queryPool, uint32_t queryIndex, VkCommandBuffer cmdBuf);
        BlasInput gltfMeshToBlasInput(const Scene &scene, const vulB::GltfLoader::GltfPrimMesh &mesh);

        As createAs(VkAccelerationStructureCreateInfoKHR &createInfo);

        As m_tlas;
        std::vector<As> m_blases;

        vulB::VulDevice &m_vulDevice;
};

}
