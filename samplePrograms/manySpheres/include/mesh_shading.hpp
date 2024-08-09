#pragma once

#include <vul_buffer.hpp>
#include <vul_descriptors.hpp>
#include <vul_swap_chain.hpp>
#include <vul_mesh_pipeline.hpp>
#include <vul_renderer.hpp>
#include <vul_camera.hpp>
#include <array>
#include <vulkan/vulkan_core.h>

struct MeshResources {
    std::unique_ptr<vul::VulMeshPipeline> renderPipeline;
    std::array<std::unique_ptr<vul::VulDescriptorSet>, vul::VulSwapChain::MAX_FRAMES_IN_FLIGHT> renderDescSets;
    std::unique_ptr<vul::VulBuffer> cubeBuf;
    std::unique_ptr<vul::VulBuffer> chunksBuf;
    std::array<std::unique_ptr<vul::VulBuffer>, vul::VulSwapChain::MAX_FRAMES_IN_FLIGHT> ubos;
};
MeshResources createMeshShadingResources(const vul::VulRenderer &vulRenderer, const vul::VulDescriptorPool &descPool, VkCommandBuffer cmdBuf, const vul::VulDevice &vulDevice);
void updateMeshUbo(const MeshResources &res, const vul::VulCamera &camera, const vul::VulRenderer &vulRenderer);
void meshShade(const MeshResources &res, const vul::VulRenderer &vulRenderer, VkCommandBuffer cmdBuf);
