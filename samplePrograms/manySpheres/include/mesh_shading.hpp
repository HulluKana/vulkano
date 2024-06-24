#pragma once

#include <vulkano_program.hpp>
#include <vul_mesh_pipeline.hpp>

struct MeshResources {
    std::unique_ptr<vul::VulMeshPipeline> pipeline;
};
MeshResources createMeshShadingResources(vul::Vulkano &vulkano);
void meshShade(const vul::Vulkano &vulkano, const MeshResources &res, VkCommandBuffer cmdBuf);
