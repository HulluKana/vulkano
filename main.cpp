#include"vulkano/vulkano_defaults.hpp"

#include"3rdParty/imgui/imgui.h"

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

    bool stop = false;
    float ownStuffTime = 0.0f;
    while (!stop){
        VkCommandBuffer commandBuffer = vulkano.startFrame();
        double ownStuffStartTime = glfwGetTime();

        if (vulkano.shouldShowGUI()) GuiStuff(vulkano, ownStuffTime);

        ownStuffTime = glfwGetTime() - ownStuffStartTime;
        stop = vulkano.endFrame(commandBuffer);
    }

    vulkano.letVulkanoFinish();

    return 0;
}
