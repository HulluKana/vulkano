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
#include <string>
#include <vk_video/vulkan_video_codec_h264std.h>
#include <vul_device.hpp>

#include <iostream>
#include <vulkan/vulkan_core.h>

template <typename T>
T swapEndian(T data)
{
    static_assert (CHAR_BIT == 8, "CHAR_BIT != 8");
    union {
        T data;
        uint8_t dataChars[sizeof(T)];
    } source, dest;
    source.data = data;
    for (size_t i = 0; i < sizeof(T); i++) dest.dataChars[i] = source.dataChars[sizeof(T) - i - 1];
    return dest.data;
}

template <typename T>
T readValue(const std::vector<uint8_t> &data, size_t offset)
{
    return swapEndian(*reinterpret_cast<const T *>(&data[offset]));
}

bool compare4byteStr(char *str, const char *testStr)
{
    return str[0] == testStr[0] && str[1] == testStr[1] && str[2] == testStr[2] && str[3] == testStr[3];
}

void getAndValidate4byteStr(const std::vector<uint8_t> &data, size_t offset, const char *testStr, const char *errStr)
{
    char str[4];
    memcpy(str, &data[offset], 4);
    if (!compare4byteStr(str, testStr)) throw std::runtime_error(errStr);
}

size_t checkBoxValidityAndReturnSize(const std::vector<uint8_t> &data, size_t offset, const char *testStr, const char *errStr)
{
    uint32_t boxSize = readValue<uint32_t>(data, offset);
    getAndValidate4byteStr(data, offset + 4, testStr, errStr);
    return boxSize;
}

size_t findBoxOffset(const std::vector<uint8_t> &data, size_t startOffset, size_t endOffset, const char *boxName)
{
    for (size_t i = startOffset; i < endOffset;) {
        uint32_t boxSize = readValue<uint32_t>(data, i);
        if (boxSize == 0) throw std::runtime_error("Box size is 0\n");
        char str[4];
        memcpy(str, &data[i + 4], 4);
        if (compare4byteStr(str, boxName)) return i;
        i += boxSize;
    }
    throw std::runtime_error(std::string("Did not find a box called ") + boxName);
}

int main() {
    std::ifstream inputFile("../badAppleVideo.mp4", std::ios::binary);
    if (!inputFile.is_open()) throw std::runtime_error("Failed to open video file\n");
    std::vector<uint8_t> inputData(std::istreambuf_iterator<char>(inputFile), {});
    size_t index = 0;
    index += checkBoxValidityAndReturnSize(inputData, index, "ftyp", "Not a valid mp4 file\n");
    checkBoxValidityAndReturnSize(inputData, index, "moov", "Expected moov\n");
    index += 8;
    index += checkBoxValidityAndReturnSize(inputData, index, "mvhd", "Expected mvhd\n");
    checkBoxValidityAndReturnSize(inputData, index, "trak", "Expected trak\n");
    index += 8;
    index += checkBoxValidityAndReturnSize(inputData, index, "tkhd", "Expected tkhd\n");
    checkBoxValidityAndReturnSize(inputData, index, "edts", "Expected edts\n");
    index += 8;
    index += checkBoxValidityAndReturnSize(inputData, index, "elst", "Expected elst\n");
    checkBoxValidityAndReturnSize(inputData, index, "mdia", "Expected mdia\n");
    index += 8;
    index += checkBoxValidityAndReturnSize(inputData, index, "mdhd", "Expected mdhd\n");
    index += checkBoxValidityAndReturnSize(inputData, index, "hdlr", "Expected hdlr\n");
    checkBoxValidityAndReturnSize(inputData, index, "minf", "Expected minf\n");
    index += 8;
    index += checkBoxValidityAndReturnSize(inputData, index, "vmhd", "Expected vmhd\n");
    checkBoxValidityAndReturnSize(inputData, index, "dinf", "Expected dinf\n");
    index += 8;
    index += checkBoxValidityAndReturnSize(inputData, index, "dref", "Expected dref\n");
    checkBoxValidityAndReturnSize(inputData, index, "stbl", "Expected stbl\n");
    index += 8;
    size_t stsdEndIndex = checkBoxValidityAndReturnSize(inputData, index, "stsd", "Expected stsd\n") + index;
    index += 12;
    if (readValue<uint32_t>(inputData, index) != 1) throw std::runtime_error("Description count isnt 1\n");
    index += 8;
    getAndValidate4byteStr(inputData, index, "avc1", "Expected avc1\n"); 
    index += 80;
    if (readValue<int16_t>(inputData, index) != -1) throw std::runtime_error("Quicktime table exists\n");
    index += 2;
    index = findBoxOffset(inputData, index, stsdEndIndex, "avcC");
    index += 12;
    /*uint32_t nalLength = (inputData[index++] & 0b11) + 1*/index++;
    if ((inputData[index++] & 0b11111) != 1) throw std::runtime_error("SPS count isnt 1\n");
    uint32_t spsSize = readValue<uint16_t>(inputData, index);
    index += spsSize + 2;
    if (inputData[index++] != 1) throw std::runtime_error("PPS count isnt 1\n");
    //uint32_t ppsSize = readValue<uint16_t>(inputData, index);


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
 
    vulDevice.waitForIdle();
    vkDestroyVideoSessionParametersKHR(vulDevice.device(), sessionParameters, nullptr);
    for (const VkBindVideoSessionMemoryInfoKHR &bindInfo : videoSessionMemoryBinds) vkFreeMemory(vulDevice.device(), bindInfo.memory, nullptr);
    vkDestroyVideoSessionKHR(vulDevice.device(), videoSession, nullptr);

    return 0;
}
