#pragma once

#include <vul_buffer.hpp>
#include <vul_swap_chain.hpp>
#include <vul_descriptors.hpp>
#include <vul_scene.hpp>
#include <vul_pipeline.hpp>
#include <vul_renderer.hpp>
#include <vul_camera.hpp>
#include <array>

struct RasResources {
    std::shared_ptr<vul::VulBuffer> objDataBuf;
    std::array<std::unique_ptr<vul::VulBuffer>, vul::VulSwapChain::MAX_FRAMES_IN_FLIGHT> ubos;
    std::array<std::unique_ptr<vul::VulDescriptorSet>, vul::VulSwapChain::MAX_FRAMES_IN_FLIGHT> descSets;
    std::unique_ptr<vul::Scene> scene;
    std::unique_ptr<vul::VulPipeline> pipeline;
    vul::VulPipeline::DrawData drawData;
};
RasResources createRasterizationResources(const vul::VulRenderer &vulRenderer, const vul::VulDescriptorPool &descPool, vul::VulCmdPool &cmdPool, const vul::VulDevice &vulDevice);
void updateRasUbo(const vul::VulCamera &camera, const vul::VulRenderer &vulRenderer, RasResources &res);
void rasterize(const RasResources &res, const vul::VulRenderer &vulRenderer, VkCommandBuffer cmdBuf);
