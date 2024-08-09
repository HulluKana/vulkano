#include <vulkan/vulkan_core.h>
#include "vul_camera.hpp"
#include "vul_renderer.hpp"
#include <memory>
#include <rasterizing.hpp>
#include <host_device.hpp>

#include <random>

RasResources createRasterizationResources(const vul::VulRenderer &vulRenderer, const vul::VulDescriptorPool &descPool, vul::VulCmdPool &cmdPool, const vul::VulDevice &vulDevice)
{
    RasResources res{};

    std::random_device dev{};
    std::mt19937 rng{dev()};
    std::uniform_real_distribution<> dist{-0.5, 0.5};
    std::vector<ObjData> objDatas;
    for (int x = -VOLUME_LEN / 2; x < VOLUME_LEN / 2; x++) {
        for (int y = -VOLUME_LEN / 2; y < VOLUME_LEN / 2; y++) {
            for (int z = -VOLUME_LEN / 2; z < VOLUME_LEN / 2; z++) {
                glm::vec3 pos = glm::vec3(x, y, z);
                // pos += glm::vec3(dist(rng), dist(rng), dist(rng));

                const glm::vec3 offset = pos - glm::vec3(-static_cast<float>(VOLUME_LEN) / 2.0f); 
                const glm::vec3 color = offset / static_cast<float>(VOLUME_LEN);

                objDatas.emplace_back(ObjData{glm::vec4{pos, 1.0f}, glm::vec4{color, 1.0f}});
            }
        }
    }
    VkCommandBuffer cmdBuf = cmdPool.getPrimaryCommandBuffer();
    res.objDataBuf = std::make_unique<vul::VulBuffer>(sizeof(*objDatas.data()), objDatas.size(), true, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, vulDevice);
    res.objDataBuf->writeVector(objDatas, 0, cmdBuf);
    cmdPool.submitAndWait(cmdBuf);

    res.scene = std::make_unique<vul::Scene>(vulDevice);
    res.scene->loadCubes({{{0.0f, 0.0f, 0.0f}, {0.3f, 0.3f, 0.3f}}}, {{}}, {}, cmdPool);
    //res.scene->loadSpheres({{{0.0f, 0.0f, 0.0f}, 0.3f, 3}}, {{}}, {}, cmdPool);

    res.drawData.firstIndex = 0;
    res.drawData.indexCount = res.scene->indices.size();
    res.drawData.vertexOffset = 0;
    res.drawData.instanceCount = objDatas.size();
    res.drawData.firstInstance = 0;
    res.drawData.pPushData = nullptr;
    res.drawData.pushDataSize = 0;

    for (int i = 0; i < vul::VulSwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        res.ubos[i] = std::make_unique<vul::VulBuffer>(sizeof(RasUbo), 1, false, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, vulDevice);

        std::vector<vul::VulDescriptorSet::Descriptor> descs;
        vul::VulDescriptorSet::Descriptor desc;
        desc.count = 1;

        desc.type = vul::VulDescriptorSet::DescriptorType::uniformBuffer;
        desc.stages = VK_SHADER_STAGE_VERTEX_BIT;
        desc.content = res.ubos[i].get();
        descs.push_back(desc);

        desc.type = vul::VulDescriptorSet::DescriptorType::storageBuffer;
        desc.stages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        desc.content = res.objDataBuf.get();
        descs.push_back(desc);

        res.descSets[i] = vul::VulDescriptorSet::createDescriptorSet(descs, descPool);
    }

    vul::VulPipeline::PipelineConfigInfo pipelineConfig{};
    pipelineConfig.colorAttachmentFormats = {vulRenderer.getSwapChainColorFormat()};
    pipelineConfig.depthAttachmentFormat = vulRenderer.getDepthFormat();
    pipelineConfig.enableColorBlending = false;
    pipelineConfig.cullMode = VK_CULL_MODE_BACK_BIT;
    pipelineConfig.attributeDescriptions = {VkVertexInputAttributeDescription{0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0}};
    pipelineConfig.bindingDescriptions = {VkVertexInputBindingDescription{0, sizeof(glm::vec3), VK_VERTEX_INPUT_RATE_VERTEX}};
    pipelineConfig.setLayouts = {res.descSets[0]->getLayout()->getDescriptorSetLayout()};

    res.pipeline = std::make_unique<vul::VulPipeline>(vulDevice, "../bin/default.vert.spv", "../bin/default.frag.spv", pipelineConfig);

    return res;
}

void updateRasUbo(const vul::VulCamera &camera, const vul::VulRenderer &vulRenderer, RasResources &res)
{
    RasUbo ubo{};
    ubo.viewMatrix = camera.getView();
    ubo.projectionMatrix = camera.getProjection();
    for (int i = 0; i < vul::VulSwapChain::MAX_FRAMES_IN_FLIGHT; i++) res.ubos[i]->writeData(&ubo, sizeof(ubo), 0, VK_NULL_HANDLE);
}

void rasterize(const RasResources &res, const vul::VulRenderer &vulRenderer, VkCommandBuffer cmdBuf)
{
    vulRenderer.beginRendering(cmdBuf, {}, vul::VulRenderer::SwapChainImageMode::clearPreviousStoreCurrent, vul::VulRenderer::DepthImageMode::clearPreviousStoreCurrent, {0.0f, 0.0f, 0.0f, 1.0f}, 1.0f, 0, 0);
    res.pipeline->draw(cmdBuf, {res.descSets[vulRenderer.getFrameIndex()]->getSet()}, {res.scene->vertexBuffer->getBuffer()}, res.scene->indexBuffer->getBuffer(), {res.drawData});
}
