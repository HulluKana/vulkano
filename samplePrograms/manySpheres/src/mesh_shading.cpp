#include "vul_comp_pipeline.hpp"
#include "vul_image.hpp"
#include <iostream>
#include <limits>
#include <memory>
#include <mesh_shading.hpp>
#include <host_device.hpp>
#include <vector>
#include <vulkan/vulkan_core.h>

MeshResources createMeshShadingResources(const vul::Vulkano &vulkano)
{
    MeshResources meshResources;

    std::vector<ObjData> objDatas;
    std::vector<ChunkData> chunkDatas;
    uint32_t inChunkIdx = 0;
    glm::vec3 minChunkPos = glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 maxChunkPos = glm::vec3(std::numeric_limits<float>::min());
    for (int x = -VOLUME_LEN / 2; x < VOLUME_LEN / 2; x++) {
        for (int y = -VOLUME_LEN / 2; y < VOLUME_LEN / 2; y++) {
            for (int z = -VOLUME_LEN / 2; z < VOLUME_LEN / 2; z++) {
                constexpr float SIDE_LENGTH = 0.5f;

                glm::vec3 pos = glm::vec3(x, y, z);
                const glm::vec3 offset = pos + glm::vec3(static_cast<float>(VOLUME_LEN) / 2.0f); 
                const glm::vec3 color = offset / static_cast<float>(VOLUME_LEN);
                objDatas.emplace_back(ObjData{glm::vec4{pos, SIDE_LENGTH}, glm::vec4{color, 1.0f}});

                minChunkPos = glm::min(minChunkPos, pos);
                maxChunkPos = glm::max(maxChunkPos, pos);
                inChunkIdx++;
                if (inChunkIdx >= CUBES_PER_MESH) {
                    chunkDatas.emplace_back(ChunkData{.minPos = minChunkPos - glm::vec3(SIDE_LENGTH / 2.0f), .maxPos = maxChunkPos + glm::vec3(SIDE_LENGTH / 2.0)}); 
                    minChunkPos = glm::vec3(std::numeric_limits<float>::max());
                    maxChunkPos = glm::vec3(std::numeric_limits<float>::min());
                    inChunkIdx = 0;
                }
            }
        }
    }

    meshResources.cubeBuf = std::make_unique<vul::VulBuffer>(vulkano.getVulDevice());
    meshResources.cubeBuf->loadVector(objDatas);
    meshResources.cubeBuf->createBuffer(true, static_cast<vul::VulBuffer::Usage>(vul::VulBuffer::usage_ssbo | vul::VulBuffer::usage_transferDst));

    meshResources.chunksBuf = std::make_unique<vul::VulBuffer>(vulkano.getVulDevice());
    meshResources.chunksBuf->loadVector(chunkDatas);
    meshResources.chunksBuf->createBuffer(true, static_cast<vul::VulBuffer::Usage>(vul::VulBuffer::usage_ssbo | vul::VulBuffer::usage_transferDst));

    for (int i = 0; i < vul::VulSwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        meshResources.ubos[i] = std::make_unique<vul::VulBuffer>(vulkano.getVulDevice());
        meshResources.ubos[i]->keepEmpty(sizeof(MeshUbo), 1);
        meshResources.ubos[i]->createBuffer(false, vul::VulBuffer::usage_ubo);

        std::vector<vul::Vulkano::Descriptor> descs;
        vul::Vulkano::Descriptor desc;
        desc.type = vul::Vulkano::DescriptorType::ssbo;
        desc.stages = {vul::Vulkano::ShaderStage::mesh, vul::Vulkano::ShaderStage::task};
        desc.count = 1;
        desc.content = meshResources.cubeBuf.get();
        descs.push_back(desc);

        desc.stages = {vul::Vulkano::ShaderStage::task};
        desc.content = meshResources.chunksBuf.get();
        descs.push_back(desc);

        desc.type = vul::Vulkano::DescriptorType::ubo;
        desc.stages = {vul::Vulkano::ShaderStage::mesh, vul::Vulkano::ShaderStage::task};
        desc.content = meshResources.ubos[i].get();
        descs.push_back(desc);

        meshResources.renderDescSets[i] = vulkano.createDescriptorSet(descs);
    }

    vul::VulMeshPipeline::PipelineConfigInfo configInfo{};
    configInfo.colorAttachmentFormats = {vulkano.vulRenderer.getSwapChainColorFormat()};
    configInfo.depthAttachmentFormat = vulkano.vulRenderer.getDepthFormat();
    configInfo.setLayouts = {meshResources.renderDescSets[0]->getLayout()->getDescriptorSetLayout()};
    configInfo.cullMode = VK_CULL_MODE_BACK_BIT;

    meshResources.renderPipeline = std::make_unique<vul::VulMeshPipeline>(vulkano.getVulDevice(), "mesh.task.spv", "mesh.mesh.spv", "mesh.frag.spv", configInfo);

    for (const std::unique_ptr<vul::VulImage> &depthImg : vulkano.vulRenderer.getDepthImages()) {
        std::vector<vul::Vulkano::RawImageDescriptorInfo> descriptorInfos;
        for (uint32_t i = 0; i < depthImg->getMipCount(); i++) {
            vul::Vulkano::RawImageDescriptorInfo descInfo;
            descInfo.descriptorInfo = depthImg->getMipDescriptorInfo(i);
            descInfo.descriptorInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            descInfo.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorInfos.push_back(descInfo);
        }
        vul::Vulkano::Descriptor desc;
        desc.type = vul::Vulkano::DescriptorType::rawImageInfo;
        desc.stages = {vul::Vulkano::ShaderStage::comp};
        desc.content = descriptorInfos.data();
        desc.count = descriptorInfos.size();
        meshResources.mipCreationDescSets.push_back(vulkano.createDescriptorSet({desc}));
    }

    /*
    meshResources.mipCreationPipeline = std::make_unique<vul::VulCompPipeline>("mipCreator.comp.spv",
            std::vector<VkDescriptorSetLayout>{meshResources.mipCreationDescSets[0]->getLayout()->getDescriptorSetLayout()},
            vulkano.getVulDevice(), meshResources.mipCreationDescSets.size());
    */

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
    res.renderPipeline->meshShade(VOLUME_VOLUME / CUBES_PER_MESH / MESH_PER_TASK, 1, 1, nullptr, 0, {res.renderDescSets[vulkano.getFrameIdx()]->getSet()}, cmdBuf);
    vulkano.vulRenderer.stopRendering(cmdBuf);
}
