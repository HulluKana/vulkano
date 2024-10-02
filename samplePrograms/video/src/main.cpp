#include "vul_command_pool.hpp"
#include "vul_extensions.hpp"
#include "vul_window.hpp"
#include <bit>
#include <cassert>
#include <cstring>
#include <fstream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <climits>
#include <vk_video/vulkan_video_codec_h264std.h>
#include <vul_device.hpp>

#include <iostream>
#include <vulkan/vulkan_core.h>

template <typename T>
T swapEndian(T data)
{
    static_assert (CHAR_BIT == 8, "CHAR_BIT != 8");

    union
    {
        T data;
        uint8_t dataChars[sizeof(T)];
    } source, dest;

    source.data = data;

    for (size_t i = 0; i < sizeof(T); i++)
        dest.dataChars[i] = source.dataChars[sizeof(T) - i - 1];

    return dest.data;
}

int main() {
    vul::VulWindow vulWindow(2560, 1440, "Vulkano video");
    vul::VulDevice vulDevice(vulWindow, 0, false, false, true);
    vul::VulCmdPool cmdPool(vul::VulCmdPool::QueueType::main, 0, 0, vulDevice);

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

    VkVideoDecodeH264CapabilitiesKHR videoDecodeH264Capabilities{};
    videoDecodeH264Capabilities.sType = VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_CAPABILITIES_KHR;
    VkVideoDecodeCapabilitiesKHR videoDecodeCapabilities{};
    videoDecodeCapabilities.sType = VK_STRUCTURE_TYPE_VIDEO_DECODE_CAPABILITIES_KHR;
    videoDecodeCapabilities.pNext = &videoDecodeH264Capabilities;
    VkVideoCapabilitiesKHR videoCapabilities{};
    videoCapabilities.sType = VK_STRUCTURE_TYPE_VIDEO_CAPABILITIES_KHR;
    videoCapabilities.pNext = &videoDecodeCapabilities;
    result = vkGetPhysicalDeviceVideoCapabilitiesKHR(vulDevice.getPhysicalDevice(), &videoProfileInfo, &videoCapabilities);

    //std::cout << videoDecodeH264Capabilities.fieldOffsetGranularity.x << " " << videoDecodeH264Capabilities.fieldOffsetGranularity.y << " " << videoDecodeH264Capabilities.maxLevelIdc << " " << videoDecodeCapabilities.flags << " " << videoCapabilities.minCodedExtent.width << " " << videoCapabilities.minCodedExtent.height << " " << videoCapabilities.maxCodedExtent.width << " " << videoCapabilities.maxCodedExtent.height << " " << videoCapabilities.minBitstreamBufferOffsetAlignment << " " << videoCapabilities.minBitstreamBufferSizeAlignment << " " << videoCapabilities.stdHeaderVersion.specVersion << " " << videoCapabilities.stdHeaderVersion.extensionName << " " << videoCapabilities.maxDpbSlots << " " << videoCapabilities.pictureAccessGranularity.width << " " << videoCapabilities.pictureAccessGranularity.height << " " << videoCapabilities.maxActiveReferencePictures << "\n";

    VkVideoSessionCreateInfoKHR videoSessionCreateInfo{};
    videoSessionCreateInfo.sType = VK_STRUCTURE_TYPE_VIDEO_SESSION_CREATE_INFO_KHR;
    videoSessionCreateInfo.queueFamilyIndex = vulDevice.getQueueFamilies().videoDecodeFamily;
    videoSessionCreateInfo.flags = 0;
    videoSessionCreateInfo.pVideoProfile = &videoProfileInfo;
    videoSessionCreateInfo.pictureFormat = videoFormatProperties[0].format;
    videoSessionCreateInfo.maxCodedExtent = VkExtent2D{480, 360};
    videoSessionCreateInfo.referencePictureFormat = videoFormatProperties[0].format;
    videoSessionCreateInfo.maxDpbSlots = 1;
    videoSessionCreateInfo.maxActiveReferencePictures = 1;
    videoSessionCreateInfo.pStdHeaderVersion = &videoCapabilities.stdHeaderVersion;
    VkVideoSessionKHR videoSession;
    result = vkCreateVideoSessionKHR(vulDevice.device(), &videoSessionCreateInfo, nullptr, &videoSession);
    assert(result == VK_SUCCESS);

    uint32_t videoMemReqCount;
    vkGetVideoSessionMemoryRequirementsKHR(vulDevice.device(), videoSession, &videoMemReqCount, nullptr);
    assert(videoMemReqCount > 0);
    std::vector<VkVideoSessionMemoryRequirementsKHR> videoSessionMemReqs(videoMemReqCount);
    for (uint32_t i = 0; i < videoMemReqCount; i++) videoSessionMemReqs[i].sType = VK_STRUCTURE_TYPE_VIDEO_SESSION_MEMORY_REQUIREMENTS_KHR;
    vkGetVideoSessionMemoryRequirementsKHR(vulDevice.device(), videoSession, &videoMemReqCount, videoSessionMemReqs.data());

    std::vector<VkBindVideoSessionMemoryInfoKHR> videoSessionMemoryBinds(videoMemReqCount);
    for (uint32_t i = 0; i < videoMemReqCount; i++) {
        VkMemoryAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocateInfo.memoryTypeIndex = vulDevice.findMemoryType(videoSessionMemReqs[i].memoryRequirements.memoryTypeBits, 0);
        allocateInfo.allocationSize = videoSessionMemReqs[i].memoryRequirements.size;
        vkAllocateMemory(vulDevice.device(), &allocateInfo, nullptr, &videoSessionMemoryBinds[i].memory);

        videoSessionMemoryBinds[i].sType = VK_STRUCTURE_TYPE_BIND_VIDEO_SESSION_MEMORY_INFO_KHR;
        videoSessionMemoryBinds[i].memoryBindIndex = videoSessionMemReqs[i].memoryBindIndex;
        videoSessionMemoryBinds[i].memoryOffset = 0;
        videoSessionMemoryBinds[i].memorySize = videoSessionMemReqs[i].memoryRequirements.size;
    }
    result = vkBindVideoSessionMemoryKHR(vulDevice.device(), videoSession, videoMemReqCount, videoSessionMemoryBinds.data());
    assert(result == VK_SUCCESS);

    StdVideoH264SequenceParameterSet h264sps{};
    h264sps.seq_parameter_set_id = 0;
    h264sps.level_idc = videoDecodeH264Capabilities.maxLevelIdc;
    StdVideoH264PictureParameterSet h264pps{};
    h264pps.seq_parameter_set_id = 0;
    h264pps.pic_parameter_set_id = 0;
    VkVideoDecodeH264SessionParametersAddInfoKHR h264DecodeParametersAddInfo{};
    h264DecodeParametersAddInfo.sType = VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_SESSION_PARAMETERS_ADD_INFO_KHR;
    h264DecodeParametersAddInfo.pStdSPSs = &h264sps;
    h264DecodeParametersAddInfo.pStdPPSs = &h264pps;
    h264DecodeParametersAddInfo.stdSPSCount = 1;
    h264DecodeParametersAddInfo.stdPPSCount = 1;
    VkVideoDecodeH264SessionParametersCreateInfoKHR h264DecodeParametersInfo{};
    h264DecodeParametersInfo.sType = VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_SESSION_PARAMETERS_CREATE_INFO_KHR;
    h264DecodeParametersInfo.maxStdSPSCount = 1;
    h264DecodeParametersInfo.maxStdPPSCount = 1;
    h264DecodeParametersInfo.pParametersAddInfo = &h264DecodeParametersAddInfo;
    VkVideoSessionParametersCreateInfoKHR sessionParametersInfo{};
    sessionParametersInfo.sType = VK_STRUCTURE_TYPE_VIDEO_SESSION_PARAMETERS_CREATE_INFO_KHR;
    sessionParametersInfo.pNext = &h264DecodeParametersInfo;
    sessionParametersInfo.videoSession = videoSession;
    VkVideoSessionParametersKHR sessionParameters;
    result = vkCreateVideoSessionParametersKHR(vulDevice.device(), &sessionParametersInfo, nullptr, &sessionParameters);
    assert(result == VK_SUCCESS);

    
    std::ifstream inputFile("../badAppleVideo.mp4", std::ios::binary);
    if (!inputFile.is_open()) throw std::runtime_error("Failed to open video file\n");
    std::vector<uint8_t> inputData(std::istreambuf_iterator<char>(inputFile), {});
    uint32_t identificationBoxSize = swapEndian(*reinterpret_cast<uint32_t *>(&inputData[0]));
    char identifierString[4];
    memcpy(identifierString, &inputData[4], 4);
    if (identifierString[0] != 'f' || identifierString[1] != 't' || identifierString[2] != 'y' || identifierString[3] != 'p')
        throw std::runtime_error("Not a valid mp4 file\n");
    uint32_t mediaHeaderBoxSize = swapEndian(*reinterpret_cast<uint32_t *>(&inputData[identificationBoxSize]));
    char mediaHeaderString[4];
    memcpy(mediaHeaderString, &inputData[identificationBoxSize + 4], 4);
    if (mediaHeaderString[0] != 'm' || mediaHeaderString[1] != 'o' || mediaHeaderString[2] != 'o' || mediaHeaderString[3] != 'v')
        throw std::runtime_error("Cant sparse this mp4 file\n");

    vulDevice.waitForIdle();
    vkDestroyVideoSessionParametersKHR(vulDevice.device(), sessionParameters, nullptr);
    for (const VkBindVideoSessionMemoryInfoKHR &bindInfo : videoSessionMemoryBinds) vkFreeMemory(vulDevice.device(), bindInfo.memory, nullptr);
    vkDestroyVideoSessionKHR(vulDevice.device(), videoSession, nullptr);

    return 0;
}
