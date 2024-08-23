#include <atomic>
#include <functional>
#include <mutex>
#include <thread>
#include <unordered_set>
#include <vul_GUI.hpp>
#include <vul_camera.hpp>
#include <iostream>
#include <vul_descriptors.hpp>
#include <vul_rt_pipeline.hpp>
#include<host_device.hpp>
#include <vul_acceleration_structure.hpp>
#include <vul_pipeline.hpp>
#include <vul_renderer.hpp>
#include <vul_transform.hpp>
#include <vul_command_pool.hpp>

#include<imgui.h>
#include <memory>
#include <vulkan/vulkan_core.h>

void GuiStuff(double frameTime) {
    ImGui::Begin("Performance");
    ImGui::Text("Fps: %f\nTotal frame time: %fms", 1.0f / frameTime, frameTime * 1000.0f);
    ImGui::End();
}

std::unique_ptr<vul::VulDescriptorSet> createRtDescSet(const vul::Scene &scene, const std::vector<std::shared_ptr<vul::VulImage>> &imgs, const vul::VulAs &as,
        const std::unique_ptr<vul::VulImage> &rtImg, const std::unique_ptr<vul::VulBuffer> &ubo, const std::unique_ptr<vul::VulImage> &enviromentMap,
        const std::unique_ptr<vul::VulDescriptorPool> &descPool)
{
    std::vector<vul::VulDescriptorSet::Descriptor> descriptors;
    vul::VulDescriptorSet::Descriptor desc{};
    desc.count = 1;

    desc.type = vul::VulDescriptorSet::DescriptorType::storageImage;
    desc.stages = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_FRAGMENT_BIT;
    desc.content = rtImg.get();
    descriptors.push_back(desc);

    desc.type = vul::VulDescriptorSet::DescriptorType::uniformBuffer;
    desc.stages = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
    desc.content = ubo.get();
    descriptors.push_back(desc);

    desc.type = vul::VulDescriptorSet::DescriptorType::storageBuffer;
    desc.stages = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
    desc.content = scene.indexBuffer.get();
    descriptors.push_back(desc);
    desc.content = scene.vertexBuffer.get();
    descriptors.push_back(desc);

    desc.stages = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
    desc.content = scene.indexBuffer.get();
    desc.content = scene.normalBuffer.get();
    descriptors.push_back(desc);
    desc.content = scene.tangentBuffer.get();
    descriptors.push_back(desc);

    desc.stages = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
    desc.content = scene.indexBuffer.get();
    desc.content = scene.uvBuffer.get();
    descriptors.push_back(desc);
    desc.content = scene.materialBuffer.get();
    descriptors.push_back(desc);
    desc.content = scene.primInfoBuffer.get();
    descriptors.push_back(desc);

    desc.type = vul::VulDescriptorSet::DescriptorType::spCombinedImgSampler;
    desc.content = imgs.data();
    desc.count = imgs.size();
    descriptors.push_back(desc);

    desc.type = vul::VulDescriptorSet::DescriptorType::combinedImgSampler;
    desc.stages = VK_SHADER_STAGE_MISS_BIT_KHR;
    desc.content = enviromentMap.get();
    desc.count = 1;
    descriptors.push_back(desc);

    desc.type = vul::VulDescriptorSet::DescriptorType::accelerationStructure;
    desc.stages = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
    desc.count = 1;
    desc.content = &as;
    descriptors.push_back(desc);

    return vul::VulDescriptorSet::createDescriptorSet(descriptors, *descPool);
}

std::unique_ptr<vul::VulPipeline> createPipeline(const vul::VulRenderer &vulRenderer, const std::array<std::unique_ptr<vul::VulDescriptorSet>, vul::VulSwapChain::MAX_FRAMES_IN_FLIGHT> &descSets, const vul::VulDevice &device)
{
    vul::VulPipeline::PipelineConfigInfo pipConf{};
    pipConf.attributeDescriptions = {{0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0}, {1, 1, VK_FORMAT_R32G32_SFLOAT, 0}};
    pipConf.bindingDescriptions = {{0, sizeof(glm::vec3), VK_VERTEX_INPUT_RATE_VERTEX}, {1, sizeof(glm::vec2), VK_VERTEX_INPUT_RATE_VERTEX}};
    pipConf.colorAttachmentFormats = {vulRenderer.getSwapChainColorFormat()};
    pipConf.depthAttachmentFormat = vulRenderer.getDepthFormat();
    pipConf.setLayouts = {descSets[0]->getLayout()->getDescriptorSetLayout()};
    pipConf.enableColorBlending = false;
    pipConf.cullMode = VK_CULL_MODE_NONE;

    return std::make_unique<vul::VulPipeline>(device, "raytrace.vert.spv", "raytrace.frag.spv", pipConf);
}

void resizeRtImgs(std::array<std::unique_ptr<vul::VulImage>, vul::VulSwapChain::MAX_FRAMES_IN_FLIGHT> &rtImgs,
        std::array<std::unique_ptr<vul::VulDescriptorSet>, vul::VulSwapChain::MAX_FRAMES_IN_FLIGHT> &descSets,
        const vul::VulDevice &vulDevice, VkExtent2D swapChainExtent, vul::VulCmdPool &cmdPool)
{
    VkCommandBuffer cmdBuf = cmdPool.getPrimaryCommandBuffer();
    for (size_t i = 0; i < rtImgs.size(); i++) {
        rtImgs[i] = std::make_unique<vul::VulImage>(vulDevice);
        rtImgs[i]->keepRegularRaw2d32bitRgbaEmpty(swapChainExtent.width, swapChainExtent.height);
        rtImgs[i]->createDefaultImage(vul::VulImage::ImageType::storage2d, cmdBuf);
    }
    cmdPool.submit(cmdBuf, true);
    
    for (size_t i = 0; i < rtImgs.size(); i++) {
        descSets[i]->descriptorInfos[0].imageInfos[0].imageView = rtImgs[i]->getImageView();
        descSets[i]->update();
    }
}

void updateUniformBuffer(const std::unique_ptr<vul::VulBuffer> &ubo, const vul::Scene &scene, const vul::VulCamera &camera, uint32_t swapChainHeight, float fovY)
{
    GlobalUbo uboData{};
    uboData.inverseViewMatrix = glm::inverse(camera.getView());
    uboData.inverseProjectionMatrix = glm::inverse(camera.getProjection());
    uboData.cameraPosition = glm::vec4(camera.pos, 0.0f);

    uboData.numLights = std::min(static_cast<int>(scene.lights.size()), MAX_LIGHTS);
    for (int i = 0; i < uboData.numLights; i++){
        uboData.lightPositions[i] = glm::vec4(scene.lights[i].position, scene.lights[i].range);
        uboData.lightColors[i] = glm::vec4(scene.lights[i].color, scene.lights[i].intensity);
    }
    uboData.ambientLightColor = glm::vec4(1.0f, 1.0f, 1.0f, 0.3f);

    // Pixel spread angle is from equation 30 from
    // https://media.contentapi.ea.com/content/dam/ea/seed/presentations/2019-ray-tracing-gems-chapter-20-akenine-moller-et-al.pdf
    uboData.pixelSpreadAngle = atan((2.0f * tan(fovY / 2.0f)) / static_cast<float>(swapChainHeight));
    uboData.padding1 = 69;
    uboData.padding2 = 420;

    ubo->writeData(&uboData, sizeof(GlobalUbo), 0, VK_NULL_HANDLE);
}

int main() {
    vul::VulWindow vulWindow(2560, 1440, "Vulkano");
    vul::VulDevice vulDevice(vulWindow, 1, false, true);
    vul::VulRenderer vulRenderer(vulWindow, vulDevice, nullptr);
    std::unique_ptr<vul::VulDescriptorPool> descPool = vul::VulDescriptorPool::Builder(vulDevice).setMaxSets(8).setPoolFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT).build();
    vul::VulCmdPool cmdPool(vul::VulCmdPool::QueueType::main, 0, 0, vulDevice);
    vul::VulCmdPool transferCmdPool(vul::VulCmdPool::QueueType::transfer, 0, 0, vulDevice);
    vul::VulCmdPool sideCmdPool(vul::VulCmdPool::QueueType::side, 0, 0, vulDevice, 0);
    vul::VulGUI vulGui(vulWindow.getGLFWwindow(), descPool->getDescriptorPoolReference(), vulRenderer, vulDevice, cmdPool);
    vul::VulCamera camera{};

    vul::Scene mainScene(vulDevice);
    vul::GltfLoader mainSceneLoader = mainScene.loadScene("../Models/sponza/sponza.gltf", {.primInfo = true, .enableAddressTaking = true, .enableUsageForAccelerationStructures = true}, cmdPool);
    vul::GltfLoader::AsyncImageLoadingInfo asyncImageLoadingInfo{};
    mainSceneLoader.importPartialTexturesAsync(asyncImageLoadingInfo, "../Models/sponza/", 6, vulDevice, transferCmdPool, sideCmdPool);

    vul::Scene fullScreenQuad(vulDevice);
    fullScreenQuad.loadPlanes({{{-1.0f, -1.0f, 0.5f}, {1.0f, -1.0f, 0.5f}, {-1.0f, 1.0f, 0.5f}, {1.0f, 1.0f, 0.5f}, 0}}, {}, {.normal = false, .tangent = false, .material = false}, cmdPool);

    VkCommandBuffer commandBuffer = cmdPool.getPrimaryCommandBuffer();
    std::unique_ptr<vul::VulImage> enviromentMap = vul::VulImage::createDefaultWholeImageAllInOne(vulDevice, "../enviromentMaps/sunsetCube.exr",
            {}, true, vul::VulImage::InputDataType::exrFile, vul::VulImage::ImageType::hdrCube, commandBuffer);
    cmdPool.submit(commandBuffer, true);

    std::vector<vul::VulAs::AsNode> asNodes(mainScene.nodes.size());
    for (size_t i = 0; i < mainScene.nodes.size(); i++) asNodes[i] = {.nodeIndex = static_cast<uint32_t>(i), .blasIndex = 0};
    vul::VulAs as(vulDevice);
    as.loadScene(mainScene, asNodes, {vul::VulAs::InstanceInfo{.blasIdx = 0, .customIndex = 0,
            .shaderBindingTableRecordOffset = 0, .transform = vul::transform3D{}.transformMat()}}, false, cmdPool);

    std::array<std::unique_ptr<vul::VulImage>, vul::VulSwapChain::MAX_FRAMES_IN_FLIGHT> rtImgs;
    std::array<std::unique_ptr<vul::VulBuffer>, vul::VulSwapChain::MAX_FRAMES_IN_FLIGHT> ubos;
    std::array<std::unique_ptr<vul::VulDescriptorSet>, vul::VulSwapChain::MAX_FRAMES_IN_FLIGHT> descSets;
    asyncImageLoadingInfo.pauseMutex.lock();
    commandBuffer = cmdPool.getPrimaryCommandBuffer();
    for (int i = 0; i < vul::VulSwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        rtImgs[i] = std::make_unique<vul::VulImage>(vulDevice);
        rtImgs[i]->keepRegularRaw2d32bitRgbaEmpty(vulRenderer.getSwapChainExtent().width, vulRenderer.getSwapChainExtent().height);
        rtImgs[i]->createDefaultImage(vul::VulImage::ImageType::storage2d, commandBuffer);

        ubos[i] = std::make_unique<vul::VulBuffer>(sizeof(GlobalUbo), 1, false, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, vulDevice);
        descSets[i] = createRtDescSet(mainScene, mainSceneLoader.images, as, rtImgs[i], ubos[i], enviromentMap, descPool);
    }
    cmdPool.submit(commandBuffer, true);
    asyncImageLoadingInfo.pauseMutex.unlock();

    std::unique_ptr<vul::VulPipeline> pipeline = createPipeline(vulRenderer, descSets, vulDevice);
    vul::VulRtPipeline rtPipeline(vulDevice, "raytrace.rgen.spv", {"raytrace.rmiss.spv", "raytraceShadow.rmiss.spv"},
            {"raytrace.rchit.spv"}, {"raytraceShadow.rahit.spv"}, {}, {{0, 0, -1}},
            {descSets[0]->getLayout()->getDescriptorSetLayout()}, cmdPool);

    double frameStartTime = glfwGetTime();
    uint32_t frameCounter = 0;
    while (!vulWindow.shouldClose()) {
        glfwPollEvents();
        commandBuffer = vulRenderer.beginFrame();
        vulGui.startFrame();
        if (commandBuffer == nullptr) continue;
        int frameIdx = vulRenderer.getFrameIndex();

        const double frameTime = glfwGetTime() - frameStartTime;
        frameStartTime = glfwGetTime();
        if (!camera.shouldHideGui()) GuiStuff(frameTime);

        camera.applyInputs(vulWindow.getGLFWwindow(), frameTime, vulRenderer.getSwapChainExtent().height);
        camera.setPerspectiveProjection(80.0f * (M_PI * 2.0f / 360.0f), vulRenderer.getAspectRatio(), 0.01f, 100.0f);
        camera.updateXYZ();
        updateUniformBuffer(ubos[frameIdx], mainScene, camera, vulRenderer.getSwapChainExtent().height, 80.0f * (M_PI * 2.0f / 360.0f));

        std::vector<VkDescriptorSet> vkDescSets = {descSets[(frameIdx + 1) % vul::VulSwapChain::MAX_FRAMES_IN_FLIGHT]->getSet()};
        rtPipeline.traceRays(vulRenderer.getSwapChainExtent().width, vulRenderer.getSwapChainExtent().height, 0, nullptr, vkDescSets, commandBuffer);

        vulRenderer.beginRendering(commandBuffer, {}, vul::VulRenderer::SwapChainImageMode::clearPreviousStoreCurrent, vul::VulRenderer::DepthImageMode::noDepthImage,
                glm::vec4(0.0, 0.0, 0.0, 1.0), {}, vulRenderer.getSwapChainExtent().width, vulRenderer.getSwapChainExtent().height);
        pipeline->draw(commandBuffer, vkDescSets, {fullScreenQuad.vertexBuffer->getBuffer(), fullScreenQuad.uvBuffer->getBuffer()}, fullScreenQuad.indexBuffer->getBuffer(),
                {vul::VulPipeline::DrawData{.indexCount = 6}});
        vulGui.endFrame(commandBuffer);
        vulRenderer.stopRendering(commandBuffer);

        vulRenderer.endFrame();
        if (vulRenderer.wasSwapChainRecreated()) resizeRtImgs(rtImgs, descSets, vulDevice, vulRenderer.getSwapChainExtent(), cmdPool);

        if (frameCounter % 200 == 0) {
            asyncImageLoadingInfo.pauseMutex.lock();
            vkQueueWaitIdle(vulDevice.mainQueue());
            for (size_t i = 0; i < descSets.size(); i++) {
                for (size_t j = 0; j < mainSceneLoader.images.size(); j++)
                    descSets[i]->descriptorInfos[9].imageInfos[j].imageView = mainSceneLoader.images[j]->getImageView();
                descSets[i]->update();
            }
            asyncImageLoadingInfo.oldVkImageStuff.clear();
            asyncImageLoadingInfo.pauseMutex.unlock();
        }
        frameCounter++;
    }
    asyncImageLoadingInfo.stopLoadingImagesSignal = true;
    asyncImageLoadingInfo.asyncLoadingThread.join();
    vulDevice.waitForIdle();

    return 0;
}
