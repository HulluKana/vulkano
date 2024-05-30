#include "vul_swap_chain.hpp"
#include <rasterizing.hpp>
#include <host_device.hpp>

#include <random>

RasResources createRasterizationResources(vul::Vulkano &vulkano)
{
    RasResources res{};

    std::random_device dev{};
    std::mt19937 rng{dev()};
    std::uniform_real_distribution<> dist{-0.5, 0.5};
    std::vector<ObjData> objDatas;
    constexpr int VOLUME_LEN = 50;
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
    res.objDataBuf = std::make_shared<vulB::VulBuffer>(vulkano.getVulDevice());
    res.objDataBuf->loadVector(objDatas);
    res.objDataBuf->createBuffer(true, static_cast<vulB::VulBuffer::Usage>(vulB::VulBuffer::usage_ssbo | vulB::VulBuffer::usage_transferDst));

    //vulkano.scene.loadCubes({{{0.0f, 0.0f, 0.0f}, {0.3f, 0.3f, 0.3f}}});
    vulkano.scene.loadSpheres({{{0.0f, 0.0f, 0.0f}, 0.3f, 3}});
    vulkano.hasScene = true;

    vulB::VulPipeline::DrawData drawData;
    drawData.firstIndex = vulkano.scene.meshes[0].firstIndex;
    drawData.indexCount = vulkano.scene.meshes[0].indexCount;
    drawData.vertexOffset = vulkano.scene.meshes[0].vertexOffset;
    drawData.instanceCount = objDatas.size();
    drawData.firstInstance = 0;
    drawData.pPushData = nullptr;
    drawData.pushDataSize = 0;

    for (int i = 0; i < vulB::VulSwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        res.ubos[i] = std::make_unique<vulB::VulBuffer>(vulkano.getVulDevice());
        res.ubos[i]->keepEmpty(sizeof(RasUbo), 1);
        res.ubos[i]->createBuffer(false, static_cast<vulB::VulBuffer::Usage>(vulB::VulBuffer::usage_ubo | vulB::VulBuffer::usage_transferDst));

        std::vector<vul::Vulkano::Descriptor> descs;
        vul::Vulkano::Descriptor desc;
        desc.count = 1;

        desc.type = vul::Vulkano::DescriptorType::ubo;
        desc.stages = {vul::Vulkano::ShaderStage::vert};
        desc.content = res.ubos[i].get();
        descs.push_back(desc);

        desc.type = vul::Vulkano::DescriptorType::ssbo;
        desc.stages = {vul::Vulkano::ShaderStage::vert, vul::Vulkano::ShaderStage::frag};
        desc.content = res.objDataBuf.get();
        descs.push_back(desc);

        res.descSets[i] = vulkano.createDescriptorSet(descs);
    }

    vulB::VulPipeline::PipelineConfigInfo pipelineConfig{};
    pipelineConfig.colorAttachmentFormats = {vulkano.vulRenderer.getSwapChainColorFormat()};
    pipelineConfig.depthAttachmentFormat = vulkano.vulRenderer.getDepthFormat();
    pipelineConfig.enableColorBlending = false;
    pipelineConfig.cullMode = VK_CULL_MODE_BACK_BIT;
    pipelineConfig.attributeDescriptions = {VkVertexInputAttributeDescription{0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0}};
    pipelineConfig.bindingDescriptions = {VkVertexInputBindingDescription{0, sizeof(glm::vec3), VK_VERTEX_INPUT_RATE_VERTEX}};
    pipelineConfig.setLayouts = {res.descSets[0]->getLayout()->getDescriptorSetLayout()};

    vul::Vulkano::RenderData renderData{};
    renderData.is3d = true;
    renderData.swapChainImageMode = vulB::VulRenderer::SwapChainImageMode::clearPreviousStoreCurrent;
    renderData.depthImageMode = vulB::VulRenderer::DepthImageMode::clearPreviousStoreCurrent;
    renderData.sampleFromDepth = false;
    renderData.pipeline = std::make_shared<vulB::VulPipeline>(vulkano.getVulDevice(), "../bin/default.vert.spv", "../bin/default.frag.spv", pipelineConfig);
    renderData.drawDatas = {drawData};
    for (int i = 0; i < vulB::VulSwapChain::MAX_FRAMES_IN_FLIGHT; i++) renderData.descriptorSets[i].push_back(res.descSets[i]);
    vulkano.renderDatas.push_back(renderData);

    return res;
}

void updateRasUbo(const vul::Vulkano &vulkano, RasResources &res, bool fullUpdate)
{
    if (fullUpdate) {
        RasUbo ubo{};
        ubo.viewMatrix = vulkano.camera.getView();
        ubo.projectionMatrix = vulkano.camera.getProjection();
        for (int i = 0; i < vulB::VulSwapChain::MAX_FRAMES_IN_FLIGHT; i++) res.ubos[i]->writeData(&ubo, sizeof(ubo), 0);
    }
    else {
        glm::mat4 viewMat = vulkano.camera.getView();
        res.ubos[vulkano.getFrameIdx()]->writeData(&viewMat, sizeof(viewMat), 0);
    }
}
