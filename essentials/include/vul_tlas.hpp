#include "vul_buffer.hpp"
#include "vul_gltf_loader.hpp"
#include <vul_device.hpp>
#include <vul_scene.hpp>
#include <vulkan/vulkan_core.h>

#pragma once

namespace vul {

class VulAs {
    public:
        VulAs(vulB::VulDevice &vulDevice);
        ~VulAs();

        VulAs(const VulAs &) = delete;
        VulAs &operator=(const VulAs &) = delete;
        // Move instead of reference, you know the deal
        VulAs(VulAs &&) = default;
        VulAs &operator=(VulAs &&) = default;

        void loadScene(const Scene &scene);
    private:
        struct BlasInput {
            std::vector<VkAccelerationStructureGeometryKHR> asGeometries;
            std::vector<VkAccelerationStructureBuildRangeInfoKHR> asBuildOffsetInfos;
            VkBuildAccelerationStructureFlagsKHR flags{};
        };
        
        void buildBlases(const std::vector<BlasInput> &blasInputs, VkBuildAccelerationStructureFlagsKHR flags);
        BlasInput gltfNodeToBlasInput(const Scene &scene, const vulB::GltfLoader::GltfNode &node);

        struct As {
            VkAccelerationStructureKHR as;
            std::shared_ptr<vulB::VulBuffer> buffer;
        };
        std::vector<As> m_blases;

        vulB::VulDevice &m_vulDevice;
};

}
