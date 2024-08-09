#pragma once

#include "vul_pipeline.hpp"
#include <vul_acceleration_structure.hpp>
#include <vul_rt_pipeline.hpp>
#include <vul_swap_chain.hpp>
#include <vul_renderer.hpp>
#include <vul_camera.hpp>
#include <vul_descriptors.hpp>

struct RtResources {
    std::unique_ptr<vul::VulBuffer> spheresBuf;
    std::unique_ptr<vul::VulAs> as;
    std::unique_ptr<vul::VulRtPipeline> rtPipeline;
    std::unique_ptr<vul::VulPipeline> fullScreenQuadPipeline;
    std::array<std::unique_ptr<vul::VulImage>, vul::VulSwapChain::MAX_FRAMES_IN_FLIGHT> rtImgs;
    std::array<std::unique_ptr<vul::VulBuffer>, vul::VulSwapChain::MAX_FRAMES_IN_FLIGHT> ubos;
    std::array<std::shared_ptr<vul::VulDescriptorSet>, vul::VulSwapChain::MAX_FRAMES_IN_FLIGHT> descSets;
    std::unique_ptr<vul::Scene> fullScreenQuad;
};
RtResources createRaytracingResources(const vul::VulRenderer &vulRenderer, const vul::VulDescriptorPool &descPool, vul::VulCmdPool &cmdPool, const vul::VulDevice &vulDevice);
void raytrace(const RtResources &res, const vul::VulRenderer &vulRenderer, VkCommandBuffer cmdBuf);
void updateRtUbo(RtResources &res, const vul::VulRenderer &vulRenderer, const vul::VulCamera &camera);
void resizeRtImgs(RtResources &res, const vul::VulRenderer &vulRenderer, vul::VulCmdPool &cmdPool, const vul::VulDevice &vulDevice);
