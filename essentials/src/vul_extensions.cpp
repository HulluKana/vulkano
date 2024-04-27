#include<vul_extensions.hpp>

using namespace vulB::extensions;

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
// Ray tracing pipeline functions end here

namespace vulB {

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

}
}
