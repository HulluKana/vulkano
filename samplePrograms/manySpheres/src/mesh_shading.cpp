#include "vul_buffer.hpp"
#include "vul_swap_chain.hpp"
#include <iostream>
#include <memory>
#include <mesh_shading.hpp>
#include <host_device.hpp>
#include <vector>

MeshResources createMeshShadingResources(const vul::Vulkano &vulkano)
{
    MeshResources meshResources;

    std::vector<ObjData> objDatas;
    for (int x = -VOLUME_LEN / 2; x < VOLUME_LEN / 2; x++) {
        for (int y = -VOLUME_LEN / 2; y < VOLUME_LEN / 2; y++) {
            for (int z = -VOLUME_LEN / 2; z < VOLUME_LEN / 2; z++) {
                glm::vec3 pos = glm::vec3(x, y, z);
                const glm::vec3 offset = pos + glm::vec3(static_cast<float>(VOLUME_LEN) / 2.0f); 
                const glm::vec3 color = offset / static_cast<float>(VOLUME_LEN);
                objDatas.emplace_back(ObjData{glm::vec4{pos, 0.5f}, glm::vec4{color, 1.0f}});
            }
        }
    }

    meshResources.cubeBuf = std::make_unique<vul::VulBuffer>(vulkano.getVulDevice());
    meshResources.cubeBuf->loadVector(objDatas);
    meshResources.cubeBuf->createBuffer(true, static_cast<vul::VulBuffer::Usage>(vul::VulBuffer::usage_ssbo | vul::VulBuffer::usage_transferDst));

    for (int i = 0; i < vul::VulSwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        meshResources.ubos[i] = std::make_unique<vul::VulBuffer>(vulkano.getVulDevice());
        meshResources.ubos[i]->keepEmpty(sizeof(MeshUbo), 1);
        meshResources.ubos[i]->createBuffer(false, vul::VulBuffer::usage_ubo);

        std::vector<vul::Vulkano::Descriptor> descs;
        vul::Vulkano::Descriptor desc;
        desc.type = vul::Vulkano::DescriptorType::ssbo;
        desc.stages = {vul::Vulkano::ShaderStage::mesh, vul::Vulkano::ShaderStage::frag};
        desc.count = 1;
        desc.content = meshResources.cubeBuf.get();
        descs.push_back(desc);

        desc.type = vul::Vulkano::DescriptorType::ubo;
        desc.stages = {vul::Vulkano::ShaderStage::mesh};
        desc.content = meshResources.ubos[i].get();
        descs.push_back(desc);

        meshResources.descSets[i] = vulkano.createDescriptorSet(descs);
    }

    vul::VulMeshPipeline::PipelineConfigInfo configInfo{};
    configInfo.colorAttachmentFormats = {vulkano.vulRenderer.getSwapChainColorFormat()};
    configInfo.depthAttachmentFormat = vulkano.vulRenderer.getDepthFormat();
    configInfo.setLayouts = {meshResources.descSets[0]->getLayout()->getDescriptorSetLayout()};

    meshResources.pipeline = std::make_unique<vul::VulMeshPipeline>(vulkano.getVulDevice(), "mesh.task.spv", "mesh.mesh.spv", "mesh.frag.spv", configInfo);

    return meshResources;
}

void updateMeshUbo(const vul::Vulkano &vulkano, const MeshResources &res)
{
    MeshUbo ubo;
    ubo.projectionMatrix = vulkano.camera.getProjection();
    ubo.viewMatrix = vulkano.camera.getView();

    res.ubos[vulkano.getFrameIdx()]->writeData(&ubo, sizeof(ubo), 0);
}

void meshShade(const vul::Vulkano &vulkano, const MeshResources &res, VkCommandBuffer cmdBuf)
{
    vulkano.vulRenderer.beginRendering(cmdBuf, {}, vul::VulRenderer::SwapChainImageMode::clearPreviousStoreCurrent,
            vul::VulRenderer::DepthImageMode::clearPreviousStoreCurrent, 0, 0);
    res.pipeline->meshShade(1, 1, 1, nullptr, 0, {res.descSets[vulkano.getFrameIdx()]->getSet()}, cmdBuf);
    vulkano.vulRenderer.stopRendering(cmdBuf);
}
