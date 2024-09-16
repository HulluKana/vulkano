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
    std::unique_ptr<vul::VulMeshPipeline> shadowPipeline;
    std::unique_ptr<vul::VulPipeline> cubeMapPipeline;
    std::array<std::unique_ptr<vul::VulDescriptorSet>, vul::VulSwapChain::MAX_FRAMES_IN_FLIGHT> descSets;
    std::array<std::unique_ptr<vul::VulDescriptorSet>, vul::VulSwapChain::MAX_FRAMES_IN_FLIGHT> shadowDescSets;
    std::array<std::unique_ptr<vul::VulDescriptorSet>, vul::VulSwapChain::MAX_FRAMES_IN_FLIGHT> cubeMapDescSets;
    std::array<std::unique_ptr<vul::VulBuffer>, vul::VulSwapChain::MAX_FRAMES_IN_FLIGHT> ubos;
    std::array<std::unique_ptr<vul::VulBuffer>, vul::VulSwapChain::MAX_FRAMES_IN_FLIGHT> shadowUbos;
};
MeshResources createMeshShadingResources(const vul::VulMeshletScene &scene, vul::VulImage &cubeMap, vul::VulImage &shadowMapPoint,
        vul::VulImage &shadowMapDir, vul::VulRenderer &vulRenderer, const vul::VulDescriptorPool &descPool, const vul::VulDevice &vulDevice);
void renderShadowMaps(vul::VulRenderer &vulRenderer, vul::VulImage &shadowMapPoint, vul::VulImage &shadowMapDir, const vul::VulMeshletScene &scene, const MeshResources &meshResources);
void updateMeshUbo(const MeshResources &res, const vul::VulMeshletScene &scene, const vul::VulCamera &camera, const vul::VulRenderer &vulRenderer, const glm::vec4 &ambientLightColor);
void meshShade(const MeshResources &res, const vul::VulMeshletScene &scene, const vul::Scene &cubeMapScene,
        const vul::VulCamera &camera, const vul::VulRenderer &vulRenderer, const glm::vec4 &ambientLightColor, VkCommandBuffer cmdBuf);
