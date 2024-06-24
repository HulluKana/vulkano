#pragma once

#include "vul_device.hpp"
#include <chrono>
#include <string>
#include <string.h>
#include <vulkan/vulkan_core.h>

#ifdef VUL_ENABLE_PROFILER
#define COMBINE_THINGS_ASSISTANT(X,Y) X##Y
#define COMBINE_THINGS(X,Y) COMBINE_THINGS_ASSISTANT(X,Y)
#define VUL_PROFILE_SCOPE(name) ScopedTimer COMBINE_THINGS(scopedTimer_, __LINE__)(name);
#define VUL_PROFILE_FUNC() VUL_PROFILE_SCOPE(__PRETTY_FUNCTION__)
#else
#define VUL_PROFILE_SCOPE(name);
#define VUL_PROFILE_FUNC();
#endif

#ifdef VUL_ENABLE_DEBUG_NAMER
#define FILE_NAME (strrchr("/" __FILE__, '/') + 1)
#define VUL_NAME_VK_MANUAL(obj, name) DebugNamer::setObjectName(obj, name);
#define VUL_NAME_VK(obj) VUL_NAME_VK_MANUAL(obj, #obj " " + std::string(FILE_NAME) + ':' + std::to_string(__LINE__));
#define VUL_NAME_VK_IDX(obj, idx) VUL_NAME_VK_MANUAL(obj, #obj " #" + std::to_string(idx) + " " + std::string(FILE_NAME) + ':' + std::to_string(__LINE__));
#else
#define VUL_NAME_VK_MANUAL(obj, name);
#define VUL_NAME_VK(obj);
#define VUL_NAME_VK_IDX(obj, idx);
#endif

namespace vul {

namespace ProfAnalyzer {

void resetMeasurements();
void dumpMeasurementTree(const std::string &fileName);
void dumpMeasurementSummary(const std::string &fileName);

}
    
class ScopedTimer {
    public:
        ScopedTimer(const char *name);
        ~ScopedTimer();
    private:
        const char *m_name; 
        std::chrono::steady_clock::time_point m_startTime;
};

}

namespace vul {

namespace DebugNamer {

void initialize(VulDevice &device);
void setObjectName(const uint64_t object, const std::string &name, VkObjectType type);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
static void setObjectName(VkBuffer object, const std::string& name)                     { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_BUFFER); }
static void setObjectName(VkCommandBuffer object, const std::string& name)              { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_COMMAND_BUFFER ); }
static void setObjectName(VkCommandPool object, const std::string& name)                { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_COMMAND_POOL ); }
static void setObjectName(VkDescriptorPool object, const std::string& name)             { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_DESCRIPTOR_POOL); }
static void setObjectName(VkDescriptorSet object, const std::string& name)              { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_DESCRIPTOR_SET); }
static void setObjectName(VkDescriptorSetLayout object, const std::string& name)        { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT); }
static void setObjectName(VkInstance object, const std::string& name)                   { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_INSTANCE); }
static void setObjectName(VkPhysicalDevice object, const std::string& name)             { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_PHYSICAL_DEVICE); }
static void setObjectName(VkDevice object, const std::string& name)                     { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_DEVICE); }
static void setObjectName(VkDeviceMemory object, const std::string& name)               { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_DEVICE_MEMORY); }
static void setObjectName(VkImage object, const std::string& name)                      { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_IMAGE); }
static void setObjectName(VkImageView object, const std::string& name)                  { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_IMAGE_VIEW); }
static void setObjectName(VkPipeline object, const std::string& name)                   { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_PIPELINE); }
static void setObjectName(VkPipelineLayout object, const std::string& name)             { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_PIPELINE_LAYOUT); }
static void setObjectName(VkQueue object, const std::string& name)                      { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_QUEUE); }
static void setObjectName(VkSampler object, const std::string& name)                    { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_SAMPLER); }
static void setObjectName(VkSemaphore object, const std::string& name)                  { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_SEMAPHORE); }
static void setObjectName(VkFence object, const std::string& name)                      { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_FENCE); }
static void setObjectName(VkShaderModule object, const std::string& name)               { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_SHADER_MODULE); }
static void setObjectName(VkSwapchainKHR object, const std::string& name)               { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_SWAPCHAIN_KHR); }
static void setObjectName(VkSurfaceKHR object, const std::string& name)                 { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_SURFACE_KHR); }
static void setObjectName(VkAccelerationStructureKHR object, const std::string& name)   { setObjectName((uint64_t)object, name, VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR); }
#pragma GCC diagnostic pop

}

}
