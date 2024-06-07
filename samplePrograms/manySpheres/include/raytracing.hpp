#pragma once

#include <vulkano_program.hpp>
#include <vul_acceleration_structure.hpp>
#include <vul_rt_pipeline.hpp>

struct RtResources {
    std::unique_ptr<vulB::VulBuffer> spheresBuf;
    std::unique_ptr<vul::VulAs> as;
    std::unique_ptr<vul::VulRtPipeline> pipeline;
    std::array<std::unique_ptr<vul::VulImage>, vulB::VulSwapChain::MAX_FRAMES_IN_FLIGHT> rtImgs;
    std::array<std::unique_ptr<vulB::VulBuffer>, vulB::VulSwapChain::MAX_FRAMES_IN_FLIGHT> ubos;
    std::array<std::shared_ptr<vulB::VulDescriptorSet>, vulB::VulSwapChain::MAX_FRAMES_IN_FLIGHT> descSets;
};
RtResources createRaytracingResources(vul::Vulkano &vulkano);
void raytrace(const vul::Vulkano &vulkano, const RtResources &res, VkCommandBuffer cmdBuf);
void updateRtUbo(const vul::Vulkano &vulkano, RtResources &res, bool fullUpdate);
void resizeRtImgs(const vul::Vulkano &vulkano, RtResources &res);
