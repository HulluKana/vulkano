#pragma once

#include "vul_attachment_image.hpp"
#include"vul_device.hpp"
#include"vul_swap_chain.hpp"
#include"vul_window.hpp"

#include<cassert>
#include<memory>

namespace vulB{

class VulRenderer{
    public:
        VulRenderer(VulWindow &window, VulDevice &device);
        ~VulRenderer();

        /* These 2 lines remove the copy constructor and operator from VulRenderer class.
        Because I'm using a pointer to stuff and that stuff is initialized by constructor and removed by destructor,
        copying the pointer and then removing the original pointer leaves me with pointer pointing to nothing, possibly leading to VERY nasty bugs */
        VulRenderer(const VulRenderer &) = delete;
        VulRenderer &operator=(const VulRenderer &) = delete;

        float getAspectRatio() const {return vulSwapChain->extentAspectRatio();}
        VkExtent2D getSwapChainExtent() const {return vulSwapChain->getSwapChainExtent();}
        VkFormat getSwapChainColorFormat() const {return vulSwapChain->getSwapChainImageFormat();}
        bool wasSwapChainRecreated() const {return m_swapchainRecreated;}
        VkFormat getDepthFormat() const {return m_depthFormat;}
        const std::vector<std::unique_ptr<VulAttachmentImage>> &getDepthImages() const {return m_depthImages;}
        bool isFrameInProgress() const {return isFrameStarted;}

        VkCommandBuffer getCurrentCommandBuffer() const {
            assert(isFrameStarted && "Cannot get command buffer when frame is not in progress");
            return commandBuffers[currentFrameIndex];
        }

        int getFrameIndex() const {
            assert(isFrameStarted && "Cannot get frame index when frame is not in progress");
            return currentFrameIndex;
        }

        VkCommandBuffer beginFrame();
        void endFrame();
        void beginRendering(VkCommandBuffer commandBuffer, const std::vector<std::shared_ptr<VulAttachmentImage>> &attachmentImages, bool preservePreviousSwapchainImageContents, uint32_t renderWidth, uint32_t renderHeight);
        void stopRendering(VkCommandBuffer commandBuffer);
        
    private:
        void createCommandBuffers();
        void recreateSwapChain();

        VulWindow& vulWindow;
        VulDevice& vulDevice;
        std::unique_ptr<VulSwapChain> vulSwapChain;
        std::vector<VkCommandBuffer> commandBuffers;

        std::vector<std::unique_ptr<VulAttachmentImage>> m_depthImages;
        VkFormat m_depthFormat;

        uint32_t currentImageIndex;
        int currentFrameIndex{0};
        bool isFrameStarted = false;
        bool m_swapchainRecreated = false;
};
}
