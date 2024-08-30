#include "host_device.hpp"
#include "vul_gltf_loader.hpp"
#include "vul_image.hpp"
#include "vul_swap_chain.hpp"
#include <vul_window.hpp>
#include <vul_device.hpp>
#include <vul_renderer.hpp>
#include <vul_descriptors.hpp>
#include <vul_GUI.hpp>
#include <vul_meshlet_scene.hpp>
#include <mesh_shading.hpp>

#include<imgui.h>
#include <iostream>
#include <vulkan/vulkan_core.h>

void GuiStuff(double frameTime) {
    ImGui::Begin("Menu");
    ImGui::Text("Fps: %f\nTotal frame time: %fms", 1.0f / frameTime, frameTime * 1000.0f);
    ImGui::End();
}

int main() {
    vul::VulWindow vulWindow(2560, 1440, "Vulkano");
    vul::VulDevice vulDevice(vulWindow, 1, true, false);
    std::shared_ptr<vul::VulSampler> depthImgSampler = vul::VulSampler::createDefaultTexSampler(vulDevice);
    vul::VulRenderer vulRenderer(vulWindow, vulDevice, depthImgSampler);
    std::unique_ptr<vul::VulDescriptorPool> descPool = vul::VulDescriptorPool::Builder(vulDevice)
        .setMaxSets(16).setPoolFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT).build();
    vul::VulCmdPool cmdPool(vul::VulCmdPool::QueueType::main, 0, 0, vulDevice);
    vul::VulCmdPool sideCmdPool(vul::VulCmdPool::QueueType::side, 0, 0, vulDevice, 0);
    vul::VulCmdPool transferCmdPool(vul::VulCmdPool::QueueType::transfer, 0, 0, vulDevice);
    vul::VulGUI vulGui(vulWindow.getGLFWwindow(), descPool->getDescriptorPoolReference(), vulRenderer, vulDevice, cmdPool);
    vul::VulCamera camera{};
    vul::VulMeshletScene scene;
    std::unique_ptr<vul::GltfLoader::AsyncImageLoadingInfo> asyncImageLoadingInfo = scene.loadGltfAsync(
            "../Models/sponza/sponza.gltf", "../Models/sponza/", MAX_MESHLET_TRIANGLES, MAX_MESHLET_VERTICES,
            MESHLETS_PER_TASK_SHADER, 6, {}, cmdPool, transferCmdPool, sideCmdPool, vulDevice); vul::Scene cube(vulDevice);
    cube.loadCubes({vul::Scene::Cube{.centerPos = glm::vec3(0.0f), .dimensions = glm::vec3(1.0f), .matIdx = 0}}, {},
            {.normal = false, .tangent = false, .uv = false, .material = false}, cmdPool);

    VkCommandBuffer commandBuffer = cmdPool.getPrimaryCommandBuffer();
    vul::VulImage cubeMap(vulDevice);
    /*
    cubeMap.loadCubemapFromEXR( "../enviromentMaps/sunsetCube.exr");
    cubeMap.createDefaultImage(vul::VulImage::ImageType::hdrCube, commandBuffer);
    */
    cubeMap.keepEmpty(1024, 1024, 1, 1, 6, VK_FORMAT_D32_SFLOAT);
    cubeMap.createCustomImage(VK_IMAGE_VIEW_TYPE_CUBE, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT, commandBuffer);
    cubeMap.vulSampler = vul::VulSampler::createDefaultTexSampler(vulDevice);
    asyncImageLoadingInfo->pauseMutex.lock();
    MeshResources meshRes = createMeshShadingResources(scene, cubeMap, vulRenderer, *descPool.get(), commandBuffer, vulDevice);
    asyncImageLoadingInfo->pauseMutex.unlock();
    cmdPool.submit(commandBuffer, true);

    double frameStartTime = glfwGetTime();
    bool imagesFullyLoaded = false;
    while (!vulWindow.shouldClose()) {
        if (!imagesFullyLoaded && asyncImageLoadingInfo->fullyProcessedImageCount >= scene.images.size()) {
            vkQueueWaitIdle(vulDevice.mainQueue());
            for (size_t i = 0; i < meshRes.descSets.size(); i++) {
                for (size_t j = 0; j < scene.images.size(); j++)
                    meshRes.descSets[i]->descriptorInfos[1].imageInfos[j] = scene.images[j]->getDescriptorInfo();
                meshRes.descSets[i]->update();
            }
            asyncImageLoadingInfo->oldVkImageStuff.clear();
        }

        glfwPollEvents();
        commandBuffer = vulRenderer.beginFrame();
        if (commandBuffer == nullptr) continue;
        vulGui.startFrame();

        const double frameTime = glfwGetTime() - frameStartTime;
        frameStartTime = glfwGetTime();
        if (!camera.shouldHideGui()) {
            GuiStuff(frameTime);
            ImGui::Begin("Debug");
            ImGui::Text("Pos: (%f, %f, %f)\nRot: (%f, %f, %f)", camera.pos.x, camera.pos.y, camera.pos.z, camera.rot.x, camera.rot.y, camera.rot.z);
            ImGui::End();
        }

        camera.applyInputs(vulWindow.getGLFWwindow(), frameTime, vulRenderer.getSwapChainExtent().height);
        camera.updateXYZ();
        camera.setPerspectiveProjection(80.0f / 360.0f * M_PI * 2.0f, vulRenderer.getAspectRatio(), 0.01f, 500.0f);

        constexpr glm::vec4 ambientLightColor(0.529f, 0.808f, 0.922f, 1.0f);
        updateMeshUbo(meshRes, scene, camera, vulRenderer, ambientLightColor);
        meshShade(meshRes, scene, cube, cubeMap, camera, vulRenderer, ambientLightColor, commandBuffer);

        vulGui.endFrame(commandBuffer); 
        vulRenderer.stopRendering(commandBuffer);
        vulRenderer.endFrame();
    }
    vulDevice.waitForIdle();

    return 0;
}
