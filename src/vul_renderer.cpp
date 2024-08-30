#include "vul_command_pool.hpp"
#include "vul_image.hpp"
#include <vul_debug_tools.hpp>
#include <cstddef>
#include <cstdlib>
#include <memory>
#include<vul_renderer.hpp>

#include<imgui.h>
#include<imconfig.h>
#include<imgui_internal.h>
#include<imgui_impl_vulkan.h>
#include<imgui_impl_glfw.h>

#include<stdexcept>
#include<iostream>
#include <vulkan/vulkan_core.h>

namespace vul{

VulRenderer::VulRenderer(VulWindow &window, VulDevice &device, std::shared_ptr<vul::VulSampler> depthImgSampler) : vulWindow{window}, vulDevice{device}
{
    m_cmdPool = std::make_unique<VulCmdPool>(VulCmdPool::QueueType::main, 0, 0, device);
    m_depthImgSampler = depthImgSampler;
    recreateSwapChain();
    createCommandBuffers();
}

VulRenderer::~VulRenderer()
{
    vkFreeCommandBuffers(vulDevice.device(), m_cmdPool->getPool(), static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
}

void VulRenderer::recreateSwapChain()
{
    VUL_PROFILE_FUNC()
    auto extent = vulWindow.getExtent();
    while (extent.width == 0 || extent.height == 0) {
        extent = vulWindow.getExtent();
        glfwWaitEvents();
    }
    vkDeviceWaitIdle(vulDevice.device());

    if (vulSwapChain == nullptr) {
        vulSwapChain = std::make_unique<VulSwapChain>(vulDevice, extent);
    } else{
        std::shared_ptr<VulSwapChain> oldSwapChain = std::move(vulSwapChain);
        vulSwapChain = std::make_unique<VulSwapChain>(vulDevice, extent, oldSwapChain);

        if (!oldSwapChain->compareSwapFormats(*vulSwapChain.get())){
            throw std::runtime_error("Swap chain image format has changed");
        }
    }

    VkCommandBuffer cmdBuf = m_cmdPool->getPrimaryCommandBuffer();
    m_depthFormat = vulDevice.findSupportedFormat({  VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT}, 
                                                            VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    VkImageUsageFlags depthUsage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    if (m_depthImgSampler != nullptr) depthUsage |= VK_IMAGE_USAGE_SAMPLED_BIT;
    m_depthImages.resize(vulSwapChain->imageCount());
    for (size_t i = 0; i < vulSwapChain->imageCount(); i++) {
        m_depthImages[i] = std::make_unique<vul::VulImage>(vulDevice);
        m_depthImages[i]->keepEmpty(vulSwapChain->getSwapChainExtent().width, vulSwapChain->getSwapChainExtent().height, 1, 1, 1, m_depthFormat);
        m_depthImages[i]->createCustomImage(VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, depthUsage,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT, cmdBuf);
        m_depthImages[i]->vulSampler = m_depthImgSampler;
    }
    m_cmdPool->submit(cmdBuf, true);
}

void VulRenderer::createCommandBuffers() 
{
    commandBuffers.resize(VulSwapChain::MAX_FRAMES_IN_FLIGHT);
    
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_cmdPool->getPool();
    allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

    if (vkAllocateCommandBuffers(vulDevice.device(), &allocInfo, commandBuffers.data()) != VK_SUCCESS){
        throw std::runtime_error("Failed to allocate command buffers in vul_renderer.cpp file");
    }

    for (VkCommandBuffer rendererCmdBuf : commandBuffers) VUL_NAME_VK(rendererCmdBuf)
}

VkCommandBuffer VulRenderer::beginFrame()
{
    VUL_PROFILE_FUNC()
    assert(!isFrameStarted && "Can't call beginFrame while frame is already in progress");

    VkResult result = vulSwapChain->acquireNextImage(&currentImageIndex);

    m_swapchainRecreated = false;
    if (result == VK_ERROR_OUT_OF_DATE_KHR){
        recreateSwapChain();
        m_swapchainRecreated = true;
        return nullptr;
    }

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR){
        throw std::runtime_error("Failed to acquire swap chain image in vul_renderer.cpp file");
    }

    isFrameStarted = true;

   VkCommandBuffer commandBuffer = getCurrentCommandBuffer();
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    
    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS){
        throw std::runtime_error("Failed to begin recording command buffer in vul_renderer.cpp file");
    }

    return commandBuffer;
}

void VulRenderer::endFrame()
{
    VUL_PROFILE_FUNC()
    assert(isFrameStarted && "Can't call endFrame when frame hasn't even been started");
    VkCommandBuffer commandBuffer = getCurrentCommandBuffer();
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS){
        throw std::runtime_error("Failed to end commandBuffer in the vul_renderer.cpp file");
    }

    VkResult result = vulSwapChain->submitCommandBuffers(&commandBuffer, &currentImageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || vulWindow.wasWindowResized()){
        vulWindow.resetWindowResizedFlag();
        recreateSwapChain();
        m_swapchainRecreated = true;
    }

    else if (result != VK_SUCCESS){
        throw std::runtime_error("Failed to present swap chain image in vul_renderer.cpp file");
    }

    isFrameStarted = false;
    currentFrameIndex = (currentFrameIndex + 1) % VulSwapChain::MAX_FRAMES_IN_FLIGHT;
}

void VulRenderer::beginRendering(VkCommandBuffer commandBuffer, const std::vector<std::shared_ptr<VulImage>> &attachmentImages, SwapChainImageMode swapChainImageMode, DepthImageMode depthImageMode, const glm::vec4 &swapChainClearColor, float depthClearColor, uint32_t renderWidth, uint32_t renderHeight) const
{
    VUL_PROFILE_FUNC()
    assert(isFrameStarted && "Can't call beginRendering if the frame hasn't been started either");
    assert(commandBuffer == getCurrentCommandBuffer() && "Can't begin rendering on a command buffer from a different frame");

    VkExtent2D renderArea{};
    renderArea.width = renderWidth > 0 ? renderWidth : vulSwapChain->getSwapChainExtent().width;
    renderArea.height = renderHeight > 0 ? renderHeight : vulSwapChain->getSwapChainExtent().height;

    if (swapChainImageMode == SwapChainImageMode::preservePreviousStoreCurrent) vulSwapChain->getImage(currentImageIndex)->attachmentPreservePreviousContents = true;
    else vulSwapChain->getImage(currentImageIndex)->attachmentPreservePreviousContents = false;
    std::vector<VkRenderingAttachmentInfo> colorAttachmentInfos;
    if (swapChainImageMode != SwapChainImageMode::noSwapChainImage) colorAttachmentInfos.push_back(vulSwapChain->getImage(currentImageIndex)->getAttachmentInfo({{{swapChainClearColor.r, swapChainClearColor.g, swapChainClearColor.b, swapChainClearColor.a}}}));
    for (size_t i = 0; i < attachmentImages.size(); i++) colorAttachmentInfos.push_back(attachmentImages[i]->getAttachmentInfo({{{0.0f, 0.0f, 0.0f, 1.0f}}}));

    if (depthImageMode == DepthImageMode::clearPreviousStoreCurrent) {
        m_depthImages[currentImageIndex]->attachmentStoreCurrentContents = true;
        m_depthImages[currentImageIndex]->attachmentPreservePreviousContents = false;
    } else if (depthImageMode == DepthImageMode::clearPreviousDiscardCurrent) {
        m_depthImages[currentImageIndex]->attachmentStoreCurrentContents = false;
        m_depthImages[currentImageIndex]->attachmentPreservePreviousContents = false;
    } else if (depthImageMode == DepthImageMode::preservePreviousStoreCurrent) {
        m_depthImages[currentImageIndex]->attachmentStoreCurrentContents = true;
        m_depthImages[currentImageIndex]->attachmentPreservePreviousContents = true;
    }
    VkRenderingAttachmentInfo depthAttachmentInfo = m_depthImages[currentImageIndex]->getAttachmentInfo({{{depthClearColor}}});

    VkRenderingInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea.offset = {0, 0};
    renderingInfo.renderArea.extent = vulSwapChain->getSwapChainExtent();
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = static_cast<uint32_t>(colorAttachmentInfos.size());
    renderingInfo.pColorAttachments = colorAttachmentInfos.data();
    if (depthImageMode != DepthImageMode::noDepthImage) renderingInfo.pDepthAttachment = &depthAttachmentInfo;

    vulSwapChain->getImage(currentImageIndex)->transitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, commandBuffer);
    vkCmdBeginRendering(commandBuffer, &renderingInfo);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(renderArea.width);
    viewport.height = static_cast<float>(renderArea.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = renderArea;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void VulRenderer::stopRendering(VkCommandBuffer commandBuffer) const
{
    VUL_PROFILE_FUNC()
    assert(isFrameStarted && "Can't call stopRendering if the frame hasn't been started either");
    assert(commandBuffer == getCurrentCommandBuffer() && "Can't end a render pass on a command buffer from a different frame");

    vkCmdEndRendering(commandBuffer);
    vulSwapChain->getImage(currentImageIndex)->transitionImageLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, commandBuffer);
}

}
