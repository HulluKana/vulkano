#include"../Headers/vul_renderer.hpp"

#include<imgui.h>
#include<imconfig.h>
#include<imgui_internal.h>
#include<imgui_impl_vulkan.h>
#include<imgui_impl_glfw.h>

#include<stdexcept>
#include<iostream>

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
            throw std::runtime_error("Swap chain image or depth format has changed");
        }
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

void VulRenderer::beginSwapChainRenderPass(VkCommandBuffer commandBuffer)
{
    assert(isFrameStarted && "Can't call beginSwapChainRenderPass if the frame hasn't been started either");
    assert(commandBuffer == getCurrentCommandBuffer() && "Can't begin a render pass on a command buffer from a different frame");

    VkImageSubresourceRange subResourceRange;
    subResourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subResourceRange.baseMipLevel = 0;
    subResourceRange.levelCount = 1;
    subResourceRange.baseArrayLayer = 0;
    subResourceRange.layerCount = 1;

    VkImageMemoryBarrier imageMemoryBarrier;
    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    imageMemoryBarrier.image = vulSwapChain->getImage(currentImageIndex);
    imageMemoryBarrier.subresourceRange = subResourceRange;

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);

    VkRenderingAttachmentInfo colorAttachmentInfo;
    colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachmentInfo.imageView = vulSwapChain->getImageView(currentImageIndex);
    colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
    colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentInfo.clearValue = {0.0f, 0.0f, 0.0f, 1.0f};

    VkRenderingAttachmentInfo depthAttachmentInfo;
    depthAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    depthAttachmentInfo.imageView = vulSwapChain->getImageView(currentImageIndex);
    depthAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
    depthAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachmentInfo.clearValue = {1.0f};

    VkRenderingAttachmentInfo stencilAttachmentInfo;
    stencilAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    stencilAttachmentInfo.imageView = vulSwapChain->getImageView(currentImageIndex);
    stencilAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
    stencilAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    stencilAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    stencilAttachmentInfo.clearValue = {0};

    VkRenderingInfo renderingInfo;
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea.offset = {0, 0};
    renderingInfo.renderArea.extent = vulSwapChain->getSwapChainExtent();
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorAttachmentInfo;
    renderingInfo.pDepthAttachment = &depthAttachmentInfo;
    renderingInfo.pStencilAttachment = &stencilAttachmentInfo;

    /* VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = vulSwapChain->getRenderPass();
    renderPassInfo.framebuffer = vulSwapChain->getFrameBuffer(currentImageIndex);

    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = vulSwapChain->getSwapChainExtent();

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {0.0f, 0.0f, 0.0f, 1.0f};
    clearValues[1].depthStencil = {1.0f, 0};
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE); */

    vkCmdBeginRendering(commandBuffer, &renderingInfo);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(vulSwapChain->getSwapChainExtent().width);
    viewport.height = static_cast<float>(vulSwapChain->getSwapChainExtent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    VkRect2D scissor;
    scissor.offset = {0, 0};
    scissor.extent = vulSwapChain->getSwapChainExtent();
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void VulRenderer::endSwapChainRenderPass(VkCommandBuffer commandBuffer)
{
    assert(isFrameStarted && "Can't call endSwapChainRenderPass if the frame hasn't been started either");
    assert(commandBuffer == getCurrentCommandBuffer() && "Can't end a render pass on a command buffer from a different frame");

    // vkCmdEndRenderPass(commandBuffer);
    vkCmdEndRendering(commandBuffer);

    VkImageSubresourceRange subResourceRange;
    subResourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subResourceRange.baseMipLevel = 0;
    subResourceRange.levelCount = 1;
    subResourceRange.baseArrayLayer = 0;
    subResourceRange.layerCount = 1;

    VkImageMemoryBarrier imageMemoryBarrier;
    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    imageMemoryBarrier.image = vulSwapChain->getImage(currentImageIndex);
    imageMemoryBarrier.subresourceRange = subResourceRange;

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
}

}