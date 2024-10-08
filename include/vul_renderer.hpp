#pragma once

#include "vul_command_pool.hpp"
#include"vul_device.hpp"
#include"vul_swap_chain.hpp"
#include"vul_window.hpp"
#include <vul_image.hpp>

#include <glm/glm.hpp>
#include<cassert>
#include<memory>
#include <vulkan/vulkan_core.h>

namespace vul{

class VulRenderer{
    public:
        VulRenderer(VulWindow &window, VulDevice &device, std::shared_ptr<vul::VulSampler> depthImgSampler);
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
        const std::vector<std::unique_ptr<VulImage>> &getDepthImages() const {return m_depthImages;}
        bool isFrameInProgress() const {return isFrameStarted;}

        VkCommandBuffer getCurrentCommandBuffer() const {
            assert(isFrameStarted && "Cannot get command buffer when frame is not in progress");
            return commandBuffers[currentFrameIndex];
        }

        int getFrameIndex() const {
            assert(isFrameStarted && "Cannot get frame index when frame is not in progress");
            return currentFrameIndex;
        }
        uint32_t getImageIndex() const {return currentImageIndex;}

        VkCommandBuffer beginFrame();
        void endFrame();

        enum class SwapChainImageMode {
            clearPreviousStoreCurrent,
            preservePreviousStoreCurrent,
            noSwapChainImage
        };
        enum class DepthImageMode {
            clearPreviousStoreCurrent,
            clearPreviousDiscardCurrent,
            preservePreviousStoreCurrent,
            customDepthImage,
            noDepthImage
        };
        void beginRendering(VkCommandBuffer commandBuffer, SwapChainImageMode swapChainImageMode, DepthImageMode depthImageMode,
                const std::vector<std::shared_ptr<VulImage>> &attachmentImages, const VkRenderingAttachmentInfo customDepthAttacmentInfo,
                const glm::vec4 &swapChainClearColor, float depthClearColor, uint32_t renderWidth, uint32_t renderHeight, uint32_t layerCount) const;
        void stopRendering(VkCommandBuffer commandBuffer) const;
        
    private:
        void createCommandBuffers();
        void recreateSwapChain();

        VulWindow& vulWindow;
        VulDevice& vulDevice;
        std::unique_ptr<VulSwapChain> vulSwapChain;
        std::vector<VkCommandBuffer> commandBuffers;
        std::unique_ptr<VulCmdPool> m_cmdPool;

        std::vector<std::unique_ptr<VulImage>> m_depthImages;
        VkFormat m_depthFormat;
        std::shared_ptr<vul::VulSampler> m_depthImgSampler;

        uint32_t currentImageIndex;
        int currentFrameIndex{0};
        bool isFrameStarted = false;
        bool m_swapchainRecreated = false;
};
}
