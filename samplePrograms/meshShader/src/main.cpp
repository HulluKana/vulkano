#include "mesh_shading.hpp"
#include "vul_GUI.hpp"
#include "vul_command_pool.hpp"
#include "vul_descriptors.hpp"
#include "vul_device.hpp"
#include "vul_gltf_loader.hpp"
#include "vul_image.hpp"
#include <GLFW/glfw3.h>
#include <thread>
#include <vulkan/vulkan_core.h>
#include <host_device.hpp>

#include<imgui.h>
#include <iostream>

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
    std::unique_ptr<vul::VulDescriptorPool> descPool = vul::VulDescriptorPool::Builder(vulDevice).setMaxSets(16).setPoolFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT).build();
    vul::VulCmdPool cmdPool(vul::VulCmdPool::QueueType::main, 0, 0, vulDevice);
    vul::VulCmdPool sideCmdPool(vul::VulCmdPool::QueueType::side, 0, 0, vulDevice, 0);
    vul::VulCmdPool transferCmdPool(vul::VulCmdPool::QueueType::transfer, 0, 0, vulDevice);
    vul::VulGUI vulGui(vulWindow.getGLFWwindow(), descPool->getDescriptorPoolReference(), vulRenderer, vulDevice, cmdPool);
    vul::Scene scene(vulDevice);
    scene.loadSceneSync("../Models/sponza/sponza.gltf", "../Models/sponza/", {}, cmdPool);
    using namespace std::chrono_literals;
    vul::VulCamera camera{};

    VkCommandBuffer commandBuffer = cmdPool.getPrimaryCommandBuffer();
    MeshResources meshRes = createMeshShadingResources(scene, vulRenderer, *descPool.get(), commandBuffer, vulDevice);
    cmdPool.submit(commandBuffer, true);

    double frameStartTime = glfwGetTime();
    while (!vulWindow.shouldClose()) {
        glfwPollEvents();
        commandBuffer = vulRenderer.beginFrame();
        if (commandBuffer == nullptr) continue;
        vulGui.startFrame();

        const double frameTime = glfwGetTime() - frameStartTime;
        frameStartTime = glfwGetTime();
        if (!camera.shouldHideGui()) GuiStuff(frameTime);

        camera.applyInputs(vulWindow.getGLFWwindow(), frameTime, vulRenderer.getSwapChainExtent().height);
        camera.updateXYZ();
        camera.setPerspectiveProjection(80.0f / 360.0f * M_PI * 2.0f, vulRenderer.getAspectRatio(), 0.01f, 500.0f);

        constexpr glm::vec4 ambientLightColor(0.529f, 0.808f, 0.922f, 1.0f);
        updateMeshUbo(meshRes, scene, camera, vulRenderer, ambientLightColor);
        meshShade(meshRes, scene, vulRenderer, ambientLightColor, commandBuffer);

        vulGui.endFrame(commandBuffer); 
        vulRenderer.stopRendering(commandBuffer);
        vulRenderer.endFrame();
    }
    vulDevice.waitForIdle();

    return 0;
}
