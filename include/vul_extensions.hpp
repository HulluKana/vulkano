#pragma once

#include<vulkan/vulkan_core.h>

namespace vul {

namespace extensions{

void addAccelerationStructure(VkDevice device, PFN_vkGetDeviceProcAddr getDeviceProcAddr);
void addRayTracingPipeline(VkDevice device, PFN_vkGetDeviceProcAddr getDeviceProcAddr);
void addMeshShader(VkDevice device, PFN_vkGetDeviceProcAddr getDeviceProcAddr);
void addVideo(VkInstance instance, PFN_vkGetInstanceProcAddr getInstanceProcAddr);

}
}
