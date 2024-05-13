#pragma once

#include <vulkano_program.hpp>
#include <vul_acceleration_structure.hpp>
#include <host_device.hpp>

struct ReservoirGrid {
    std::unique_ptr<vulB::VulBuffer> buffer;
    std::vector<Reservoir> reservoirs;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    int minX;
    int minY;
    int minZ;
};

ReservoirGrid createReservoirGrid(const vul::Scene &scene, const vulB::VulDevice &device);
std::unique_ptr<vulB::VulDescriptorSet> createRtDescSet(const vul::Vulkano &vulkano, const vul::VulAs &as,
        const std::unique_ptr<vul::VulImage> &rtImg, const std::unique_ptr<vulB::VulBuffer> &ubo,
        const std::unique_ptr<vul::VulImage> &enviromentMap, const std::unique_ptr<vulB::VulBuffer> &reservoirsBuffer);
vul::Vulkano::RenderData createRenderData(const vul::Vulkano &vulkano, const vulB::VulDevice &device,
        const std::array<std::shared_ptr<vulB::VulDescriptorSet>, vulB::VulSwapChain::MAX_FRAMES_IN_FLIGHT> &descSets);
void resizeRtImgs(vul::Vulkano &vulkano, std::array<std::unique_ptr<vul::VulImage>, vulB::VulSwapChain::MAX_FRAMES_IN_FLIGHT> &rtImgs);
void updateUbo(const vul::Vulkano &vulkano, std::unique_ptr<vulB::VulBuffer> &ubo);
