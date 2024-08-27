#pragma once

#include "host_device.hpp"
#include "vul_pipeline.hpp"
#include <vul_buffer.hpp>
#include <vul_descriptors.hpp>
#include <vul_swap_chain.hpp>
#include <vul_mesh_pipeline.hpp>
#include <vul_renderer.hpp>
#include <vul_camera.hpp>
#include <vul_scene.hpp>
#include <vul_meshlet_scene.hpp>
#include <array>
#include <vulkan/vulkan_core.h>

struct MeshResources {
    std::unique_ptr<vul::VulMeshPipeline> pipeline;
    std::unique_ptr<vul::VulPipeline> cubeMapPipeline;
    std::array<std::unique_ptr<vul::VulDescriptorSet>, vul::VulSwapChain::MAX_FRAMES_IN_FLIGHT> descSets;
    std::array<std::unique_ptr<vul::VulDescriptorSet>, vul::VulSwapChain::MAX_FRAMES_IN_FLIGHT> cubeMapDescSets;
    std::array<std::unique_ptr<vul::VulBuffer>, vul::VulSwapChain::MAX_FRAMES_IN_FLIGHT> ubos;
};
MeshResources createMeshShadingResources(const vul::VulMeshletScene &scene, const vul::VulImage &cubeMap, const vul::VulRenderer &vulRenderer, const vul::VulDescriptorPool &descPool, VkCommandBuffer cmdBuf, const vul::VulDevice &vulDevice);
void updateMeshUbo(const MeshResources &res, const vul::VulMeshletScene &scene, const vul::VulCamera &camera, const vul::VulRenderer &vulRenderer, const glm::vec4 &ambientLightColor);
void meshShade(const MeshResources &res, const vul::VulMeshletScene &scene, const vul::Scene &cubeMapScene, vul::VulCamera &camera, const vul::VulRenderer &vulRenderer, const glm::vec4 &ambientLightColor, VkCommandBuffer cmdBuf);
