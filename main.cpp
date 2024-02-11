#include "vulkano/Backend/Headers/vul_comp_pipeline.hpp"
#include "vulkano/Backend/Headers/vul_settings.hpp"
#include "vulkano/vulkano_program.hpp"
#include "vulkano/vulkano_random.hpp"

#include "3rdParty/imgui/imgui.h"
#include <GLFW/glfw3.h>
#include <stdexcept>

float maxFps = 60.0f;
void GuiStuff(vul::Vulkano &vulkano, float ownStuffTime) {
    ImGui::Begin("Performance");
    ImGui::DragFloat("Max FPS", &maxFps, maxFps / 30.0f, 3.0f, 10'000.0f);
    ImGui::Text("Fps: %f\nTotal frame time: %fms\nObject render time: %fms\nGUI "
            "render time: %fms\nRender preparation time: %fms\nRender "
            "finishing time: %fms\nIdle time %fms\nOwn stuff: %fms",
            1.0f / vulkano.getFrameTime(), vulkano.getFrameTime() * 1000.0f,
            vulkano.getObjRenderTime() * 1000.0f,
            vulkano.getGuiRenderTime() * 1000.0f,
            vulkano.getRenderPreparationTime() * 1000.0f,
            vulkano.getRenderFinishingTime() * 1000.0f,
            vulkano.getIdleTime() * 1000.0f, ownStuffTime * 1000.0f);
    ImGui::End();
}

int main() {
    vul::Vulkano vulkano(2560, 1440, "Vulkano");

    struct SquarePc {
        int index;
    };
    std::array<SquarePc, 320> squarePCs;
    for (int i = 0; i < 320; i++) {
        vulkano.createSquare(0.0f, 0.0f, 1.0f, 1.0f);
        squarePCs[i].index = i;
        vulkano.object2Ds[i].pCustomPushData = &squarePCs[i];
        vulkano.object2Ds[i].customPushDataSize = sizeof(SquarePc);
    }
    vul::settings::batchRender2Ds = true;

    vulB::VulBuffer squareBuffer(
            vulkano.getVulDevice(), 7 * sizeof(float) + sizeof(uint32_t), 320,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vul::Vulkano::Descriptor descriptor{};
    descriptor.type = vul::Vulkano::DescriptorType::ssbo;
    descriptor.stages = {vul::Vulkano::ShaderStage::vert,
        vul::Vulkano::ShaderStage::frag,
        vul::Vulkano::ShaderStage::comp};
    descriptor.content = &squareBuffer;
    descriptor.count = 1;
    for (int i = 0; i < vulB::VulSwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        vul::Vulkano::descSetReturnVal descRetVal =
            vulkano.createDescriptorSet({descriptor});
        if (!descRetVal.succeeded)
            throw std::runtime_error("Failed to create descriptor set");
        vulkano.mainSetLayout = std::move(descRetVal.layout);
        vulkano.mainDescriptorSets.push_back(std::move(descRetVal.set));
    }

    std::unique_ptr<vul::RenderSystem> renderSystem =
        vulkano.createNewRenderSystem(
                {vulkano.mainSetLayout->getDescriptorSetLayout()}, "example.vert.spv",
                "example.frag.spv", true);
    vulkano.renderSystem2D = std::move(renderSystem);

    vulkano.initVulkano();
    vul::settings::maxFps = 10000.0f;

    vulB::VulCompPipeline comp("example.comp.spv",
            {vulkano.mainSetLayout->getDescriptorSetLayout()},
            vulkano.getVulDevice());

    bool stop = false;
    float ownStuffTime = 0.0f;
    double lastTime = glfwGetTime();
    while (!stop) {
        double newTime = glfwGetTime();
        while (1) {
            if (1.0 / (newTime - lastTime ) < maxFps)
                break;
            newTime = glfwGetTime();
        }
        lastTime = newTime;
        glm::vec3 color = glm::vec3(vul::random::floatNormalized(),
                vul::random::floatNormalized(),
                vul::random::floatNormalized());
        comp.pPushData = &color;
        comp.pushSize = sizeof(color);
        comp.dispatch(10, 1, 1, {vulkano.mainDescriptorSets[0].getSet()});

        VkCommandBuffer commandBuffer = vulkano.startFrame();
        double ownStuffStartTime = glfwGetTime();

        if (vulkano.shouldShowGUI())
            GuiStuff(vulkano, ownStuffTime);

        ownStuffTime = glfwGetTime() - ownStuffStartTime;
        stop = vulkano.endFrame(commandBuffer);
    }

    vulkano.letVulkanoFinish();
    for (int i = 0; i < vulB::VulSwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        vulkano.mainDescriptorSets[i].free();
    }

    return 0;
}
