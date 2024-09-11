#pragma once

#include <vul_image.hpp>

// vulkan headers
#include <vulkan/vulkan.h>

#include <memory>
#include <vector>

namespace vul {

class VulSwapChain {
 public:
  static constexpr int MAX_FRAMES_IN_FLIGHT = 3;

  VulSwapChain(VulDevice &deviceRef, VkExtent2D windowExtent);
  VulSwapChain(VulDevice &deviceRef, VkExtent2D windowExtent, std::shared_ptr<VulSwapChain> previous);
  ~VulSwapChain();

  VulSwapChain(const VulSwapChain &) = delete;
  VulSwapChain &operator=(const VulSwapChain &) = delete;

  size_t imageCount() { return swapChainImages.size(); }
  std::shared_ptr<VulImage> getImage(uint32_t index) {return swapChainImages[index];}
  VkFormat getSwapChainImageFormat() { return swapChainImageFormat; }
  VkExtent2D getSwapChainExtent() { return swapChainExtent; }
  uint32_t width() { return swapChainExtent.width; }
  uint32_t height() { return swapChainExtent.height; }

  float extentAspectRatio() {
    return static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.height);
  }

  VkResult acquireNextImage(uint32_t *imageIndex);
  VkResult submitCommandBuffers(const VkCommandBuffer *buffers, uint32_t *imageIndex);

  bool compareSwapFormats(const VulSwapChain &swapChain)
  {
    return swapChain.swapChainImageFormat == swapChainImageFormat;
  }

 private:
  void init();
  void createSwapChain();
  void createSyncObjects();

  // Helper functions
  VkSurfaceFormatKHR chooseSwapSurfaceFormat(
      const std::vector<VkSurfaceFormatKHR> &availableFormats);
  VkPresentModeKHR chooseSwapPresentMode(
      const std::vector<VkPresentModeKHR> &availablePresentModes);
  VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);

  VkFormat swapChainImageFormat;
  VkExtent2D swapChainExtent;

  std::vector<std::shared_ptr<VulImage>> swapChainImages;

  VulDevice &device;
  VkExtent2D windowExtent;

  VkSwapchainKHR swapChain;
  std::shared_ptr<VulSwapChain> oldSwapChain;

  std::vector<VkSemaphore> imageAvailableSemaphores;
  std::vector<VkSemaphore> renderFinishedSemaphores;
  std::vector<VkFence> inFlightFences;
  std::vector<VkFence> imagesInFlight;
  size_t currentFrame = 0;
};

}  // namespace lve
