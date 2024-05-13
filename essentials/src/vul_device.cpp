#include <vul_debug_tools.hpp>
#include<vul_device.hpp>
#include<vul_settings.hpp>
#include <vul_extensions.hpp>

// std headers
#include <cstring>
#include <iostream>
#include <set>
#include <unordered_set>
#include <vulkan/vulkan_core.h>

namespace vulB {

// local callback functions
static VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
              VkDebugUtilsMessageTypeFlagsEXT messageType,
              const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
              void *pUserData) {
  std::cerr << "Validation layer";
  if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
    std::cerr << " info";
  if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    std::cerr << " error";
  if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
    std::cerr << " verbose";
  if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    std::cerr << " warning";
  std::cerr << ": " << pCallbackData->pMessage << std::endl;

  return VK_FALSE;
}

VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkDebugUtilsMessengerEXT *pDebugMessenger) {
  auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkCreateDebugUtilsMessengerEXT");
  if (func != nullptr) {
    return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
  } else {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                   VkDebugUtilsMessengerEXT debugMessenger,
                                   const VkAllocationCallbacks *pAllocator) {
  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkDestroyDebugUtilsMessengerEXT");
  if (func != nullptr) {
    func(instance, debugMessenger, pAllocator);
  }
}

// class member functions
VulDevice::VulDevice(vulB::VulWindow &window) : window{window} {
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createCommandPools();

    DebugNamer::initialize(*this);
    VUL_NAME_VK(instance)
    VUL_NAME_VK(physicalDevice)
    VUL_NAME_VK(m_computeCommandPool)
    VUL_NAME_VK(commandPool)
    VUL_NAME_VK(device_)
    VUL_NAME_VK(surface_)
    VUL_NAME_VK(m_computeQueue)
    VUL_NAME_VK(presentQueue_)
    VUL_NAME_VK(graphicsQueue_)
}

VulDevice::~VulDevice() {
  vkDestroyCommandPool(device_, commandPool, nullptr);
  vkDestroyCommandPool(device_, m_computeCommandPool, nullptr);
  vkDestroyDevice(device_, nullptr);

  if (enableValidationLayers) {
    DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
  }

  vkDestroySurfaceKHR(instance, surface_, nullptr);
  vkDestroyInstance(instance, nullptr);
}

void VulDevice::createInstance() {
  if (enableValidationLayers && !checkValidationLayerSupport()) {
    throw std::runtime_error("validation layers requested, but not available!");
  }

  VkApplicationInfo appInfo = {};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = "Vulkano";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "Vulkano";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_3;

  VkInstanceCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;

  auto extensions = getRequiredExtensions();

  createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  createInfo.ppEnabledExtensionNames = extensions.data();

  VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
  if (enableValidationLayers) {
    createInfo.enabledLayerCount =
        static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();

    populateDebugMessengerCreateInfo(debugCreateInfo);
    createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;
  } else {
    createInfo.enabledLayerCount = 0;
    createInfo.pNext = nullptr;
  }

  if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
    throw std::runtime_error("failed to create instance!");
  }

  hasGflwRequiredInstanceExtensions();
}

void VulDevice::pickPhysicalDevice() {
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
  if (deviceCount == 0) {
    throw std::runtime_error("failed to find GPUs with Vulkan support!");
  }
  // std::cout << "Device count: " << deviceCount << std::endl;
  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

  for (const auto &device : devices) {
    if (isDeviceSuitable(device)) {
      physicalDevice = device;
      break;
    }
  }

  if (physicalDevice == VK_NULL_HANDLE) {
    throw std::runtime_error("failed to find a suitable GPU!");
  }

  vkGetPhysicalDeviceProperties(physicalDevice, &properties);
  // std::cout << "physical device: " << properties.deviceName << std::endl;
}

void VulDevice::createLogicalDevice() {
  QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  std::set<uint32_t> uniqueQueueFamilies = {
      indices.graphicsFamily, indices.presentFamily, indices.computeFamily};

  float queuePriority = 1.0f;
  for (uint32_t queueFamily : uniqueQueueFamilies) {
    VkDeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = queueFamily;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    queueCreateInfos.push_back(queueCreateInfo);
  }

  VkPhysicalDeviceAccelerationStructureFeaturesKHR accelStructFeatures{};
  accelStructFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;

  VkPhysicalDeviceRayTracingPipelineFeaturesKHR rtPipelineFeatures{};
  rtPipelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
  rtPipelineFeatures.pNext = &accelStructFeatures;

  VkPhysicalDeviceHostQueryResetFeatures reset{};
  reset.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_QUERY_RESET_FEATURES;
  reset.pNext = &rtPipelineFeatures;

  VkPhysicalDeviceScalarBlockLayoutFeatures scalarBlockFeatures{};
  scalarBlockFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SCALAR_BLOCK_LAYOUT_FEATURES;
  scalarBlockFeatures.scalarBlockLayout = VK_TRUE;

  if (vul::settings::deviceInitConfig.enableRaytracingSupport) {
      deviceExtensions.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
      deviceExtensions.push_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
      deviceExtensions.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
      scalarBlockFeatures.pNext = &reset;
  }

  VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeatures{};
  descriptorIndexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
  descriptorIndexingFeatures.runtimeDescriptorArray = VK_TRUE;
  descriptorIndexingFeatures.pNext = &scalarBlockFeatures;

  VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddresFeature{};
  bufferDeviceAddresFeature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
  bufferDeviceAddresFeature.bufferDeviceAddress = VK_TRUE;
  bufferDeviceAddresFeature.pNext = &descriptorIndexingFeatures;

  VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamic_rendering_feature{};
  dynamic_rendering_feature.sType =
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR;
  dynamic_rendering_feature.dynamicRendering = VK_TRUE;
  dynamic_rendering_feature.pNext = &bufferDeviceAddresFeature;

  VkPhysicalDeviceFeatures2 physicalFeatures2{};
  physicalFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
  physicalFeatures2.features.samplerAnisotropy = VK_TRUE;
  physicalFeatures2.pNext = &dynamic_rendering_feature;
  vkGetPhysicalDeviceFeatures2(physicalDevice, &physicalFeatures2);

  VkDeviceCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

  createInfo.queueCreateInfoCount =
      static_cast<uint32_t>(queueCreateInfos.size());
  createInfo.pQueueCreateInfos = queueCreateInfos.data();

  createInfo.pNext = &physicalFeatures2;
  createInfo.enabledExtensionCount =
      static_cast<uint32_t>(deviceExtensions.size());
  createInfo.ppEnabledExtensionNames = deviceExtensions.data();

  // might not really be necessary anymore because device specific validation
  // layers have been deprecated
  if (enableValidationLayers) {
    createInfo.enabledLayerCount =
        static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();
  } else {
    createInfo.enabledLayerCount = 0;
  }

  if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device_) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create logical device!");
  }

  vkGetDeviceQueue(device_, indices.graphicsFamily, 0, &graphicsQueue_);
  vkGetDeviceQueue(device_, indices.presentFamily, 0, &presentQueue_);
  vkGetDeviceQueue(device_, indices.computeFamily, 0, &m_computeQueue);

  if (vul::settings::deviceInitConfig.enableRaytracingSupport) {
      extensions::addAccelerationStructure(device_, vkGetDeviceProcAddr);
      extensions::addRayTracingPipeline(device_, vkGetDeviceProcAddr);
  }
}

void VulDevice::createCommandPools() {
  QueueFamilyIndices queueFamilyIndices = findPhysicalQueueFamilies();

  VkCommandPoolCreateInfo poolInfo = {};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  if (vkCreateCommandPool(device_, &poolInfo, nullptr, &commandPool) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create command pool!");
  }

  poolInfo.queueFamilyIndex = queueFamilyIndices.computeFamily;
  if (vkCreateCommandPool(device_, &poolInfo, nullptr, &m_computeCommandPool) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create compute command pool!");
  }
}

void VulDevice::createSurface() {
  window.createWindowSurface(instance, &surface_);
}

bool VulDevice::isDeviceSuitable(VkPhysicalDevice device) {
  QueueFamilyIndices indices = findQueueFamilies(device);

  bool extensionsSupported = checkDeviceExtensionSupport(device);

  bool swapChainAdequate = false;
  if (extensionsSupported) {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
    swapChainAdequate = !swapChainSupport.formats.empty() &&
                        !swapChainSupport.presentModes.empty();
  }

  VkPhysicalDeviceFeatures supportedFeatures;
  vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

  return indices.isComplete() && extensionsSupported && swapChainAdequate &&
         supportedFeatures.samplerAnisotropy;
}

void VulDevice::populateDebugMessengerCreateInfo(
    VkDebugUtilsMessengerCreateInfoEXT &createInfo) {
  createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  createInfo.pfnUserCallback = debugCallback;
  createInfo.pUserData = nullptr; // Optional
}

void VulDevice::setupDebugMessenger() {
  if (!enableValidationLayers)
    return;
  VkDebugUtilsMessengerCreateInfoEXT createInfo;
  populateDebugMessengerCreateInfo(createInfo);
  if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr,
                                   &debugMessenger) != VK_SUCCESS) {
    throw std::runtime_error("failed to set up debug messenger!");
  }
}

bool VulDevice::checkValidationLayerSupport() {
  uint32_t layerCount;
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

  std::vector<VkLayerProperties> availableLayers(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

  for (const char *layerName : validationLayers) {
    bool layerFound = false;

    for (const auto &layerProperties : availableLayers) {
      if (strcmp(layerName, layerProperties.layerName) == 0) {
        layerFound = true;
        break;
      }
    }

    if (!layerFound) {
      return false;
    }
  }

  return true;
}

std::vector<const char *> VulDevice::getRequiredExtensions() {
  uint32_t glfwExtensionCount = 0;
  const char **glfwExtensions;
  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

  std::vector<const char *> extensions(glfwExtensions,
                                       glfwExtensions + glfwExtensionCount);
  // Required by imguis dynamic rendering
  extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

  if (enableValidationLayers) {
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }

  return extensions;
}

void VulDevice::hasGflwRequiredInstanceExtensions() {
  uint32_t extensionCount = 0;
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
  std::vector<VkExtensionProperties> extensions(extensionCount);
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount,
                                         extensions.data());

  // std::cout << "available extensions:" << std::endl;
  std::unordered_set<std::string> available;
  for (const auto &extension : extensions) {
    // std::cout << "\t" << extension.extensionName << std::endl;
    available.insert(extension.extensionName);
  }

  // std::cout << "required extensions:" << std::endl;
  auto requiredExtensions = getRequiredExtensions();
  for (const auto &required : requiredExtensions) {
    // std::cout << "\t" << required << std::endl;
    if (available.find(required) == available.end()) {
      throw std::runtime_error("Missing required glfw extension");
    }
  }
}

bool VulDevice::checkDeviceExtensionSupport(VkPhysicalDevice device) {
  uint32_t extensionCount;
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
                                       nullptr);

  std::vector<VkExtensionProperties> availableExtensions(extensionCount);
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
                                       availableExtensions.data());

  std::set<std::string> requiredExtensions(deviceExtensions.begin(),
                                           deviceExtensions.end());

  for (const auto &extension : availableExtensions) {
    requiredExtensions.erase(extension.extensionName);
  }

  return requiredExtensions.empty();
}

QueueFamilyIndices VulDevice::findQueueFamilies(VkPhysicalDevice device) {
  QueueFamilyIndices indices;

  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
                                           queueFamilies.data());

  std::vector<uint32_t> computeFamilies;
  uint32_t i = 0;
  for (const auto &queueFamily : queueFamilies) {
    if (queueFamily.queueCount > 0 &&
        (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
        !indices.graphicsFamilyHasValue) {
      indices.graphicsFamily = i;
      indices.graphicsFamilyHasValue = true;
    }
    if (queueFamily.queueCount > 0 &&
        (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) &&
        !indices.computeFamilyHasValue) {
      if (vul::settings::deviceInitConfig.preferSeparateComputeQueue)
        computeFamilies.push_back(i);
      else {
        indices.computeFamily = i;
        indices.computeFamilyHasValue = true;
      }
    }
    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface_, &presentSupport);
    if (queueFamily.queueCount > 0 && presentSupport &&
        !indices.presentFamilyHasValue) {
      indices.presentFamily = i;
      indices.presentFamilyHasValue = true;
    }

    i++;
  }

  if (vul::settings::deviceInitConfig.preferSeparateComputeQueue) {
    for (uint32_t computeFamilyIdx : computeFamilies) {
      if (computeFamilyIdx != indices.graphicsFamily) {
        indices.computeFamily = computeFamilyIdx;
        indices.computeFamilyHasValue = true;
        break;
      }
    }
    if (!indices.computeFamilyHasValue)
      indices.computeFamily = computeFamilies[0];
  }

  return indices;
}

SwapChainSupportDetails
VulDevice::querySwapChainSupport(VkPhysicalDevice device) {
  SwapChainSupportDetails details;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface_,
                                            &details.capabilities);

  uint32_t formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &formatCount, nullptr);

  if (formatCount != 0) {
    details.formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &formatCount,
                                         details.formats.data());
  }

  uint32_t presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &presentModeCount,
                                            nullptr);

  if (presentModeCount != 0) {
    details.presentModes.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        device, surface_, &presentModeCount, details.presentModes.data());
  }
  return details;
}

VkFormat VulDevice::findSupportedFormat(const std::vector<VkFormat> &candidates,
                                        VkImageTiling tiling,
                                        VkFormatFeatureFlags features) {
  for (VkFormat format : candidates) {
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

    if (tiling == VK_IMAGE_TILING_LINEAR &&
        (props.linearTilingFeatures & features) == features) {
      return format;
    } else if (tiling == VK_IMAGE_TILING_OPTIMAL &&
               (props.optimalTilingFeatures & features) == features) {
      return format;
    }
  }
  throw std::runtime_error("failed to find supported format!");
}

uint32_t VulDevice::findMemoryType(uint32_t typeFilter,
                                   VkMemoryPropertyFlags properties) const {
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags &
                                    properties) == properties) {
      return i;
    }
  }

  throw std::runtime_error("failed to find suitable memory type!");
}

VkCommandBuffer VulDevice::beginSingleTimeCommands() const{
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = commandPool;
  allocInfo.commandBufferCount = 1;

  VkCommandBuffer commandBuffer;
  vkAllocateCommandBuffers(device_, &allocInfo, &commandBuffer);

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(commandBuffer, &beginInfo);
  return commandBuffer;
}

void VulDevice::endSingleTimeCommands(VkCommandBuffer commandBuffer) const{
  vkEndCommandBuffer(commandBuffer);

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  vkQueueSubmit(graphicsQueue_, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(graphicsQueue_);

  vkFreeCommandBuffers(device_, commandPool, 1, &commandBuffer);
}

void VulDevice::copyBufferToImage(VkBuffer buffer, VkImage image,
                                  uint32_t width, uint32_t height,
                                  uint32_t layerCount) {
  VkCommandBuffer commandBuffer = beginSingleTimeCommands();

  VkBufferImageCopy region{};
  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;

  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = layerCount;

  region.imageOffset = {0, 0, 0};
  region.imageExtent = {width, height, 1};

  vkCmdCopyBufferToImage(commandBuffer, buffer, image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
  endSingleTimeCommands(commandBuffer);
}

} // namespace vulB
