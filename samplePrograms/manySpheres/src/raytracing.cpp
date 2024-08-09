#include <vulkan/vulkan_core.h>
#include "vul_camera.hpp"
#include "vul_command_pool.hpp"
#include "vul_descriptors.hpp"
#include "vul_renderer.hpp"
#include <raytracing.hpp>
#include <host_device.hpp>
#include <vul_transform.hpp>
#include <vul_pipeline.hpp>

#include <vul_acceleration_structure.hpp>
#include <vul_rt_pipeline.hpp>
#include <iostream>

RtResources createRaytracingResources(const vul::VulRenderer &vulRenderer, const vul::VulDescriptorPool &descPool, vul::VulCmdPool &cmdPool, const vul::VulDevice &vulDevice)
{
    RtResources res{};

    res.fullScreenQuad = std::make_unique<vul::Scene>(vulDevice);
    res.fullScreenQuad->loadPlanes({{{-1.0f, -1.0f, 0.5f}, {1.0f, -1.0f, 0.5f}, {-1.0f, 1.0f, 0.5f}, {1.0f, 1.0f, 0.5f}, 0}}, {}, {.normal = false, .tangent = false, .material = false}, cmdPool);

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

        res.as = std::make_unique<vul::VulAs>(vulDevice);
        res.as->loadAabbs(aabbs, instanceInfos, true, cmdPool);
    }
    VkCommandBuffer cmdBuf = cmdPool.getPrimaryCommandBuffer();
    {
        std::vector<glm::vec4> spheres;
        for (int y = -VOLUME_LEN / 2; y < VOLUME_LEN / 2; y++) {
            for (int x = -VOLUME_LEN / 2; x < VOLUME_LEN / 2; x++) {
                glm::vec3 pos = glm::vec3(x, y, -VOLUME_LEN / 2);
                spheres.emplace_back(glm::vec4{pos, 0.3f});
            }
        }
        res.spheresBuf = std::make_unique<vul::VulBuffer>(sizeof(*spheres.data()), spheres.size(), true, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, vulDevice);
        res.spheresBuf->writeVector(spheres, 0, cmdBuf);
    }

    for (int i = 0; i < vul::VulSwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        res.rtImgs[i] = std::make_unique<vul::VulImage>(vulDevice);
        res.rtImgs[i]->keepRegularRaw2d32bitRgbaEmpty(vulRenderer.getSwapChainExtent().width, vulRenderer.getSwapChainExtent().height);
        res.rtImgs[i]->createDefaultImage(vul::VulImage::ImageType::storage2d, cmdBuf);

        res.ubos[i] = std::make_unique<vul::VulBuffer>(sizeof(RtUbo), 1, false, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, vulDevice);

        std::vector<vul::VulDescriptorSet::Descriptor> descriptors;
        vul::VulDescriptorSet::Descriptor desc{};
        desc.count = 1;

        desc.type = vul::VulDescriptorSet::DescriptorType::storageImage;
        desc.stages = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_FRAGMENT_BIT;
        desc.content = res.rtImgs[i].get();
        descriptors.push_back(desc);

        desc.type = vul::VulDescriptorSet::DescriptorType::accelerationStructure;
        desc.stages = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
        desc.content = res.as.get();
        descriptors.push_back(desc);

        desc.type = vul::VulDescriptorSet::DescriptorType::storageBuffer;
        desc.stages = VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
        desc.content = res.spheresBuf.get();
        descriptors.push_back(desc);

        desc.type = vul::VulDescriptorSet::DescriptorType::uniformBuffer;
        desc.stages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR;
        desc.content = res.ubos[i].get();
        descriptors.push_back(desc);

        res.descSets[i] = vul::VulDescriptorSet::createDescriptorSet(descriptors, descPool);
    }
    cmdPool.submitAndWait(cmdBuf);

    vul::VulPipeline::PipelineConfigInfo pipConf{};
    pipConf.attributeDescriptions = {{0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0}, {1, 1, VK_FORMAT_R32G32_SFLOAT, 0}};
    pipConf.bindingDescriptions = {{0, sizeof(glm::vec3), VK_VERTEX_INPUT_RATE_VERTEX},
        {1, sizeof(glm::vec2), VK_VERTEX_INPUT_RATE_VERTEX}};
    pipConf.colorAttachmentFormats = {vulRenderer.getSwapChainColorFormat()};
    pipConf.setLayouts = {res.descSets[0]->getLayout()->getDescriptorSetLayout()};
    pipConf.enableColorBlending = false;
    pipConf.cullMode = VK_CULL_MODE_NONE;

    res.fullScreenQuadPipeline = std::make_unique<vul::VulPipeline>(vulDevice, "raytrace.vert.spv", "raytrace.frag.spv", pipConf);

    res.rtPipeline = std::make_unique<vul::VulRtPipeline>(vulDevice, "../bin/raytrace.rgen.spv",
            std::vector<std::string>{"../bin/raytrace.rmiss.spv"}, std::vector<std::string>{"../bin/raytraceNormal.rchit.spv",
            "../bin/raytraceInverted.rchit.spv"}, std::vector<std::string>(), std::vector<std::string>{"../bin/raytraceSphere.rint.spv",
            "../bin/raytraceCube.rint.spv"}, std::vector<vul::VulRtPipeline::HitGroup>{{0, -1, 0}, {1, -1, 1}},
            std::vector{res.descSets[0]->getLayout()->getDescriptorSetLayout()}, cmdPool);
    return res;
}

void raytrace(const RtResources &res, const vul::VulRenderer &vulRenderer, VkCommandBuffer cmdBuf)
{
    std::vector<VkDescriptorSet> vkDescSets = {res.descSets[(vulRenderer.getFrameIndex()) % vul::VulSwapChain::MAX_FRAMES_IN_FLIGHT]->getSet()};
    res.rtPipeline->traceRays(vulRenderer.getSwapChainExtent().width, vulRenderer.getSwapChainExtent().height, 0, nullptr, vkDescSets, cmdBuf);

    vulRenderer.beginRendering(cmdBuf, {}, vul::VulRenderer::SwapChainImageMode::clearPreviousStoreCurrent, vul::VulRenderer::DepthImageMode::noDepthImage, {}, {}, vulRenderer.getSwapChainExtent().width, vulRenderer.getSwapChainExtent().height);
    res.fullScreenQuadPipeline->draw(cmdBuf, vkDescSets, {res.fullScreenQuad->vertexBuffer->getBuffer(), res.fullScreenQuad->uvBuffer->getBuffer()},
            res.fullScreenQuad->indexBuffer->getBuffer(), {{.indexCount = static_cast<uint32_t>(res.fullScreenQuad->indices.size())}});
}

void updateRtUbo(RtResources &res, const vul::VulRenderer &vulRenderer, const vul::VulCamera &camera)
{
    RtUbo ubo{};
    ubo.cameraPosition = glm::vec4(camera.pos, 1.0f);
    ubo.inverseViewMatrix = glm::inverse(camera.getView());
    ubo.inverseProjectionMatrix = glm::inverse(camera.getProjection());
    for (int i = 0; i < vul::VulSwapChain::MAX_FRAMES_IN_FLIGHT; i++)
        res.ubos[i]->writeData(&ubo, sizeof(ubo), 0, VK_NULL_HANDLE);
}

void resizeRtImgs(RtResources &res, const vul::VulRenderer &vulRenderer, vul::VulCmdPool &cmdPool, const vul::VulDevice &vulDevice)
{
    VkCommandBuffer cmdBuf = cmdPool.getPrimaryCommandBuffer();
    std::array<vul::VulImage *, vul::VulSwapChain::MAX_FRAMES_IN_FLIGHT> oldRtImgs;
    for (size_t i = 0; i < res.rtImgs.size(); i++) {
        oldRtImgs[i] = res.rtImgs[i].release();
        res.rtImgs[i] = std::make_unique<vul::VulImage>(vulDevice);
        res.rtImgs[i]->keepRegularRaw2d32bitRgbaEmpty(vulRenderer.getSwapChainExtent().width, vulRenderer.getSwapChainExtent().height);
        res.rtImgs[i]->createDefaultImage(vul::VulImage::ImageType::storage2d, cmdBuf);
    }
    cmdPool.submitAndWait(cmdBuf);

    for (size_t i = 0; i < res.rtImgs.size(); i++) {
        res.descSets[i]->descriptorInfos[0].imageInfos[0].imageView = res.rtImgs[i]->getImageView();
        res.descSets[i]->update();
    }
    for (size_t i = 0; i < oldRtImgs.size(); i++) delete oldRtImgs[i];
}
