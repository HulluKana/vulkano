#include "mesh_shading.hpp"
#include "vul_settings.hpp"
#include <vulkan/vulkan_core.h>
#include<vulkano_program.hpp>
#include <host_device.hpp>
#include <raytracing.hpp>

#include<imgui.h>

void GuiStuff(vul::Vulkano &vulkano, uint32_t cullCount) {
    ImGui::Begin("Performance");
    ImGui::DragFloat("Max FPS", &vul::settings::maxFps, vul::settings::maxFps / 30.0f, 3.0f, 10'000.0f);
    ImGui::Text("Fps: %f\nTotal frame time: %fms\nIdle time %fms",
            1.0f / vulkano.getFrameTime(), vulkano.getFrameTime() * 1000.0f,
            vulkano.getIdleTime() * 1000.0f);
    ImGui::Text("Cull count: %u\nTotal count: %d\nCull ratio: %f", cullCount, VOLUME_VOLUME, static_cast<float>(cullCount) / static_cast<float>(VOLUME_VOLUME));
    ImGui::End();

}

int main() {
    vul::settings::deviceInitConfig.enableMeshShaderSupport = true;
    vul::settings::rendererConfig.enableSamplingDepthImages = true;
    vul::Vulkano vulkano(2560, 1440, "Vulkano");

    vul::settings::rendererConfig.depthImageSampler = vul::VulSampler::createCustomSampler(vulkano.getVulDevice(), VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT,
                0.0f, VK_BORDER_COLOR_INT_OPAQUE_BLACK, VK_SAMPLER_MIPMAP_MODE_LINEAR, true, VK_SAMPLER_REDUCTION_MODE_MAX, 0.0f, 0.0f, 12.0f);
    for (const std::unique_ptr<vul::VulImage> &depthImg : vulkano.vulRenderer.getDepthImages()) depthImg->vulSampler = vul::settings::rendererConfig.depthImageSampler;

    vul::settings::maxFps = 60.0f;
    vul::settings::cameraProperties.farPlane = 500.0f;

    vulkano.cameraController.baseMoveSpeed *= 2.5f;
    vulkano.cameraController.speedChanger *= 5.0f;

    MeshResources meshRes = createMeshShadingResources(vulkano);


    bool stop = false;
    uint32_t prevImgIdx = vulkano.vulRenderer.getImageIndex();
    while (!stop) {
        VkCommandBuffer commandBuffer = vulkano.startFrame();
        if (commandBuffer == nullptr) continue;

        updateMeshUbo(vulkano, meshRes, prevImgIdx);
        meshShade(vulkano, meshRes, commandBuffer);
        prevImgIdx = vulkano.vulRenderer.getImageIndex();

        uint32_t cullCount;
        meshRes.cullCounters[vulkano.vulRenderer.getFrameIndex()]->readData(&cullCount, sizeof(cullCount), 0);
        meshRes.cullCounters[vulkano.vulRenderer.getFrameIndex()]->writeVector(std::vector<uint32_t>{0}, 0);
        cullCount *= CUBES_PER_MESH;
        if (vulkano.shouldShowGUI()) GuiStuff(vulkano, cullCount);

        stop = vulkano.endFrame(commandBuffer);
        if (vulkano.vulRenderer.wasSwapChainRecreated()) resizeUsableDepthImgs(vulkano, meshRes);
    }
    vul::settings::rendererConfig.depthImageSampler = nullptr;
    vulkano.letVulkanoFinish();

    return 0;
}
