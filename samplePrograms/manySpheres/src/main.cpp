#include "mesh_shading.hpp"
#include "rasterizing.hpp"
#include "vul_GUI.hpp"
#include "vul_command_pool.hpp"
#include "vul_descriptors.hpp"
#include "vul_device.hpp"
#include "vul_image.hpp"
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>
#include <host_device.hpp>
#include <raytracing.hpp>

#include<imgui.h>
#include <iostream>

enum class RenderingStyle {
    rasterizing,
    meshShading,
    rayTracing
};
void GuiStuff(double frameTime, RenderingStyle &renderingStyle) {
    ImGui::Begin("Menu");
    ImGui::Text("Fps: %f\nTotal frame time: %fms", 1.0f / frameTime, frameTime * 1000.0f);
    if (ImGui::Button("Rasterize")) renderingStyle = RenderingStyle::rasterizing;
    else if (ImGui::Button("Mesh shade")) renderingStyle = RenderingStyle::meshShading;
    else if (ImGui::Button("Ray trace")) renderingStyle = RenderingStyle::rayTracing;
    ImGui::End();
}

int main() {
    vul::VulWindow vulWindow(2560, 1440, "Vulkano");
    vul::VulDevice vulDevice(vulWindow, 0, true, true);
    std::shared_ptr<vul::VulSampler> depthImgSampler = vul::VulSampler::createDefaultTexSampler(vulDevice, 1);
    vul::VulRenderer vulRenderer(vulWindow, vulDevice, depthImgSampler);
    std::unique_ptr<vul::VulDescriptorPool> descPool = vul::VulDescriptorPool::Builder(vulDevice).setMaxSets(16).setPoolFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT).build();
    vul::VulCmdPool cmdPool(vul::VulCmdPool::QueueType::main, 0, 0, vulDevice);
    vul::VulGUI vulGui(vulWindow.getGLFWwindow(), descPool->getDescriptorPoolReference(), vulRenderer, vulDevice, cmdPool);
    vul::VulCamera camera{};
    camera.baseMoveSpeed *= 2.5f;
    camera.speedChanger *= 5.0;

    VkCommandBuffer commandBuffer = cmdPool.getPrimaryCommandBuffer();
    MeshResources meshRes = createMeshShadingResources(vulRenderer, *descPool.get(), commandBuffer, vulDevice);
    cmdPool.submit(commandBuffer, true);
    RtResources rtRes = createRaytracingResources(vulRenderer, *descPool.get(), cmdPool, vulDevice);
    RasResources rasRes = createRasterizationResources(vulRenderer, *descPool.get(), cmdPool, vulDevice);

    double frameStartTime = glfwGetTime();
    RenderingStyle renderingStyle = RenderingStyle::meshShading;
    while (!vulWindow.shouldClose()) {
        glfwPollEvents();
        commandBuffer = vulRenderer.beginFrame();
        vulGui.startFrame();
        if (commandBuffer == nullptr) continue;

        const double frameTime = glfwGetTime() - frameStartTime;
        frameStartTime = glfwGetTime();
        if (!camera.shouldHideGui()) GuiStuff(frameTime, renderingStyle);

        camera.applyInputs(vulWindow.getGLFWwindow(), frameTime, vulRenderer.getSwapChainExtent().height);
        camera.updateXYZ();
        camera.setPerspectiveProjection(80.0f / 360.0f * M_PI * 2.0f, vulRenderer.getAspectRatio(), 0.01f, 500.0f);

        if (renderingStyle == RenderingStyle::rasterizing) {
            updateRasUbo(camera, vulRenderer, rasRes);
            rasterize(rasRes, vulRenderer, commandBuffer);
        } else if (renderingStyle == RenderingStyle::meshShading) {
            updateMeshUbo(meshRes, camera, vulRenderer);
            meshShade(meshRes, vulRenderer, commandBuffer);
        } else {
            updateRtUbo(rtRes, vulRenderer, camera);
            raytrace(rtRes, vulRenderer, commandBuffer);
        }

        vulGui.endFrame(commandBuffer); 
        vulRenderer.stopRendering(commandBuffer);
        vulRenderer.endFrame();

        if (renderingStyle == RenderingStyle::rayTracing && (rtRes.rtImgs[0]->getBaseWidth() != vulRenderer.getSwapChainExtent().width ||
                    rtRes.rtImgs[0]->getBaseHeight() != vulRenderer.getSwapChainExtent().height)) resizeRtImgs(rtRes, vulRenderer, cmdPool, vulDevice);
    }
    vulDevice.waitForIdle();

    return 0;
}
