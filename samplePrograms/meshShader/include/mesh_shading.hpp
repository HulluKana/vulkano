#pragma once

#include "host_device.hpp"
#include <vul_buffer.hpp>
#include <vul_descriptors.hpp>
#include <vul_swap_chain.hpp>
#include <vul_mesh_pipeline.hpp>
#include <vul_renderer.hpp>
#include <vul_camera.hpp>
#include <vul_scene.hpp>
#include <array>
#include <vulkan/vulkan_core.h>

struct MeshResources {
    std::unique_ptr<vul::VulMeshPipeline> pipeline;
    std::array<std::unique_ptr<vul::VulDescriptorSet>, vul::VulSwapChain::MAX_FRAMES_IN_FLIGHT> descSets;
    std::array<std::unique_ptr<vul::VulBuffer>, vul::VulSwapChain::MAX_FRAMES_IN_FLIGHT> ubos;
    std::unique_ptr<vul::VulBuffer> meshletVerticesBuffer;
    std::unique_ptr<vul::VulBuffer> meshletTrianglesBuffer;
    std::unique_ptr<vul::VulBuffer> meshletBuffer;
    std::unique_ptr<vul::VulBuffer> meshletBoundsBuffer;
    std::unique_ptr<vul::VulBuffer> meshInfoBuffer;
    std::unique_ptr<vul::VulBuffer> drawMeshTasksIndirectBuffer;
};
MeshResources createMeshShadingResources(vul::Scene &scene, const vul::VulRenderer &vulRenderer, const vul::VulDescriptorPool &descPool, VkCommandBuffer cmdBuf, const vul::VulDevice &vulDevice);
void updateMeshUbo(const MeshResources &res, const vul::Scene &scene, const vul::VulCamera &camera, const vul::VulRenderer &vulRenderer, const glm::vec4 &ambientLightColor);
void meshShade(const MeshResources &res, const vul::Scene &scene, const vul::VulRenderer &vulRenderer, const glm::vec4 &ambientLightColor, VkCommandBuffer cmdBuf);
