#include "vul_descriptors.hpp"
#include <vul_device.hpp>
#include <vul_renderer.hpp>
#include <vul_camera.hpp>
#include <limits>
#include <memory>
#include <mesh_shading.hpp>
#include <host_device.hpp>
#include <vector>
#include <vulkan/vulkan_core.h>

MeshResources createMeshShadingResources(const vul::VulRenderer &vulRenderer, const vul::VulDescriptorPool &descPool, VkCommandBuffer cmdBuf, const vul::VulDevice &vulDevice)
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

    meshResources.cubeBuf = std::make_unique<vul::VulBuffer>(sizeof(*objDatas.data()), objDatas.size(), true, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, vulDevice);
    meshResources.cubeBuf->writeVector(objDatas, 0, cmdBuf);
    meshResources.chunksBuf = std::make_unique<vul::VulBuffer>(sizeof(*chunkDatas.data()), chunkDatas.size(), true, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, vulDevice);
    meshResources.chunksBuf->writeVector(chunkDatas, 0, cmdBuf);

    for (int i = 0; i < vul::VulSwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        meshResources.ubos[i] = std::make_unique<vul::VulBuffer>(sizeof(MeshUbo), 1, false, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, vulDevice);

        std::vector<vul::VulDescriptorSet::Descriptor> descs;
        vul::VulDescriptorSet::Descriptor desc;
        desc.type = vul::VulDescriptorSet::DescriptorType::storageBuffer;
        desc.stages = VK_SHADER_STAGE_MESH_BIT_EXT | VK_SHADER_STAGE_TASK_BIT_EXT;
        desc.count = 1;
        desc.content = meshResources.cubeBuf.get();
        descs.push_back(desc);

        desc.stages = VK_SHADER_STAGE_TASK_BIT_EXT;
        desc.content = meshResources.chunksBuf.get();
        descs.push_back(desc);

        desc.type = vul::VulDescriptorSet::DescriptorType::uniformBuffer;
        desc.stages = VK_SHADER_STAGE_MESH_BIT_EXT | VK_SHADER_STAGE_TASK_BIT_EXT;
        desc.content = meshResources.ubos[i].get();
        descs.push_back(desc);

        meshResources.renderDescSets[i] = vul::VulDescriptorSet::createDescriptorSet(descs, descPool);
    }

    vul::VulMeshPipeline::PipelineConfigInfo configInfo{};
    configInfo.colorAttachmentFormats = {vulRenderer.getSwapChainColorFormat()};
    configInfo.depthAttachmentFormat = vulRenderer.getDepthFormat();
    configInfo.setLayouts = {meshResources.renderDescSets[0]->getLayout()->getDescriptorSetLayout()};
    configInfo.cullMode = VK_CULL_MODE_BACK_BIT;

    meshResources.renderPipeline = std::make_unique<vul::VulMeshPipeline>(vulDevice, "mesh.task.spv", "mesh.mesh.spv", "mesh.frag.spv", configInfo);

    return meshResources;
}

void updateMeshUbo(const MeshResources &res, const vul::VulCamera &camera, const vul::VulRenderer &vulRenderer)
{
    MeshUbo ubo;
    ubo.projectionMatrix = camera.getProjection();
    ubo.viewMatrix = camera.getView();

    res.ubos[vulRenderer.getFrameIndex()]->writeData(&ubo, sizeof(ubo), 0, VK_NULL_HANDLE);
}

void meshShade(const MeshResources &res, const vul::VulRenderer &vulRenderer, VkCommandBuffer cmdBuf)
{
    vulRenderer.beginRendering(cmdBuf, vul::VulRenderer::SwapChainImageMode::clearPreviousStoreCurrent,
            vul::VulRenderer::DepthImageMode::clearPreviousStoreCurrent, {}, {}, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), 1.0f, 0, 0, 1);
    res.renderPipeline->meshShade(VOLUME_VOLUME / CUBES_PER_MESH / MESH_PER_TASK, 1, 1, nullptr, 0, {res.renderDescSets[vulRenderer.getFrameIndex()]->getSet()}, cmdBuf);
}
