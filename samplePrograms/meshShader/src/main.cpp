#include "mesh_shading.hpp"
#include "vul_GUI.hpp"
#include "vul_command_pool.hpp"
#include "vul_descriptors.hpp"
#include "vul_device.hpp"
#include "vul_image.hpp"
#include <GLFW/glfw3.h>
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
    vul::VulDevice vulDevice(vulWindow, true, false);
    std::shared_ptr<vul::VulSampler> depthImgSampler = vul::VulSampler::createDefaultTexSampler(vulDevice, 1);
    vul::VulRenderer vulRenderer(vulWindow, vulDevice, depthImgSampler);
    std::unique_ptr<vul::VulDescriptorPool> descPool = vul::VulDescriptorPool::Builder(vulDevice).setMaxSets(16).setPoolFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT).build();
    vul::VulCmdPool cmdPool(vul::VulCmdPool::QueueFamilyType::GraphicsFamily, vulDevice.graphicsQueue(), 0, 0, vulDevice);
    vul::VulGUI vulGui(vulWindow.getGLFWwindow(), descPool->getDescriptorPoolReference(), vulRenderer, vulDevice, cmdPool);
    vul::Scene scene(vulDevice);
    scene.loadScene("../Models/sponza/sponza.gltf", "../Models/sponza", {}, cmdPool);
    vul::VulCamera camera{};

    VkCommandBuffer commandBuffer = cmdPool.getPrimaryCommandBuffer();
    MeshResources meshRes = createMeshShadingResources(scene, vulRenderer, *descPool.get(), commandBuffer, vulDevice);
    cmdPool.submitAndWait(commandBuffer);

    double frameStartTime = glfwGetTime();
    while (!vulWindow.shouldClose()) {
        glfwPollEvents();
        commandBuffer = vulRenderer.beginFrame();
        vulGui.startFrame();
        if (commandBuffer == nullptr) continue;

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
