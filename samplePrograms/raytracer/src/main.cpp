#include "vul_image.hpp"
#include "vul_rt_pipeline.hpp"
#include<vulkano_program.hpp>
#include<host_device.hpp>
#include<vul_debug_tools.hpp>
#include <vul_acceleration_structure.hpp>

#include<imgui.h>
#include <iostream>

void GuiStuff(vul::Vulkano &vulkano, float ownStuffTime) {
    ImGui::Begin("Performance");
    ImGui::DragFloat("Max FPS", &vul::settings::maxFps, vul::settings::maxFps / 30.0f, 3.0f, 10'000.0f);
    ImGui::Text("Fps: %f\nTotal frame time: %fms\nIdle time %fms\nOwn stuff: %fms",
            1.0f / vulkano.getFrameTime(), vulkano.getFrameTime() * 1000.0f,
            vulkano.getIdleTime() * 1000.0f, ownStuffTime * 1000.0f);
    ImGui::End();
}

int main() {
    vul::settings::deviceInitConfig.enableRaytracingSupport = true;
    vul::Vulkano vulkano(2560, 1440, "Vulkano");
    vulkano.loadScene("../Models/Room.gltf");
    vulkano.initVulkano();
    vul::settings::maxFps = 60.0f;

    vul::VulAs as(vulkano.getVulDevice(), vulkano.scene);
    vul::VulImage rtImg(vulkano.getVulDevice());
    rtImg.keepEmpty(vulkano.getSwapChainExtent().width, vulkano.getSwapChainExtent().height, 4, 4);
    rtImg.createImageSingleTime(false, true, vul::VulImage::ImageType::storageFloat, 2);

    std::vector<vul::Vulkano::Descriptor> descriptors;
    vul::Vulkano::Descriptor asDesc{};
    asDesc.type = vul::Vulkano::DescriptorType::accelerationStructure;
    asDesc.stages = {vul::Vulkano::ShaderStage::rgen};
    asDesc.count = 1;
    asDesc.content = &as;
    descriptors.push_back(asDesc);

    vul::Vulkano::Descriptor rtImgDesc{};
    rtImgDesc.type = vul::Vulkano::DescriptorType::storageImage;
    rtImgDesc.stages = {vul::Vulkano::ShaderStage::rgen};
    rtImgDesc.count = 1;
    rtImgDesc.content = &rtImg;
    descriptors.push_back(rtImgDesc);

    vul::Vulkano::descSetReturnVal dsrv = vulkano.createDescriptorSet(descriptors);

    vul::VulRtPipeline rtPipeline(vulkano.getVulDevice(), {"../bin/raytrace.rgen.spv"}, {"../bin/raytrace.rmiss.spv"}, {"../bin/raytrace.rchit.spv"},
            {dsrv.set->getLayout()->getDescriptorSetLayout()});

    bool stop = false;
    float ownStuffTime = 0.0f;
    while (!stop) {
        VkCommandBuffer commandBuffer = vulkano.startFrame();
        if (commandBuffer == nullptr) continue;
        double ownStuffStartTime = glfwGetTime();

        if (vulkano.shouldShowGUI()) GuiStuff(vulkano, ownStuffTime);

        ownStuffTime = glfwGetTime() - ownStuffStartTime;
        stop = vulkano.endFrame(commandBuffer);
    }
    vulkano.letVulkanoFinish();

    return 0;
}
