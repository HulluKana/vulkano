#include <raytracing.hpp>
#include <host_device.hpp>

#include <vul_acceleration_structure.hpp>
#include <vul_rt_pipeline.hpp>
#include <iostream>

RtResources createRaytracingResources(vul::Vulkano &vulkano)
{
    RtResources res{};

    vulkano.createSquare(0.0f, 0.0f, 1.0f, 1.0f);

    {
        std::vector<vul::VulAs::Aabb> aabbs;
        for (int y = -VOLUME_LEN / 2; y < VOLUME_LEN / 2; y++) {
            for (int x = -VOLUME_LEN / 2; x < VOLUME_LEN / 2; x++) {
                glm::vec3 pos = glm::vec3(x, y, 0);
                aabbs.emplace_back(vul::VulAs::Aabb{pos - glm::vec3(0.3f), pos + glm::vec3(0.3f), 0});
            }
        }

        std::vector<vul::VulAs::InstanceInfo> instanceInfos;
        for (int i = 0; i < VOLUME_LEN; i++) {
            vul::transform3D trans{};
            trans.pos = glm::vec3(0.0f, 0.0f, i - VOLUME_LEN / 2);
            instanceInfos.push_back(vul::VulAs::InstanceInfo{0, 0, static_cast<uint32_t>(i % 2), trans.transformMat()});
        };

        res.as = std::make_unique<vul::VulAs>(vulkano.getVulDevice());
        res.as->loadAabbs(aabbs, instanceInfos, true);
    }
    {
        std::vector<glm::vec4> spheres;
        for (int y = -VOLUME_LEN / 2; y < VOLUME_LEN / 2; y++) {
            for (int x = -VOLUME_LEN / 2; x < VOLUME_LEN / 2; x++) {
                glm::vec3 pos = glm::vec3(x, y, -VOLUME_LEN / 2);
                spheres.emplace_back(glm::vec4{pos, 0.3f});
            }
        }
        res.spheresBuf = std::make_unique<vulB::VulBuffer>(vulkano.getVulDevice());
        res.spheresBuf->loadVector(spheres);
        res.spheresBuf->createBuffer(true, static_cast<vulB::VulBuffer::Usage>(
                    vulB::VulBuffer::usage_ssbo | vulB::VulBuffer::usage_transferDst));
    }

    for (int i = 0; i < vulB::VulSwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        res.rtImgs[i] = std::make_unique<vul::VulImage>(vulkano.getVulDevice());
        res.rtImgs[i]->keepRegularRaw2d32bitRgbaEmpty(vulkano.getSwapChainExtent().width, vulkano.getSwapChainExtent().height);
        res.rtImgs[i]->createDefaultImageSingleTime(vul::VulImage::ImageType::storage2d);

        res.ubos[i] = std::make_unique<vulB::VulBuffer>(vulkano.getVulDevice());
        res.ubos[i]->keepEmpty(sizeof(RtUbo), 1);
        res.ubos[i]->createBuffer(false, static_cast<vulB::VulBuffer::Usage>(
                    vulB::VulBuffer::usage_ubo | vulB::VulBuffer::usage_transferDst));

        std::vector<vul::Vulkano::Descriptor> descriptors;
        vul::Vulkano::Descriptor desc{};
        desc.count = 1;

        desc.type = vul::Vulkano::DescriptorType::storageImage;
        desc.stages = {vul::Vulkano::ShaderStage::rgen, vul::Vulkano::ShaderStage::frag};
        desc.content = res.rtImgs[i].get();
        descriptors.push_back(desc);

        desc.type = vul::Vulkano::DescriptorType::accelerationStructure;
        desc.stages = {vul::Vulkano::ShaderStage::rgen, vul::Vulkano::ShaderStage::rchit};
        desc.content = res.as.get();
        descriptors.push_back(desc);

        desc.type = vul::Vulkano::DescriptorType::ssbo;
        desc.stages = {vul::Vulkano::ShaderStage::rint};
        desc.content = res.spheresBuf.get();
        descriptors.push_back(desc);

        desc.type = vul::Vulkano::DescriptorType::ubo;
        desc.stages = {vul::Vulkano::ShaderStage::vert, vul::Vulkano::ShaderStage::rgen};
        desc.content = res.ubos[i].get();
        descriptors.push_back(desc);

        res.descSets[i] = vulkano.createDescriptorSet(descriptors);
    }

    vulB::VulPipeline::PipelineConfigInfo pipConf{};
    pipConf.attributeDescriptions = {{0, 0, VK_FORMAT_R32G32_SFLOAT, 0}, {1, 1, VK_FORMAT_R32G32_SFLOAT, 0}};
    pipConf.bindingDescriptions = {{0, sizeof(glm::vec2), VK_VERTEX_INPUT_RATE_VERTEX},
        {1, sizeof(glm::vec2), VK_VERTEX_INPUT_RATE_VERTEX}};
    pipConf.colorAttachmentFormats = {vulkano.vulRenderer.getSwapChainColorFormat()};
    pipConf.setLayouts = {res.descSets[0]->getLayout()->getDescriptorSetLayout()};
    pipConf.enableColorBlending = false;
    pipConf.cullMode = VK_CULL_MODE_NONE;

    vul::Vulkano::RenderData renderData{};
    renderData.is3d = false;
    renderData.depthImageMode = vulB::VulRenderer::DepthImageMode::noDepthImage;
    renderData.swapChainImageMode = vulB::VulRenderer::SwapChainImageMode::clearPreviousStoreCurrent;
    renderData.sampleFromDepth = false;
    for (int i = 0; i < vulB::VulSwapChain::MAX_FRAMES_IN_FLIGHT; i++) renderData.descriptorSets[i].push_back(res.descSets[i]);
    renderData.pipeline = std::make_shared<vulB::VulPipeline>(vulkano.getVulDevice(),
            "../bin/raytrace.vert.spv", "../bin/raytrace.frag.spv", pipConf);
    vulkano.renderDatas.push_back(renderData);

    res.pipeline = std::make_unique<vul::VulRtPipeline>(vulkano.getVulDevice(), "../bin/raytrace.rgen.spv",
            std::vector<std::string>{"../bin/raytrace.rmiss.spv"}, std::vector<std::string>{"../bin/raytraceNormal.rchit.spv",
            "../bin/raytraceInverted.rchit.spv"}, std::vector<std::string>(), std::vector<std::string>{"../bin/raytraceSphere.rint.spv",
            "../bin/raytraceCube.rint.spv"}, std::vector<vul::VulRtPipeline::HitGroup>{{0, -1, 0}, {1, -1, 1}},
            std::vector{res.descSets[0]->getLayout()->getDescriptorSetLayout()});
    return res;
}

void raytrace(const vul::Vulkano &vulkano, const RtResources &res, VkCommandBuffer cmdBuf)
{
    std::vector<VkDescriptorSet> vkDescSets = {res.descSets[(vulkano.getFrameIdx()) % vulB::VulSwapChain::MAX_FRAMES_IN_FLIGHT]->getSet()};
    res.pipeline->traceRays(vulkano.getSwapChainExtent().width, vulkano.getSwapChainExtent().height, 0, nullptr, vkDescSets, cmdBuf);
}

void updateRtUbo(const vul::Vulkano &vulkano, RtResources &res, bool fullUpdate)
{
    if (fullUpdate) {
        RtUbo ubo{};
        ubo.cameraPosition = glm::vec4(vulkano.cameraTransform.pos, 1.0f);
        ubo.inverseViewMatrix = glm::inverse(vulkano.camera.getView());
        ubo.inverseProjectionMatrix = glm::inverse(vulkano.camera.getProjection());
        for (int i = 0; i < vulB::VulSwapChain::MAX_FRAMES_IN_FLIGHT; i++)
            res.ubos[i]->writeData(&ubo, sizeof(ubo), 0);
    }
    else {
        RtUbo ubo{};
        ubo.cameraPosition = glm::vec4(vulkano.cameraTransform.pos, 1.0f);
        ubo.inverseViewMatrix = glm::inverse(vulkano.camera.getView());
        res.ubos[vulkano.getFrameIdx()]->writeData(&ubo, sizeof(glm::vec4) + sizeof(glm::mat4), 0);
    }
}

void resizeRtImgs(const vul::Vulkano &vulkano, RtResources &res)
{
    VkCommandBuffer cmdBuf = vulkano.getVulDevice().beginSingleTimeCommands();
    std::array<vul::VulImage *, vulB::VulSwapChain::MAX_FRAMES_IN_FLIGHT> oldRtImgs;
    for (size_t i = 0; i < res.rtImgs.size(); i++) {
        oldRtImgs[i] = res.rtImgs[i].release();
        res.rtImgs[i] = std::make_unique<vul::VulImage>(vulkano.getVulDevice());
        res.rtImgs[i]->keepRegularRaw2d32bitRgbaEmpty(vulkano.getSwapChainExtent().width, vulkano.getSwapChainExtent().height);
        res.rtImgs[i]->createDefaultImage(vul::VulImage::ImageType::storage2d, cmdBuf);
    }
    vulkano.getVulDevice().endSingleTimeCommands(cmdBuf);

    for (size_t i = 0; i < res.rtImgs.size(); i++) {
        vulkano.renderDatas[0].descriptorSets[i][0]->descriptorInfos[0].imageInfos[0].imageView = res.rtImgs[i]->getImageView();
        vulkano.renderDatas[0].descriptorSets[i][0]->update();
    }
    for (size_t i = 0; i < oldRtImgs.size(); i++) delete oldRtImgs[i];
}
