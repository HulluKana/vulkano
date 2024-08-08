#include <vul_debug_tools.hpp>
#include<vul_GUI.hpp>

#include<imgui.h>
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

VulGUI::VulGUI(GLFWwindow *window, VkDescriptorPool &descriptorPool, VulRenderer &vulRenderer, VulDevice &vulDevice, VulCmdPool &cmdPool)
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
    info.UseDynamicRendering = true;
    info.ColorAttachmentFormat = vulRenderer.getSwapChainColorFormat();
    info.DepthAttachmentFormat = vulRenderer.getDepthFormat();
    info.Queue = vulDevice.graphicsQueue();
    ImGui_ImplVulkan_Init(&info, nullptr);

    VkCommandBuffer commandBuffer = cmdPool.getPrimaryCommandBuffer();
    ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
    cmdPool.submitAndWait(commandBuffer);

    vkDeviceWaitIdle(vulDevice.device());
    ImGui_ImplVulkan_DestroyFontUploadObjects();
}

VulGUI::~VulGUI()
{
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void VulGUI::startFrame()
{
    VUL_PROFILE_FUNC()
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void VulGUI::endFrame(VkCommandBuffer &commandBuffer)
{
    VUL_PROFILE_FUNC()
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
}


}
