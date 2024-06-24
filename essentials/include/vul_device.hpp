#pragma once

#include"vul_window.hpp"

// std lib headers
#include <string>
#include <vector>

namespace vul {

struct SwapChainSupportDetails {
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> presentModes;
};

struct QueueFamilyIndices {
  uint32_t graphicsFamily;
  uint32_t presentFamily;
  uint32_t computeFamily;
  bool graphicsFamilyHasValue = false;
  bool presentFamilyHasValue = false;
  bool computeFamilyHasValue = false;
  bool isComplete() { return graphicsFamilyHasValue && presentFamilyHasValue && computeFamilyHasValue; }
};

class VulDevice {
 public:
#ifdef NDEBUG
  const bool enableValidationLayers = false;
#else
  const bool enableValidationLayers = true;
#endif

  VulDevice(VulWindow &window);
  ~VulDevice();

  // Not copyable or movable
  VulDevice(const VulDevice &) = delete;
  VulDevice &operator=(const VulDevice &) = delete;
  VulDevice(VulDevice &&) = delete;
  VulDevice &operator=(VulDevice &&) = delete;

  VkCommandPool getCommandPool() const { return commandPool; }
  VkCommandPool getComputeCommandPool() const { return m_computeCommandPool; }
  VkDevice device() const { return device_; }
  VkPhysicalDevice getPhysicalDevice() const {return physicalDevice;}
  VkSurfaceKHR surface() const { return surface_; }
  VkQueue graphicsQueue() const { return graphicsQueue_; }
  VkQueue presentQueue() const { return presentQueue_; }
  VkQueue computeQueue() const { return m_computeQueue; }
  VkInstance getInstace() const {return instance;}

  SwapChainSupportDetails getSwapChainSupport() { return querySwapChainSupport(physicalDevice); }
  uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
  QueueFamilyIndices findPhysicalQueueFamilies() { return findQueueFamilies(physicalDevice); }
  VkFormat findSupportedFormat(
      const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

  // Buffer Helper Functions
  VkCommandBuffer beginSingleTimeCommands() const;
  void endSingleTimeCommands(VkCommandBuffer commandBuffer) const;
  void copyBufferToImage(
      VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layerCount);

  VkPhysicalDeviceProperties properties;

 private:
  void createInstance();
  void setupDebugMessenger();
  void createSurface();
  void pickPhysicalDevice();
  void createLogicalDevice();
  void createCommandPools();

  // helper functions
  bool isDeviceSuitable(VkPhysicalDevice device);
  std::vector<const char *> getRequiredExtensions();
  bool checkValidationLayerSupport();
  QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
  void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);
  void hasGflwRequiredInstanceExtensions();
  bool checkDeviceExtensionSupport(VkPhysicalDevice device);
  SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

  VkInstance instance;
  VkDebugUtilsMessengerEXT debugMessenger;
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
  VulWindow &window;
  VkCommandPool commandPool;
  VkCommandPool m_computeCommandPool;

  VkDevice device_;
  VkSurfaceKHR surface_;
  VkQueue graphicsQueue_;
  VkQueue presentQueue_;
  VkQueue m_computeQueue;

  const std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation"}; // Enabled validation layers for normal use
  //const std::vector<const char *> validationLayers = {}; // Disabled validation layers for nsight
  std::vector<const char *> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
};

}
