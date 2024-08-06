#include <vul_2d_object.hpp>
#include <vul_GUI.hpp>
#include <vul_camera.hpp>
#include <vul_movement_controller.hpp>
#include <iostream>
#include <vul_descriptors.hpp>
#include <vul_settings.hpp>
#include <vul_rt_pipeline.hpp>
#include<host_device.hpp>
#include <vul_acceleration_structure.hpp>
#include <vul_pipeline.hpp>
#include <vul_renderer.hpp>

#include<imgui.h>
#include <memory>

void GuiStuff(double frameTime) {
    ImGui::Begin("Performance");
    ImGui::DragFloat("Max FPS", &vul::settings::maxFps, vul::settings::maxFps / 30.0f, 3.0f, 10'000.0f);
    ImGui::Text("Fps: %f\nTotal frame time: %fms",
            1.0f / frameTime, frameTime * 1000.0f);
    ImGui::End();
}

std::unique_ptr<vul::VulDescriptorSet> createRtDescSet(const vul::Scene &scene, const vul::VulAs &as, const std::unique_ptr<vul::VulImage> &rtImg,
        const std::unique_ptr<vul::VulBuffer> &ubo, const std::unique_ptr<vul::VulImage> &enviromentMap, const std::unique_ptr<vul::VulDescriptorPool> &descPool)
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
    desc.content = scene.images.data();
    desc.count = scene.images.size();
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
    pipConf.attributeDescriptions = {{0, 0, VK_FORMAT_R32G32_SFLOAT, 0}, {1, 1, VK_FORMAT_R32G32_SFLOAT, 0}};
    pipConf.bindingDescriptions = {{0, sizeof(glm::vec2), VK_VERTEX_INPUT_RATE_VERTEX}, {1, sizeof(glm::vec2), VK_VERTEX_INPUT_RATE_VERTEX}};
    pipConf.colorAttachmentFormats = {vulRenderer.getSwapChainColorFormat()};
    pipConf.depthAttachmentFormat = vulRenderer.getDepthFormat();
    pipConf.setLayouts = {descSets[0]->getLayout()->getDescriptorSetLayout()};
    pipConf.enableColorBlending = false;
    pipConf.cullMode = VK_CULL_MODE_NONE;

    return std::make_unique<vul::VulPipeline>(device, "../bin/raytrace.vert.spv", "../bin/raytrace.frag.spv", pipConf);
}

void resizeRtImgs(std::array<std::unique_ptr<vul::VulImage>, vul::VulSwapChain::MAX_FRAMES_IN_FLIGHT> &rtImgs,
        std::array<std::unique_ptr<vul::VulDescriptorSet>, vul::VulSwapChain::MAX_FRAMES_IN_FLIGHT> &descSets, const vul::VulDevice &vulDevice, VkExtent2D swapChainExtent)
{
    VkCommandBuffer cmdBuf = vulDevice.beginSingleTimeCommands();
    for (size_t i = 0; i < rtImgs.size(); i++) {
        rtImgs[i] = std::make_unique<vul::VulImage>(vulDevice);
        rtImgs[i]->keepRegularRaw2d32bitRgbaEmpty(swapChainExtent.width, swapChainExtent.height);
        rtImgs[i]->createDefaultImage(vul::VulImage::ImageType::storage2d, cmdBuf);
    }
    vulDevice.endSingleTimeCommands(cmdBuf);
    
    for (size_t i = 0; i < rtImgs.size(); i++) {
        descSets[i]->descriptorInfos[0].imageInfos[0].imageView = rtImgs[i]->getImageView();
        descSets[i]->update();
    }
}

void updateUniformBuffer(const std::unique_ptr<vul::VulBuffer> &ubo, const vul::Scene &scene, const vul::VulCamera &camera, const glm::vec3 &camPos, uint32_t swapChainHeight)
{
    GlobalUbo uboData{};
    uboData.inverseViewMatrix = glm::inverse(camera.getView());
    uboData.inverseProjectionMatrix = glm::inverse(camera.getProjection());
    uboData.cameraPosition = glm::vec4(camPos, 0.0f);

    uboData.numLights = std::min(static_cast<int>(scene.lights.size()), MAX_LIGHTS);
    for (int i = 0; i < uboData.numLights; i++){
        uboData.lightPositions[i] = glm::vec4(scene.lights[i].position, scene.lights[i].range);
        uboData.lightColors[i] = glm::vec4(scene.lights[i].color, scene.lights[i].intensity);
    }
    uboData.ambientLightColor = glm::vec4(1.0f, 1.0f, 1.0f, 0.3f);

    // Pixel spread angle is from equation 30 from
    // https://media.contentapi.ea.com/content/dam/ea/seed/presentations/2019-ray-tracing-gems-chapter-20-akenine-moller-et-al.pdf
    uboData.pixelSpreadAngle = atan((2.0f * tan(vul::settings::cameraProperties.fovY / 2.0f)) / static_cast<float>(swapChainHeight));
    uboData.padding1 = 69;
    uboData.padding2 = 420;

    ubo->writeData(&uboData, sizeof(GlobalUbo), 0);
}

int main() {
    vul::settings::deviceInitConfig.enableRaytracingSupport = true;
    vul::VulWindow vulWindow(2560, 1440, "Vulkano");
    vul::VulDevice vulDevice(vulWindow);
    vul::VulRenderer vulRenderer(vulWindow, vulDevice);
    std::unique_ptr<vul::VulDescriptorPool> descPool = vul::VulDescriptorPool::Builder(vulDevice).setMaxSets(8).setPoolFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT).build();
    vul::Scene scene(vulDevice);
    scene.loadScene("../Models/sponza/sponza.gltf", "../Models/sponza/", {});
    vul::Object2D fullScreenQuad;
    fullScreenQuad.addSquare(vulDevice, 0.0f, 0.0f, 1.0f, 1.0f);
    vul::settings::maxFps = 60.0f;

    std::unique_ptr<vul::VulImage> enviromentMap = vul::VulImage::createDefaultWholeImageAllInOneSingleTime(vulDevice,
            "../enviromentMaps/sunsetCube.exr", {}, true, vul::VulImage::InputDataType::exrFile, vul::VulImage::ImageType::hdrCube);

    std::vector<vul::VulAs::AsNode> asNodes(scene.nodes.size());
    for (size_t i = 0; i < scene.nodes.size(); i++) asNodes[i] = {.nodeIndex = static_cast<uint32_t>(i), .blasIndex = 0};
    vul::VulAs as(vulDevice);
    as.loadScene(scene, asNodes, {vul::VulAs::InstanceInfo{.blasIdx = 0, .customIndex = 0,
            .shaderBindingTableRecordOffset = 0, .transform = vul::transform3D{}.transformMat()}}, false);

    std::array<std::unique_ptr<vul::VulImage>, vul::VulSwapChain::MAX_FRAMES_IN_FLIGHT> rtImgs;
    std::array<std::unique_ptr<vul::VulBuffer>, vul::VulSwapChain::MAX_FRAMES_IN_FLIGHT> ubos;
    std::array<std::unique_ptr<vul::VulDescriptorSet>, vul::VulSwapChain::MAX_FRAMES_IN_FLIGHT> descSets;
    for (int i = 0; i < vul::VulSwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        rtImgs[i] = std::make_unique<vul::VulImage>(vulDevice);
        rtImgs[i]->keepRegularRaw2d32bitRgbaEmpty(vulRenderer.getSwapChainExtent().width, vulRenderer.getSwapChainExtent().height);
        VkCommandBuffer cmdBuf = vulDevice.beginSingleTimeCommands();
        rtImgs[i]->createDefaultImage(vul::VulImage::ImageType::storage2d, cmdBuf);
        vulDevice.endSingleTimeCommands(cmdBuf);

        ubos[i] = std::make_unique<vul::VulBuffer>(vulDevice);
        ubos[i]->keepEmpty(sizeof(GlobalUbo), 1);
        ubos[i]->createBuffer(false, vul::VulBuffer::usage_ubo);
        descSets[i] = createRtDescSet(scene, as, rtImgs[i], ubos[i], enviromentMap, descPool);
    }

    std::unique_ptr<vul::VulPipeline> pipeline = createPipeline(vulRenderer, descSets, vulDevice);
    vul::VulRtPipeline rtPipeline(vulDevice, "../bin/raytrace.rgen.spv", {"../bin/raytrace.rmiss.spv", "../bin/raytraceShadow.rmiss.spv"},
            {"../bin/raytrace.rchit.spv"}, {"../bin/raytraceShadow.rahit.spv"}, {}, {{0, 0, -1}},
            {descSets[0]->getLayout()->getDescriptorSetLayout()});

    vul::VulGUI vulGui;
    vulGui.initImGui(vulWindow.getGLFWwindow(), descPool->getDescriptorPoolReference(), vulRenderer, vulDevice);
    vul::VulCamera camera;
    camera.setPerspectiveProjection(vul::settings::cameraProperties.fovY, vulRenderer.getAspectRatio(), vul::settings::cameraProperties.nearPlane, vul::settings::cameraProperties.farPlane);
    vul::transform3D cameraTransform{};
    vul::MovementController movementController{};

    double frameStartTime = glfwGetTime();
    while (!vulWindow.shouldClose()) {
        glfwPollEvents();
        VkCommandBuffer commandBuffer = vulRenderer.beginFrame();
        vulGui.startFrame();
        if (commandBuffer == nullptr) continue;
        int frameIdx = vulRenderer.getFrameIndex();

        const double frameTime = glfwGetTime() - frameStartTime;
        frameStartTime = glfwGetTime();
        if (!movementController.hideGUI) GuiStuff(frameTime);

        movementController.modifyValues(vulWindow.getGLFWwindow(), cameraTransform);
        movementController.rotate(vulWindow.getGLFWwindow(), frameTime, cameraTransform, vulRenderer.getSwapChainExtent().width, vulRenderer.getSwapChainExtent().height);
        movementController.move(vulWindow.getGLFWwindow(), frameTime, cameraTransform);
        camera.setPerspectiveProjection(vul::settings::cameraProperties.fovY, vulRenderer.getAspectRatio(), vul::settings::cameraProperties.nearPlane, vul::settings::cameraProperties.farPlane);
        camera.setViewXYZ(cameraTransform.pos, cameraTransform.rot);
        updateUniformBuffer(ubos[frameIdx], scene, camera, cameraTransform.pos, vulRenderer.getSwapChainExtent().height);

        std::vector<VkDescriptorSet> vkDescSets = {descSets[(frameIdx + 1) % vul::VulSwapChain::MAX_FRAMES_IN_FLIGHT]->getSet()};
        rtPipeline.traceRays(vulRenderer.getSwapChainExtent().width, vulRenderer.getSwapChainExtent().height, 0, nullptr, vkDescSets, commandBuffer);

        vulRenderer.beginRendering(commandBuffer, {}, vul::VulRenderer::SwapChainImageMode::clearPreviousStoreCurrent, vul::VulRenderer::DepthImageMode::noDepthImage,
                glm::vec4(0.0, 0.0, 0.0, 1.0), {}, vulRenderer.getSwapChainExtent().width, vulRenderer.getSwapChainExtent().height);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->getPipelineLayout(), 0, 1, vkDescSets.data(), 0, nullptr);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->getPipeline());
        fullScreenQuad.bind(commandBuffer);
        fullScreenQuad.draw(commandBuffer);

        vulGui.endFrame(commandBuffer);
        vulRenderer.stopRendering(commandBuffer);

        vulRenderer.endFrame();
        if (vulRenderer.wasSwapChainRecreated()) resizeRtImgs(rtImgs, descSets, vulDevice, vulRenderer.getSwapChainExtent());
    }
    vulDevice.waitForIdle();
    vulGui.destroyImGui();

    return 0;
}
