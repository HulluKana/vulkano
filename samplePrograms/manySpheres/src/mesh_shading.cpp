#include <mesh_shading.hpp>

MeshResources createMeshShadingResources(vul::Vulkano &vulkano)
{
    vul::VulMeshPipeline::PipelineConfigInfo configInfo{};
    configInfo.colorAttachmentFormats = {vulkano.vulRenderer.getSwapChainColorFormat()};
    configInfo.depthAttachmentFormat = vulkano.vulRenderer.getDepthFormat();

    MeshResources meshResources;
    meshResources.pipeline = std::make_unique<vul::VulMeshPipeline>(vulkano.getVulDevice(), "mesh.mesh.spv", "mesh.frag.spv", configInfo);

    return meshResources;
}

void meshShade(const vul::Vulkano &vulkano, const MeshResources &res, VkCommandBuffer cmdBuf)
{
    res.pipeline->meshShade(1, 1, 1, cmdBuf);
}
