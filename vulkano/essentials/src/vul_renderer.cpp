#include "vul_attachment_image.hpp"
#include<vul_renderer.hpp>

#include<imgui.h>
#include<imconfig.h>
#include<imgui_internal.h>
#include<imgui_impl_vulkan.h>
#include<imgui_impl_glfw.h>

#include<stdexcept>
#include<iostream>
#include <vulkan/vulkan_core.h>

namespace vulB{

VulRenderer::VulRenderer(VulWindow &window, VulDevice &device) : vulWindow{window}, vulDevice{device}
{
    recreateSwapChain();
    createCommandBuffers();
}

VulRenderer::~VulRenderer()
{
    freeCommandBuffers();
}

void VulRenderer::recreateSwapChain()
{
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

    m_depthFormat = vulDevice.findSupportedFormat({  VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT}, 
                                                            VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    m_depthImages.resize(vulSwapChain->imageCount());
    for (size_t i = 0; i < vulSwapChain->imageCount(); i++) {
        m_depthImages[i] = std::make_unique<VulAttachmentImage>(vulDevice);
        if (m_depthImages[i]->createEmptyImage( VulAttachmentImage::ImageType::depthAttachment, m_depthFormat, vulSwapChain->getSwapChainExtent()) != VK_SUCCESS)
            throw std::runtime_error("Failed to create depth image");
    }
}

void VulRenderer::createCommandBuffers() 
{
    commandBuffers.resize(VulSwapChain::MAX_FRAMES_IN_FLIGHT);
    
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = vulDevice.getCommandPool();
    allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

    if (vkAllocateCommandBuffers(vulDevice.device(), &allocInfo, commandBuffers.data()) != VK_SUCCESS){
        throw std::runtime_error("Failed to allocate command buffers in vul_renderer.cpp file");
    }
}

void VulRenderer::freeCommandBuffers(){
    vkFreeCommandBuffers(vulDevice.device(), vulDevice.getCommandPool(), static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
    commandBuffers.clear();
}

VkCommandBuffer VulRenderer::beginFrame()
{
    assert(!isFrameStarted && "Can't call beginFrame while frame is already in progress");

    VkResult result = vulSwapChain->acquireNextImage(&currentImageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR){
        recreateSwapChain();
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
    assert(isFrameStarted && "Can't call endFrame when frame hasn't even been started");
    VkCommandBuffer commandBuffer = getCurrentCommandBuffer();
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS){
        throw std::runtime_error("Failed to end commandBuffer in the vul_renderer.cpp file");
    }

    VkResult result = vulSwapChain->submitCommandBuffers(&commandBuffer, &currentImageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || vulWindow.wasWindowResized()){
        vulWindow.resetWindowResizedFlag();
        recreateSwapChain();
    }

    else if (result != VK_SUCCESS){
        throw std::runtime_error("Failed to present swap chain image in vul_renderer.cpp file");
    }

    isFrameStarted = false;
    currentFrameIndex = (currentFrameIndex + 1) % VulSwapChain::MAX_FRAMES_IN_FLIGHT;
}

void VulRenderer::beginRendering(VkCommandBuffer commandBuffer, uint32_t renderWidth, uint32_t renderHeight)
{
    assert(isFrameStarted && "Can't call beginRendering if the frame hasn't been started either");
    assert(commandBuffer == getCurrentCommandBuffer() && "Can't begin rendering on a command buffer from a different frame");

    VkExtent2D renderArea{};
    renderArea.width = renderWidth > 0 ? renderWidth : vulSwapChain->getSwapChainExtent().width;
    renderArea.height = renderHeight > 0 ? renderHeight : vulSwapChain->getSwapChainExtent().height;

    VkRenderingAttachmentInfo colorAttachmentInfo = vulSwapChain->getImage(currentImageIndex)->getAttachmentInfo({{{0.0f, 0.0f, 0.0f, 1.0f}}});
    VkRenderingAttachmentInfo depthAttachmentInfo = m_depthImages[currentImageIndex]->getAttachmentInfo({{{1.0f}}});
    VkRenderingInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea.offset = {0, 0};
    renderingInfo.renderArea.extent = vulSwapChain->getSwapChainExtent();
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorAttachmentInfo;
    renderingInfo.pDepthAttachment = &depthAttachmentInfo;

    vulSwapChain->getImage(currentImageIndex)->establishPreAttachmentPipelineBarrier(commandBuffer);
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

void VulRenderer::stopRendering(VkCommandBuffer commandBuffer)
{
    assert(isFrameStarted && "Can't call stopRendering if the frame hasn't been started either");
    assert(commandBuffer == getCurrentCommandBuffer() && "Can't end a render pass on a command buffer from a different frame");

    vkCmdEndRendering(commandBuffer);
    vulSwapChain->getImage(currentImageIndex)->establishPostAttachmentPipelineBarrier(commandBuffer);
}

}
