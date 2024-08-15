#pragma once

#include"vul_window.hpp"

#include <vector>
#include <vulkan/vulkan_core.h>

namespace vul {

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct QueueFamilyIndices {
    uint32_t mainFamily;
    uint32_t computeFamily;
    uint32_t transferFamily;
    bool hasMainFamily = false;
    bool hasSeparateComputeFamily = false;
    bool hasSeparateTransferFamily = false;
    uint32_t sideQueueCount;
};

class VulDevice {
    public:
#ifdef NDEBUG
        const bool enableValidationLayers = false;
#else
        const bool enableValidationLayers = true;
#endif

        VulDevice(VulWindow &window, bool enableMeshShading, bool enableRayTracing);
        ~VulDevice();

        VulDevice(const VulDevice &) = delete;
        VulDevice &operator=(const VulDevice &) = delete;
        VulDevice(VulDevice &&) = delete;
        VulDevice &operator=(VulDevice &&) = delete;

        VkDevice device() const { return device_; }
        VkPhysicalDevice getPhysicalDevice() const {return physicalDevice;}
        VkSurfaceKHR surface() const { return surface_; }
        VkQueue mainQueue() const { return m_mainQueue; }
        VkQueue computeQueue() const { return m_computeQueue; }
        VkQueue transferQueue() const { return m_transferQueue; }
        std::vector<VkQueue> sideQueues() const { return m_sideQueues; }
        VkInstance getInstace() const {return instance;}

        SwapChainSupportDetails getSwapChainSupport() { return querySwapChainSupport(physicalDevice); }
        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
        QueueFamilyIndices findPhysicalQueueFamilies() const { return findQueueFamilies(physicalDevice); }
        VkFormat findSupportedFormat(
                const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

        void waitForIdle() const {vkDeviceWaitIdle(device_);}

        VkPhysicalDeviceProperties properties;

    private:
        void createInstance();
        void setupDebugMessenger();
        void createSurface();
        void pickPhysicalDevice();
        void createLogicalDevice(bool enableMeshShading, bool enableRaytracing);

        bool isDeviceSuitable(VkPhysicalDevice device);
        std::vector<const char *> getRequiredExtensions();
        bool checkValidationLayerSupport();
        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) const;
        void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);
        void hasGflwRequiredInstanceExtensions();
        bool checkDeviceExtensionSupport(VkPhysicalDevice device);
        SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

        VkInstance instance;
        VkDebugUtilsMessengerEXT debugMessenger;
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VulWindow &window;

        VkDevice device_;
        VkSurfaceKHR surface_;
        VkQueue m_mainQueue;
        VkQueue m_computeQueue;
        VkQueue m_transferQueue;
        std::vector<VkQueue> m_sideQueues;

        const std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation"};
        std::vector<const char *> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
};

}
