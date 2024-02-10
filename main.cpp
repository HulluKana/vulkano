#include"vulkano/vulkano_defaults.hpp"
#include"vulkano/Backend/Headers/vul_comp_pipeline.hpp"

#include"3rdParty/imgui/imgui.h"
#include <vulkan/vulkan_core.h>

void GuiStuff(vul::Vulkano &vulkano, float ownStuffTime)
{   
    ImGui::Begin("Performance");
    ImGui::DragFloat("Max FPS", &vul::settings::maxFps, vul::settings::maxFps / 30.0f, 3.0f, 10'000.0f);
    ImGui::Text("Fps: %f\nTotal frame time: %fms\nObject render time: %fms\nGUI render time: %fms\nRender preparation time: %fms\nRender finishing time: %fms\nIdle time %fms\nOwn stuff: %fms",
                1.0f / vulkano.getFrameTime(), vulkano.getFrameTime() * 1000.0f, vulkano.getObjRenderTime() * 1000.0f, vulkano.getGuiRenderTime() * 1000.0f,
                vulkano.getRenderPreparationTime() * 1000.0f, vulkano.getRenderFinishingTime() * 1000.0f, vulkano.getIdleTime() * 1000.0f, ownStuffTime * 1000.0f);
    ImGui::End();
}

int main()
{
    vul::Vulkano vulkano(2560, 1440, "Vulkano");
    vulkano.loadScene("../Models/Room.glb");
    vul::defaults::createDefaultDescriptors(vulkano);
    vul::defaults::createDefault3dRenderSystem(vulkano);
    vulkano.initVulkano();

    vulB::VulCompPipeline comp("example.comp.spv", {vulkano.mainSetLayout->getDescriptorSetLayout()}, vulkano.getVulDevice());

    bool stop = false;
    float ownStuffTime = 0.0f;
    while (!stop){
        // ATTENTION!!! IMPORTANT!!!
        //
        // Begin single time commands uses graphics queue, but for this case it needs to use compute queue
        // Also its really slow
        //
        // So just come up with a better way to use the command buffers
        //
        // ATTENTION!!! IMPORTANT!!!
        VkCommandBuffer cmdBuf = vulkano.getVulDevice().beginSingleTimeCommands();
        comp.dispatch(1, 1, 1, {vulkano.mainDescriptorSets[0].getSet()}, cmdBuf);
        vulkano.getVulDevice().endSingleTimeCommands(cmdBuf);

        VkCommandBuffer commandBuffer = vulkano.startFrame();
        double ownStuffStartTime = glfwGetTime();

        if (vulkano.shouldShowGUI()) GuiStuff(vulkano, ownStuffTime);

        ownStuffTime = glfwGetTime() - ownStuffStartTime;
        stop = vulkano.endFrame(commandBuffer);
    }

    return 0;
}
