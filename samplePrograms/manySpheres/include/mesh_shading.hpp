#pragma once

#include "vul_buffer.hpp"
#include "vul_comp_pipeline.hpp"
#include "vul_descriptors.hpp"
#include "vul_swap_chain.hpp"
#include <vulkano_program.hpp>
#include <vul_mesh_pipeline.hpp>

struct MeshResources {
    std::unique_ptr<vul::VulMeshPipeline> renderPipeline;
    std::array<std::unique_ptr<vul::VulDescriptorSet>, vul::VulSwapChain::MAX_FRAMES_IN_FLIGHT> renderDescSets;
    std::unique_ptr<vul::VulCompPipeline> mipCreationPipeline;
    std::vector<std::unique_ptr<vul::VulDescriptorSet>> mipCreationDescSets;
    std::unique_ptr<vul::VulBuffer> cubeBuf;
    std::unique_ptr<vul::VulBuffer> chunksBuf;
    std::array<std::unique_ptr<vul::VulBuffer>, vul::VulSwapChain::MAX_FRAMES_IN_FLIGHT> ubos;
};
MeshResources createMeshShadingResources(const vul::Vulkano &vulkano);
void updateMeshUbo(const vul::Vulkano &vulkano, const MeshResources &res);
void meshShade(const vul::Vulkano &vulkano, const MeshResources &res, VkCommandBuffer cmdBuf);
