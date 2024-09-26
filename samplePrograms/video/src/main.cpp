#include "vul_extensions.hpp"
#include "vul_window.hpp"
#include <cassert>
#include <limits>
#include <vk_video/vulkan_video_codec_h264std.h>
#include <vul_device.hpp>

#include <iostream>
#include <vulkan/vulkan_core.h>

int main() {
    vul::VulWindow vulWindow(2560, 1440, "Vulkano video");
    vul::VulDevice vulDevice(vulWindow, 0, false, false);
    vul::extensions::addVideo(vulDevice.getInstace(), vkGetInstanceProcAddr);

    VkVideoDecodeH264ProfileInfoKHR videoDecodeH264ProfileInfo{};
    videoDecodeH264ProfileInfo.sType = VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_PROFILE_INFO_KHR;
    videoDecodeH264ProfileInfo.stdProfileIdc = STD_VIDEO_H264_PROFILE_IDC_HIGH;
    videoDecodeH264ProfileInfo.pictureLayout = VK_VIDEO_DECODE_H264_PICTURE_LAYOUT_PROGRESSIVE_KHR;
    VkVideoProfileInfoKHR videoProfileInfo{};
    videoProfileInfo.sType = VK_STRUCTURE_TYPE_VIDEO_PROFILE_INFO_KHR;
    videoProfileInfo.pNext = &videoDecodeH264ProfileInfo;
    videoProfileInfo.lumaBitDepth = VK_VIDEO_COMPONENT_BIT_DEPTH_8_BIT_KHR;
    videoProfileInfo.chromaBitDepth = VK_VIDEO_COMPONENT_BIT_DEPTH_8_BIT_KHR;
    videoProfileInfo.chromaSubsampling = VK_VIDEO_CHROMA_SUBSAMPLING_420_BIT_KHR;
    videoProfileInfo.videoCodecOperation = VK_VIDEO_CODEC_OPERATION_DECODE_H264_BIT_KHR;
    VkVideoProfileListInfoKHR videoProfileListInfo{};
    videoProfileListInfo.sType = VK_STRUCTURE_TYPE_VIDEO_PROFILE_LIST_INFO_KHR;
    videoProfileListInfo.pProfiles = &videoProfileInfo;
    videoProfileListInfo.profileCount = 1;
    VkPhysicalDeviceVideoFormatInfoKHR videoFormatInfo{};
    videoFormatInfo.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VIDEO_FORMAT_INFO_KHR;
    videoFormatInfo.pNext = &videoProfileListInfo;
    videoFormatInfo.imageUsage = VK_IMAGE_USAGE_VIDEO_DECODE_DST_BIT_KHR | VK_IMAGE_USAGE_SAMPLED_BIT;

    uint32_t videoFormatPropertyCount = 0;
    VkResult result = vkGetPhysicalDeviceVideoFormatPropertiesKHR(vulDevice.getPhysicalDevice(), &videoFormatInfo, &videoFormatPropertyCount, nullptr);
    assert(result == VK_SUCCESS && videoFormatPropertyCount == 1);
    std::vector<VkVideoFormatPropertiesKHR> videoFormatProperties(videoFormatPropertyCount);
    for (uint32_t i = 0; i < videoFormatPropertyCount; i++) videoFormatProperties[i].sType = VK_STRUCTURE_TYPE_VIDEO_FORMAT_PROPERTIES_KHR;
    result = vkGetPhysicalDeviceVideoFormatPropertiesKHR(vulDevice.getPhysicalDevice(), &videoFormatInfo, &videoFormatPropertyCount, videoFormatProperties.data());
    assert(result == VK_SUCCESS);

    std::cout << videoFormatProperties[0].format << "\n";

    VkVideoDecodeH264CapabilitiesKHR videoDecodeH264Capabilities{};
    videoDecodeH264Capabilities.sType = VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_CAPABILITIES_KHR;
    VkVideoDecodeCapabilitiesKHR videoDecodeCapabilities{};
    videoDecodeCapabilities.sType = VK_STRUCTURE_TYPE_VIDEO_DECODE_CAPABILITIES_KHR;
    videoDecodeCapabilities.pNext = &videoDecodeH264Capabilities;
    VkVideoCapabilitiesKHR videoCapabilities{};
    videoCapabilities.sType = VK_STRUCTURE_TYPE_VIDEO_CAPABILITIES_KHR;
    videoCapabilities.pNext = &videoDecodeCapabilities;
    result = vkGetPhysicalDeviceVideoCapabilitiesKHR(vulDevice.getPhysicalDevice(), &videoProfileInfo, &videoCapabilities);
    std::cout << result << "\n";

    std::cout << videoDecodeH264Capabilities.fieldOffsetGranularity.x << " " << videoDecodeH264Capabilities.fieldOffsetGranularity.y << " " << videoDecodeH264Capabilities.maxLevelIdc << " " << videoDecodeCapabilities.flags << " " << videoCapabilities.minCodedExtent.width << " " << videoCapabilities.minCodedExtent.height << " " << videoCapabilities.maxCodedExtent.width << " " << videoCapabilities.maxCodedExtent.height << " " << videoCapabilities.minBitstreamBufferOffsetAlignment << " " << videoCapabilities.minBitstreamBufferSizeAlignment << " " << videoCapabilities.stdHeaderVersion.specVersion << " " << videoCapabilities.stdHeaderVersion.extensionName << " " << videoCapabilities.maxDpbSlots << " " << videoCapabilities.pictureAccessGranularity.width << " " << videoCapabilities.pictureAccessGranularity.height << " " << videoCapabilities.maxActiveReferencePictures << "\n";

    return 0;
}
