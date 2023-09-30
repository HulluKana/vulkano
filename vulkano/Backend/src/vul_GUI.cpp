#include"../Headers/vul_GUI.hpp"

#include<imgui.h>
#include<imconfig.h>
#include<imgui_internal.h>
#include<imgui_impl_vulkan.h>
#include<imgui_impl_glfw.h>

namespace vul{

static void check_vk_result(VkResult err)
{
    if (err == 0)
        return;
    fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
    if (err < 0)
        abort();
}

void VulGUI::initImGui(GLFWwindow *window, VkDescriptorPool &descriptorPool, VulRenderer &vulRenderer, VulDevice &vulDevice)
{
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForVulkan(window, true);

    ImGui_ImplVulkan_InitInfo info;
    info.DescriptorPool = descriptorPool;
    info.Device = vulDevice.device();
    info.PhysicalDevice = vulDevice.getPhysicalDevice();
    info.ImageCount = VulSwapChain::MAX_FRAMES_IN_FLIGHT;
    info.MinImageCount = VulSwapChain::MAX_FRAMES_IN_FLIGHT;
    info.Instance = vulDevice.getInstace();
    info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    info.Subpass = 0;
    info.Allocator = nullptr;
    info.CheckVkResultFn = check_vk_result;
    info.PipelineCache = VK_NULL_HANDLE;
    ImGui_ImplVulkan_Init(&info, vulRenderer.getSwapChainRenderPass());

    VkCommandBuffer commandBuffer = vulDevice.beginSingleTimeCommands();
    ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
    vulDevice.endSingleTimeCommands(commandBuffer);

    vkDeviceWaitIdle(vulDevice.device());
    ImGui_ImplVulkan_DestroyFontUploadObjects();
}

void VulGUI::startFrame()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void VulGUI::endFrame(VkCommandBuffer &commandBuffer)
{
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
}

void VulGUI::destroyImGui()
{
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

}