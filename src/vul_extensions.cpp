#include<vul_extensions.hpp>
#include <vulkan/vulkan_core.h>

using namespace vul::extensions;

static PFN_vkBuildAccelerationStructuresKHR pfn_vkBuildAccelerationStructuresKHR= 0;
static PFN_vkCmdBuildAccelerationStructuresIndirectKHR pfn_vkCmdBuildAccelerationStructuresIndirectKHR= 0;
static PFN_vkCmdBuildAccelerationStructuresKHR pfn_vkCmdBuildAccelerationStructuresKHR= 0;
static PFN_vkCmdCopyAccelerationStructureKHR pfn_vkCmdCopyAccelerationStructureKHR= 0;
static PFN_vkCmdCopyAccelerationStructureToMemoryKHR pfn_vkCmdCopyAccelerationStructureToMemoryKHR= 0;
static PFN_vkCmdCopyMemoryToAccelerationStructureKHR pfn_vkCmdCopyMemoryToAccelerationStructureKHR= 0;
static PFN_vkCmdWriteAccelerationStructuresPropertiesKHR pfn_vkCmdWriteAccelerationStructuresPropertiesKHR= 0;
static PFN_vkCopyAccelerationStructureKHR pfn_vkCopyAccelerationStructureKHR= 0;
static PFN_vkCopyAccelerationStructureToMemoryKHR pfn_vkCopyAccelerationStructureToMemoryKHR= 0;
static PFN_vkCopyMemoryToAccelerationStructureKHR pfn_vkCopyMemoryToAccelerationStructureKHR= 0;
static PFN_vkCreateAccelerationStructureKHR pfn_vkCreateAccelerationStructureKHR= 0;
static PFN_vkDestroyAccelerationStructureKHR pfn_vkDestroyAccelerationStructureKHR= 0;
static PFN_vkGetAccelerationStructureBuildSizesKHR pfn_vkGetAccelerationStructureBuildSizesKHR= 0;
static PFN_vkGetAccelerationStructureDeviceAddressKHR pfn_vkGetAccelerationStructureDeviceAddressKHR= 0;
static PFN_vkGetDeviceAccelerationStructureCompatibilityKHR pfn_vkGetDeviceAccelerationStructureCompatibilityKHR= 0;
static PFN_vkWriteAccelerationStructuresPropertiesKHR pfn_vkWriteAccelerationStructuresPropertiesKHR= 0;

static PFN_vkCmdSetRayTracingPipelineStackSizeKHR pfn_vkCmdSetRayTracingPipelineStackSizeKHR= 0;
static PFN_vkCmdTraceRaysIndirectKHR pfn_vkCmdTraceRaysIndirectKHR= 0;
static PFN_vkCmdTraceRaysKHR pfn_vkCmdTraceRaysKHR= 0;
static PFN_vkCreateRayTracingPipelinesKHR pfn_vkCreateRayTracingPipelinesKHR= 0;
static PFN_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR pfn_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR= 0;
static PFN_vkGetRayTracingShaderGroupHandlesKHR pfn_vkGetRayTracingShaderGroupHandlesKHR= 0;
static PFN_vkGetRayTracingShaderGroupStackSizeKHR pfn_vkGetRayTracingShaderGroupStackSizeKHR= 0;

static PFN_vkCmdDrawMeshTasksEXT pfn_vkCmdDrawMeshTasksEXT= 0;
static PFN_vkCmdDrawMeshTasksIndirectCountEXT pfn_vkCmdDrawMeshTasksIndirectCountEXT= 0;
static PFN_vkCmdDrawMeshTasksIndirectEXT pfn_vkCmdDrawMeshTasksIndirectEXT= 0;

static PFN_vkGetPhysicalDeviceVideoFormatPropertiesKHR pfn_vkGetPhysicalDeviceVideoFormatPropertiesKHR= 0;
static PFN_vkGetPhysicalDeviceVideoCapabilitiesKHR pfn_vkGetPhysicalDeviceVideoCapabilitiesKHR= 0;
static PFN_vkCreateVideoSessionKHR pfn_vkCreateVideoSessionKHR= 0;
static PFN_vkDestroyVideoSessionKHR pfn_vkDestroyVideoSessionKHR= 0;
static PFN_vkGetVideoSessionMemoryRequirementsKHR pfn_vkGetVideoSessionMemoryRequirementsKHR= 0;
static PFN_vkBindVideoSessionMemoryKHR pfn_vkBindVideoSessionMemoryKHR= 0;

// Acceleration structure functions start here
//----------------------------------------------------------------------------------------------------------------------
VkResult vkBuildAccelerationStructuresKHR(
	VkDevice                                           device, 
	VkDeferredOperationKHR deferredOperation, 
	uint32_t infoCount, 
	const VkAccelerationStructureBuildGeometryInfoKHR* pInfos, 
	const VkAccelerationStructureBuildRangeInfoKHR* const* ppBuildRangeInfos) 
{ 
  return pfn_vkBuildAccelerationStructuresKHR(device, deferredOperation, infoCount, pInfos, ppBuildRangeInfos); 
}
void vkCmdBuildAccelerationStructuresIndirectKHR(
	VkCommandBuffer                  commandBuffer, 
	uint32_t                                           infoCount, 
	const VkAccelerationStructureBuildGeometryInfoKHR* pInfos, 
	const VkDeviceAddress*             pIndirectDeviceAddresses, 
	const uint32_t*                    pIndirectStrides, 
	const uint32_t* const*             ppMaxPrimitiveCounts) 
{ 
  pfn_vkCmdBuildAccelerationStructuresIndirectKHR(commandBuffer, infoCount, pInfos, pIndirectDeviceAddresses, pIndirectStrides, ppMaxPrimitiveCounts); 
}
void vkCmdBuildAccelerationStructuresKHR(
	VkCommandBuffer                                    commandBuffer, 
	uint32_t infoCount, 
	const VkAccelerationStructureBuildGeometryInfoKHR* pInfos, 
	const VkAccelerationStructureBuildRangeInfoKHR* const* ppBuildRangeInfos) 
{ 
  pfn_vkCmdBuildAccelerationStructuresKHR(commandBuffer, infoCount, pInfos, ppBuildRangeInfos); 
}
void vkCmdCopyAccelerationStructureKHR(
	VkCommandBuffer commandBuffer, 
	const VkCopyAccelerationStructureInfoKHR* pInfo) 
{ 
  pfn_vkCmdCopyAccelerationStructureKHR(commandBuffer, pInfo); 
}
void vkCmdCopyAccelerationStructureToMemoryKHR(
	VkCommandBuffer commandBuffer, 
	const VkCopyAccelerationStructureToMemoryInfoKHR* pInfo) 
{ 
  pfn_vkCmdCopyAccelerationStructureToMemoryKHR(commandBuffer, pInfo); 
}
void vkCmdCopyMemoryToAccelerationStructureKHR(
	VkCommandBuffer commandBuffer, 
	const VkCopyMemoryToAccelerationStructureInfoKHR* pInfo) 
{ 
  pfn_vkCmdCopyMemoryToAccelerationStructureKHR(commandBuffer, pInfo); 
}
void vkCmdWriteAccelerationStructuresPropertiesKHR(
	VkCommandBuffer commandBuffer, 
	uint32_t accelerationStructureCount, 
	const VkAccelerationStructureKHR* pAccelerationStructures, 
	VkQueryType queryType, 
	VkQueryPool queryPool, 
	uint32_t firstQuery) 
{ 
  pfn_vkCmdWriteAccelerationStructuresPropertiesKHR(commandBuffer, accelerationStructureCount, pAccelerationStructures, queryType, queryPool, firstQuery); 
}
VkResult vkCopyAccelerationStructureKHR(
	VkDevice device, 
	VkDeferredOperationKHR deferredOperation, 
	const VkCopyAccelerationStructureInfoKHR* pInfo) 
{ 
  return pfn_vkCopyAccelerationStructureKHR(device, deferredOperation, pInfo); 
}
VkResult vkCopyAccelerationStructureToMemoryKHR(
	VkDevice device, 
	VkDeferredOperationKHR deferredOperation, 
	const VkCopyAccelerationStructureToMemoryInfoKHR* pInfo) 
{ 
  return pfn_vkCopyAccelerationStructureToMemoryKHR(device, deferredOperation, pInfo); 
}
VkResult vkCopyMemoryToAccelerationStructureKHR(
	VkDevice device, 
	VkDeferredOperationKHR deferredOperation, 
	const VkCopyMemoryToAccelerationStructureInfoKHR* pInfo) 
{ 
  return pfn_vkCopyMemoryToAccelerationStructureKHR(device, deferredOperation, pInfo); 
}
VkResult vkCreateAccelerationStructureKHR(
	VkDevice                                           device, 
	const VkAccelerationStructureCreateInfoKHR*        pCreateInfo, 
	const VkAllocationCallbacks*       pAllocator, 
	VkAccelerationStructureKHR*                        pAccelerationStructure) 
{ 
  return pfn_vkCreateAccelerationStructureKHR(device, pCreateInfo, pAllocator, pAccelerationStructure); 
}
void vkDestroyAccelerationStructureKHR(
	VkDevice device, 
	VkAccelerationStructureKHR accelerationStructure, 
	const VkAllocationCallbacks* pAllocator) 
{ 
  pfn_vkDestroyAccelerationStructureKHR(device, accelerationStructure, pAllocator); 
}
void vkGetAccelerationStructureBuildSizesKHR(
	VkDevice                                            device, 
	VkAccelerationStructureBuildTypeKHR                 buildType, 
	const VkAccelerationStructureBuildGeometryInfoKHR*  pBuildInfo, 
	const uint32_t*  pMaxPrimitiveCounts, 
	VkAccelerationStructureBuildSizesInfoKHR*           pSizeInfo) 
{ 
  pfn_vkGetAccelerationStructureBuildSizesKHR(device, buildType, pBuildInfo, pMaxPrimitiveCounts, pSizeInfo); 
}
VkDeviceAddress vkGetAccelerationStructureDeviceAddressKHR(
	VkDevice device, 
	const VkAccelerationStructureDeviceAddressInfoKHR* pInfo) 
{ 
  return pfn_vkGetAccelerationStructureDeviceAddressKHR(device, pInfo); 
}
void vkGetDeviceAccelerationStructureCompatibilityKHR(
	VkDevice device, 
	const VkAccelerationStructureVersionInfoKHR* pVersionInfo, 
	VkAccelerationStructureCompatibilityKHR* pCompatibility) 
{ 
  pfn_vkGetDeviceAccelerationStructureCompatibilityKHR(device, pVersionInfo, pCompatibility); 
}
VkResult vkWriteAccelerationStructuresPropertiesKHR(
	VkDevice device, 
	uint32_t accelerationStructureCount, 
	const VkAccelerationStructureKHR* pAccelerationStructures, 
	VkQueryType  queryType, 
	size_t       dataSize, 
	void* pData, 
	size_t stride) 
{ 
  return pfn_vkWriteAccelerationStructuresPropertiesKHR(device, accelerationStructureCount, pAccelerationStructures, queryType, dataSize, pData, stride); 
}
//----------------------------------------------------------------------------------------------------------------------
// Acceleration structure functions end here and ray tracing pipeline functions begin here
//----------------------------------------------------------------------------------------------------------------------
void vkCmdSetRayTracingPipelineStackSizeKHR(
	VkCommandBuffer commandBuffer, 
	uint32_t pipelineStackSize) 
{ 
  pfn_vkCmdSetRayTracingPipelineStackSizeKHR(commandBuffer, pipelineStackSize); 
}
void vkCmdTraceRaysIndirectKHR(
	VkCommandBuffer commandBuffer, 
	const VkStridedDeviceAddressRegionKHR* pRaygenShaderBindingTable, 
	const VkStridedDeviceAddressRegionKHR* pMissShaderBindingTable, 
	const VkStridedDeviceAddressRegionKHR* pHitShaderBindingTable, 
	const VkStridedDeviceAddressRegionKHR* pCallableShaderBindingTable, 
	VkDeviceAddress indirectDeviceAddress) 
{ 
  pfn_vkCmdTraceRaysIndirectKHR(commandBuffer, pRaygenShaderBindingTable, pMissShaderBindingTable, pHitShaderBindingTable, pCallableShaderBindingTable, indirectDeviceAddress); 
}
void vkCmdTraceRaysKHR(
	VkCommandBuffer commandBuffer, 
	const VkStridedDeviceAddressRegionKHR* pRaygenShaderBindingTable, 
	const VkStridedDeviceAddressRegionKHR* pMissShaderBindingTable, 
	const VkStridedDeviceAddressRegionKHR* pHitShaderBindingTable, 
	const VkStridedDeviceAddressRegionKHR* pCallableShaderBindingTable, 
	uint32_t width, 
	uint32_t height, 
	uint32_t depth) 
{ 
  pfn_vkCmdTraceRaysKHR(commandBuffer, pRaygenShaderBindingTable, pMissShaderBindingTable, pHitShaderBindingTable, pCallableShaderBindingTable, width, height, depth); 
}
VkResult vkCreateRayTracingPipelinesKHR(
	VkDevice device, 
	VkDeferredOperationKHR deferredOperation, 
	VkPipelineCache pipelineCache, 
	uint32_t createInfoCount, 
	const VkRayTracingPipelineCreateInfoKHR* pCreateInfos, 
	const VkAllocationCallbacks* pAllocator, 
	VkPipeline* pPipelines) 
{ 
  return pfn_vkCreateRayTracingPipelinesKHR(device, deferredOperation, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines); 
}
VkResult vkGetRayTracingCaptureReplayShaderGroupHandlesKHR(
	VkDevice device, 
	VkPipeline pipeline, 
	uint32_t firstGroup, 
	uint32_t groupCount, 
	size_t dataSize, 
	void* pData) 
{ 
  return pfn_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR(device, pipeline, firstGroup, groupCount, dataSize, pData); 
}
VkResult vkGetRayTracingShaderGroupHandlesKHR( VkDevice device, VkPipeline pipeline, uint32_t firstGroup, uint32_t groupCount, size_t dataSize, void* pData) 
{ 
  return pfn_vkGetRayTracingShaderGroupHandlesKHR(device, pipeline, firstGroup, groupCount, dataSize, pData); 
}
VkDeviceSize vkGetRayTracingShaderGroupStackSizeKHR(
	VkDevice device, 
	VkPipeline pipeline, 
	uint32_t group, 
	VkShaderGroupShaderKHR groupShader) 
{ 
  return pfn_vkGetRayTracingShaderGroupStackSizeKHR(device, pipeline, group, groupShader); 
}
//----------------------------------------------------------------------------------------------------------------------
// Ray tracing pipeline functions end here and mesh shading functions begin here
//----------------------------------------------------------------------------------------------------------------------
void vkCmdDrawMeshTasksEXT(
	VkCommandBuffer commandBuffer, 
	uint32_t groupCountX, 
	uint32_t groupCountY, 
	uint32_t groupCountZ) 
{ 
    pfn_vkCmdDrawMeshTasksEXT(commandBuffer, groupCountX, groupCountY, groupCountZ); 
}
void vkCmdDrawMeshTasksIndirectCountEXT(
	VkCommandBuffer commandBuffer, 
	VkBuffer buffer, 
	VkDeviceSize offset, 
	VkBuffer countBuffer, 
	VkDeviceSize countBufferOffset, 
	uint32_t maxDrawCount, 
	uint32_t stride) 
{ 
    pfn_vkCmdDrawMeshTasksIndirectCountEXT(commandBuffer, buffer, offset, countBuffer, countBufferOffset, maxDrawCount, stride); 
}
void vkCmdDrawMeshTasksIndirectEXT(
	VkCommandBuffer commandBuffer, 
	VkBuffer buffer, 
	VkDeviceSize offset, 
	uint32_t drawCount, 
	uint32_t stride) 
{ 
    pfn_vkCmdDrawMeshTasksIndirectEXT(commandBuffer, buffer, offset, drawCount, stride); 
}
//----------------------------------------------------------------------------------------------------------------------
// Mesh shading functions end here and video functions begin here
//----------------------------------------------------------------------------------------------------------------------
VkResult vkGetPhysicalDeviceVideoFormatPropertiesKHR(
        VkPhysicalDevice physicalDevice,
        const VkPhysicalDeviceVideoFormatInfoKHR *pVideoFormatInfo,
        uint32_t *pVideoFormatPropertyCount,
        VkVideoFormatPropertiesKHR *pVideoFormatProperties)
{
    return pfn_vkGetPhysicalDeviceVideoFormatPropertiesKHR(physicalDevice, pVideoFormatInfo, pVideoFormatPropertyCount, pVideoFormatProperties);
}
VkResult vkGetPhysicalDeviceVideoCapabilitiesKHR(
        VkPhysicalDevice physicalDevice,
        const VkVideoProfileInfoKHR *pVideoProfile,
        VkVideoCapabilitiesKHR *pCapabilities)
{
    return pfn_vkGetPhysicalDeviceVideoCapabilitiesKHR(physicalDevice, pVideoProfile, pCapabilities);
}
VkResult vkCreateVideoSessionKHR(VkDevice device,
        const VkVideoSessionCreateInfoKHR *pCreateInfo,
        const VkAllocationCallbacks *pAllocator,
        VkVideoSessionKHR *pVideoSession)
{
    return pfn_vkCreateVideoSessionKHR(device, pCreateInfo, pAllocator, pVideoSession);
}
void vkDestroyVideoSessionKHR(
        VkDevice device,
        VkVideoSessionKHR videoSession,
        const VkAllocationCallbacks *pAllocator)
{
    pfn_vkDestroyVideoSessionKHR(device, videoSession, pAllocator);
}
VkResult vkGetVideoSessionMemoryRequirementsKHR(
        VkDevice device,
        VkVideoSessionKHR videoSession,
        uint32_t *pMemoryRequirementsCount,
        VkVideoSessionMemoryRequirementsKHR *pMemoryRequirements)
{
    return pfn_vkGetVideoSessionMemoryRequirementsKHR(device, videoSession, pMemoryRequirementsCount, pMemoryRequirements);
}

namespace vul {

namespace extensions{

void addAccelerationStructure(VkDevice device, PFN_vkGetDeviceProcAddr getDeviceProcAddr)
{
    pfn_vkBuildAccelerationStructuresKHR = (PFN_vkBuildAccelerationStructuresKHR)getDeviceProcAddr(device, "vkBuildAccelerationStructuresKHR");
    pfn_vkCmdBuildAccelerationStructuresIndirectKHR = (PFN_vkCmdBuildAccelerationStructuresIndirectKHR)getDeviceProcAddr(device, "vkCmdBuildAccelerationStructuresIndirectKHR");
    pfn_vkCmdBuildAccelerationStructuresKHR = (PFN_vkCmdBuildAccelerationStructuresKHR)getDeviceProcAddr(device, "vkCmdBuildAccelerationStructuresKHR");
    pfn_vkCmdCopyAccelerationStructureKHR = (PFN_vkCmdCopyAccelerationStructureKHR)getDeviceProcAddr(device, "vkCmdCopyAccelerationStructureKHR");
    pfn_vkCmdCopyAccelerationStructureToMemoryKHR = (PFN_vkCmdCopyAccelerationStructureToMemoryKHR)getDeviceProcAddr(device, "vkCmdCopyAccelerationStructureToMemoryKHR");
    pfn_vkCmdCopyMemoryToAccelerationStructureKHR = (PFN_vkCmdCopyMemoryToAccelerationStructureKHR)getDeviceProcAddr(device, "vkCmdCopyMemoryToAccelerationStructureKHR");
    pfn_vkCmdWriteAccelerationStructuresPropertiesKHR = (PFN_vkCmdWriteAccelerationStructuresPropertiesKHR)getDeviceProcAddr(device, "vkCmdWriteAccelerationStructuresPropertiesKHR");
    pfn_vkCopyAccelerationStructureKHR = (PFN_vkCopyAccelerationStructureKHR)getDeviceProcAddr(device, "vkCopyAccelerationStructureKHR");
    pfn_vkCopyAccelerationStructureToMemoryKHR = (PFN_vkCopyAccelerationStructureToMemoryKHR)getDeviceProcAddr(device, "vkCopyAccelerationStructureToMemoryKHR");
    pfn_vkCopyMemoryToAccelerationStructureKHR = (PFN_vkCopyMemoryToAccelerationStructureKHR)getDeviceProcAddr(device, "vkCopyMemoryToAccelerationStructureKHR");
    pfn_vkCreateAccelerationStructureKHR = (PFN_vkCreateAccelerationStructureKHR)getDeviceProcAddr(device, "vkCreateAccelerationStructureKHR");
    pfn_vkDestroyAccelerationStructureKHR = (PFN_vkDestroyAccelerationStructureKHR)getDeviceProcAddr(device, "vkDestroyAccelerationStructureKHR");
    pfn_vkGetAccelerationStructureBuildSizesKHR = (PFN_vkGetAccelerationStructureBuildSizesKHR)getDeviceProcAddr(device, "vkGetAccelerationStructureBuildSizesKHR");
    pfn_vkGetAccelerationStructureDeviceAddressKHR = (PFN_vkGetAccelerationStructureDeviceAddressKHR)getDeviceProcAddr(device, "vkGetAccelerationStructureDeviceAddressKHR");
    pfn_vkGetDeviceAccelerationStructureCompatibilityKHR = (PFN_vkGetDeviceAccelerationStructureCompatibilityKHR)getDeviceProcAddr(device, "vkGetDeviceAccelerationStructureCompatibilityKHR");
    pfn_vkWriteAccelerationStructuresPropertiesKHR = (PFN_vkWriteAccelerationStructuresPropertiesKHR)getDeviceProcAddr(device, "vkWriteAccelerationStructuresPropertiesKHR");
}

void addRayTracingPipeline(VkDevice device, PFN_vkGetDeviceProcAddr getDeviceProcAddr)
{
	pfn_vkCmdSetRayTracingPipelineStackSizeKHR = (PFN_vkCmdSetRayTracingPipelineStackSizeKHR)getDeviceProcAddr(device, "vkCmdSetRayTracingPipelineStackSizeKHR");
	pfn_vkCmdTraceRaysIndirectKHR = (PFN_vkCmdTraceRaysIndirectKHR)getDeviceProcAddr(device, "vkCmdTraceRaysIndirectKHR");
	pfn_vkCmdTraceRaysKHR = (PFN_vkCmdTraceRaysKHR)getDeviceProcAddr(device, "vkCmdTraceRaysKHR");
	pfn_vkCreateRayTracingPipelinesKHR = (PFN_vkCreateRayTracingPipelinesKHR)getDeviceProcAddr(device, "vkCreateRayTracingPipelinesKHR");
	pfn_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR = (PFN_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR)getDeviceProcAddr(device, "vkGetRayTracingCaptureReplayShaderGroupHandlesKHR");
	pfn_vkGetRayTracingShaderGroupHandlesKHR = (PFN_vkGetRayTracingShaderGroupHandlesKHR)getDeviceProcAddr(device, "vkGetRayTracingShaderGroupHandlesKHR");
	pfn_vkGetRayTracingShaderGroupStackSizeKHR = (PFN_vkGetRayTracingShaderGroupStackSizeKHR)getDeviceProcAddr(device, "vkGetRayTracingShaderGroupStackSizeKHR");
}

void addMeshShader(VkDevice device, PFN_vkGetDeviceProcAddr getDeviceProcAddr)
{
    pfn_vkCmdDrawMeshTasksEXT = (PFN_vkCmdDrawMeshTasksEXT)getDeviceProcAddr(device, "vkCmdDrawMeshTasksEXT");
    pfn_vkCmdDrawMeshTasksIndirectCountEXT = (PFN_vkCmdDrawMeshTasksIndirectCountEXT)getDeviceProcAddr(device, "vkCmdDrawMeshTasksIndirectCountEXT");
    pfn_vkCmdDrawMeshTasksIndirectEXT = (PFN_vkCmdDrawMeshTasksIndirectEXT)getDeviceProcAddr(device, "vkCmdDrawMeshTasksIndirectEXT");
}

void addVideo(VkInstance instance, PFN_vkGetInstanceProcAddr getInstanceProcAddr)
{
    pfn_vkGetPhysicalDeviceVideoFormatPropertiesKHR = (PFN_vkGetPhysicalDeviceVideoFormatPropertiesKHR)getInstanceProcAddr(instance, "vkGetPhysicalDeviceVideoFormatPropertiesKHR");
    pfn_vkGetPhysicalDeviceVideoCapabilitiesKHR = (PFN_vkGetPhysicalDeviceVideoCapabilitiesKHR)getInstanceProcAddr(instance, "vkGetPhysicalDeviceVideoCapabilitiesKHR");
    pfn_vkCreateVideoSessionKHR = (PFN_vkCreateVideoSessionKHR)getInstanceProcAddr(instance, "vkCreateVideoSessionKHR");
    pfn_vkDestroyVideoSessionKHR = (PFN_vkDestroyVideoSessionKHR)getInstanceProcAddr(instance, "vkDestroyVideoSessionKHR");
    pfn_vkGetVideoSessionMemoryRequirementsKHR = (PFN_vkGetVideoSessionMemoryRequirementsKHR)getInstanceProcAddr(instance, "vkGetVideoSessionMemoryRequirementsKHR");
    pfn_vkBindVideoSessionMemoryKHR = (PFN_vkBindVideoSessionMemoryKHR)getInstanceProcAddr(instance, "vkBindVideoSessionMemoryKHR");
}

}
}
