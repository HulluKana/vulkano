#include "imgui.h"
#include "vul_GUI.hpp"
#include "vul_buffer.hpp"
#include "vul_command_pool.hpp"
#include "vul_descriptors.hpp"
#include "vul_extensions.hpp"
#include "vul_image.hpp"
#include "vul_renderer.hpp"
#include "vul_window.hpp"
#include <GLFW/glfw3.h>
#include <bit>
#include <cassert>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <climits>
#include <string>
#include <thread>
#include <vk_video/vulkan_video_codec_h264std.h>
#include <vk_video/vulkan_video_codec_h264std_decode.h>
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

template <typename T>
T readValueAndProceed(const std::vector<uint8_t> &data, size_t &offset)
{
    T value = swapEndian(*reinterpret_cast<const T *>(&data[offset]));
    offset += sizeof(T);
    return value;
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

size_t checkBoxValidityReturnSizeAndProceed(const std::vector<uint8_t> &data, size_t &offset, const char *testStr, const char *errStr)
{
    uint32_t size = checkBoxValidityAndReturnSize(data, offset, testStr, errStr);
    offset += 8;
    return size;
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

uint32_t alignUp(uint32_t alignee, uint32_t aligner)
{
    return (alignee % aligner) ? alignee + (aligner - (alignee % aligner)) : alignee;
}

struct Block {
    uint32_t nextBlock;
    uint32_t frameCount;
    uint32_t samplesDescriptionId;
};

int main() {
    std::ifstream inputFile("../badAppleVideo.mp4", std::ios::binary);
    if (!inputFile.is_open()) throw std::runtime_error("Failed to open video file\n");
    std::vector<uint8_t> inputData(std::istreambuf_iterator<char>(inputFile), {});
    size_t index = 0;
    index += checkBoxValidityAndReturnSize(inputData, index, "ftyp", "Not a valid mp4 file\n");
    const size_t freeIndex = index + checkBoxValidityAndReturnSize(inputData, index, "moov", "Expected moov\n");
    index += 8;
    index += checkBoxValidityAndReturnSize(inputData, index, "mvhd", "Expected mvhd\n");
    checkBoxValidityReturnSizeAndProceed(inputData, index, "trak", "Expected trak\n");
    index += checkBoxValidityAndReturnSize(inputData, index, "tkhd", "Expected tkhd\n");
    checkBoxValidityReturnSizeAndProceed(inputData, index, "edts", "Expected edts\n");
    index += checkBoxValidityAndReturnSize(inputData, index, "elst", "Expected elst\n");
    checkBoxValidityReturnSizeAndProceed(inputData, index, "mdia", "Expected mdia\n");
    const size_t hdlrIndex = index + checkBoxValidityAndReturnSize(inputData, index, "mdhd", "Expected mdhd\n");
    index += 8;
    if (readValue<uint32_t>(inputData, index++) != 0) throw std::runtime_error("Version is not 0\n");
    index += 11;
    const uint32_t tickRate = readValue<uint32_t>(inputData, index);
    index += 4;
    const uint32_t duration = readValue<uint32_t>(inputData, index);
    index = hdlrIndex + checkBoxValidityAndReturnSize(inputData, hdlrIndex, "hdlr", "Expected hdlr\n");
    checkBoxValidityReturnSizeAndProceed(inputData, index, "minf", "Expected minf\n");
    index += checkBoxValidityAndReturnSize(inputData, index, "vmhd", "Expected vmhd\n");
    checkBoxValidityReturnSizeAndProceed(inputData, index, "dinf", "Expected dinf\n");
    index += checkBoxValidityAndReturnSize(inputData, index, "dref", "Expected dref\n");
    checkBoxValidityReturnSizeAndProceed(inputData, index, "stbl", "Expected stbl\n");
    const size_t stsdEndIndex = checkBoxValidityAndReturnSize(inputData, index, "stsd", "Expected stsd\n") + index;
    index += 12;
    if (readValueAndProceed<uint32_t>(inputData, index) != 1) throw std::runtime_error("Description count isnt 1\n");
    index += 4;
    getAndValidate4byteStr(inputData, index, "avc1", "Expected avc1\n"); 
    index += 80;
    if (readValueAndProceed<int16_t>(inputData, index) != -1) throw std::runtime_error("Quicktime table exists\n");
    index = findBoxOffset(inputData, index, stsdEndIndex, "avcC");
    index += 12;
    /*uint32_t nalLength = (inputData[index++] & 0b11) + 1*/index++;
    if ((inputData[index++] & 0b11111) != 1) throw std::runtime_error("SPS count isnt 1\n");
    const uint32_t spsSize = readValueAndProceed<uint16_t>(inputData, index);
    index += spsSize;
    if (inputData[index++] != 1) throw std::runtime_error("PPS count isnt 1\n");
    //uint32_t ppsSize = readValue<uint16_t>(inputData, index);
    index = stsdEndIndex;
    checkBoxValidityReturnSizeAndProceed(inputData, index, "stts", "Expected stts\n");
    index += 4;
    if (readValueAndProceed<uint32_t>(inputData, index) != 1) throw std::runtime_error("Time count isn't 1\n");
    const uint32_t frameCount = readValueAndProceed<uint32_t>(inputData, index);
    const uint32_t frameDuration = readValueAndProceed<uint32_t>(inputData, index);
    checkBoxValidityReturnSizeAndProceed(inputData, index, "stss", "Expected stss\n");
    index += 4;
    const uint32_t keyFrameCount = readValueAndProceed<uint32_t>(inputData, index);
    std::vector<uint32_t> keyFrameIndices(keyFrameCount);
    for (uint32_t i = 0; i < keyFrameCount; i++) {
        const uint32_t keyFrameIdx = readValueAndProceed<uint32_t>(inputData, index);
        keyFrameIndices[i] = keyFrameIdx;
    }
    index += checkBoxValidityAndReturnSize(inputData, index, "ctts", "Expected ctts\n");
    checkBoxValidityReturnSizeAndProceed(inputData, index, "stsc", "Expected stsc\n");
    index += 4;
    const uint32_t blockCount = readValueAndProceed<uint32_t>(inputData, index);
    std::vector<Block> blocks(blockCount);
    for (uint32_t i = 0; i < blockCount; i++) {
        const uint32_t nextBlock = readValueAndProceed<uint32_t>(inputData, index);
        const uint32_t frameCount = readValueAndProceed<uint32_t>(inputData, index);
        const uint32_t samplesDescriptionId = readValueAndProceed<uint32_t>(inputData, index);
        blocks[i] = {.nextBlock = nextBlock, .frameCount = frameCount, .samplesDescriptionId = samplesDescriptionId};
    }
    checkBoxValidityReturnSizeAndProceed(inputData, index, "stsz", "Expected stsz\n");
    index += 8;
    const uint32_t blockSizeCount = readValueAndProceed<uint32_t>(inputData, index);
    std::vector<uint32_t> blockSizes(blockSizeCount);
    for (uint32_t i = 0; i < blockSizeCount; i++) blockSizes[i] = readValueAndProceed<uint32_t>(inputData, index);
    checkBoxValidityReturnSizeAndProceed(inputData, index, "stco", "Expected stco\n");
    index += 4;
    const uint32_t blockOffsetCount = readValueAndProceed<uint32_t>(inputData, index);
    if (blockSizeCount != blockOffsetCount + 1) throw std::runtime_error("Block offset count is not one less than block size count\n");
    std::vector<uint32_t> blockOffsets(blockOffsetCount);
    for (uint32_t i = 0; i < blockOffsetCount; i++) blockOffsets[i] = readValueAndProceed<uint32_t>(inputData, index);
    index = freeIndex + checkBoxValidityAndReturnSize(inputData, freeIndex, "free", "Expected free\n");
    size_t mdatSize = checkBoxValidityAndReturnSize(inputData, index, "mdat", "expected mdat\n");
    if (mdatSize <= 1) throw std::runtime_error("mdat size is less than or equal 1\n");



    vul::VulWindow vulWindow(2560, 1440, "Vulkano video");
    vul::VulDevice vulDevice(vulWindow, 0, false, false, true);
    vul::VulCmdPool decodeCmdPool(vul::VulCmdPool::QueueType::videoDecode, 0, 0, vulDevice);
    vul::VulCmdPool cmdPool(vul::VulCmdPool::QueueType::main, 0, 0, vulDevice);
    std::unique_ptr<vul::VulDescriptorPool> descPool = vul::VulDescriptorPool::Builder(vulDevice).setMaxSets(2).setPoolFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT).build();
    vul::VulRenderer vulRenderer(vulWindow, vulDevice, nullptr);
    vul::VulGUI vulGUi(vulWindow.getGLFWwindow(), descPool->getDescriptorPoolReference(), vulRenderer, vulDevice, cmdPool);

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

    VkCommandBuffer cmdBuf = decodeCmdPool.getPrimaryCommandBuffer();
    uint32_t blockIdx = keyFrameIndices[keyFrameIndices.size() / 2];
    uint32_t maxBlockSize = 0;
    for (uint32_t blockSize : blockSizes) maxBlockSize = std::max(maxBlockSize, blockSize);
    maxBlockSize = alignUp(maxBlockSize, videoCapabilities.minBitstreamBufferSizeAlignment);
    vul::VulBuffer srcBuffer(1, maxBlockSize, false, VK_BUFFER_USAGE_VIDEO_DECODE_SRC_BIT_KHR | VK_BUFFER_USAGE_TRANSFER_DST_BIT, vulDevice);
    uint32_t alignedBlockSize =  alignUp(blockSizes[blockIdx], videoCapabilities.minBitstreamBufferSizeAlignment);
    vkCmdFillBuffer(cmdBuf, srcBuffer.getBuffer(), 0, maxBlockSize, 0);
    srcBuffer.writeData(&inputData[blockOffsets[blockIdx]], blockSizes[blockIdx], 0, VK_NULL_HANDLE);

    vul::VulImage dstImg(vulDevice);
    dstImg.keepEmpty(480, 360, 1, 1, 1, VK_FORMAT_G8_B8R8_2PLANE_420_UNORM);
    dstImg.createCustomImage(VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_LAYOUT_VIDEO_DECODE_DPB_KHR, VK_IMAGE_USAGE_VIDEO_DECODE_DPB_BIT_KHR | VK_IMAGE_USAGE_VIDEO_DECODE_DST_BIT_KHR | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, cmdBuf);
 
    VkVideoPictureResourceInfoKHR referencePictureResourceInfo{};
    referencePictureResourceInfo.sType = VK_STRUCTURE_TYPE_VIDEO_PICTURE_RESOURCE_INFO_KHR;
    referencePictureResourceInfo.codedExtent = {.width = 480, .height = 360};
    referencePictureResourceInfo.codedOffset = {0, 0};
    referencePictureResourceInfo.baseArrayLayer = 0;
    referencePictureResourceInfo.imageViewBinding = dstImg.getImageView();
    VkVideoReferenceSlotInfoKHR referenceSlotInfo{};
    referenceSlotInfo.sType = VK_STRUCTURE_TYPE_VIDEO_REFERENCE_SLOT_INFO_KHR;
    referenceSlotInfo.slotIndex = 0;
    referenceSlotInfo.pPictureResource = &referencePictureResourceInfo;
    VkVideoBeginCodingInfoKHR videoCodingBeginInfo{};
    videoCodingBeginInfo.sType = VK_STRUCTURE_TYPE_VIDEO_BEGIN_CODING_INFO_KHR;
    videoCodingBeginInfo.videoSession = videoSession;
    videoCodingBeginInfo.videoSessionParameters = sessionParameters;
    videoCodingBeginInfo.referenceSlotCount = 1;
    videoCodingBeginInfo.pReferenceSlots = &referenceSlotInfo;
    vkCmdBeginVideoCodingKHR(cmdBuf, &videoCodingBeginInfo);

    VkVideoCodingControlInfoKHR codingControlInfo{};
    codingControlInfo.sType = VK_STRUCTURE_TYPE_VIDEO_CODING_CONTROL_INFO_KHR;
    codingControlInfo.flags = VK_VIDEO_CODING_CONTROL_RESET_BIT_KHR;
    vkCmdControlVideoCodingKHR(cmdBuf, &codingControlInfo);

    StdVideoDecodeH264PictureInfo stdDecodeH264PictureInfo{};
    stdDecodeH264PictureInfo.flags.is_reference = true;
    VkVideoDecodeH264PictureInfoKHR decodeH264PictureInfo{};
    decodeH264PictureInfo.sType = VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_PICTURE_INFO_KHR;
    decodeH264PictureInfo.pStdPictureInfo = &stdDecodeH264PictureInfo;
    decodeH264PictureInfo.sliceCount = 1;
    uint32_t sliceOffset = 0;
    decodeH264PictureInfo.pSliceOffsets = &sliceOffset;
    VkVideoPictureResourceInfoKHR dstPictureResourceInfo{};
    dstPictureResourceInfo.sType = VK_STRUCTURE_TYPE_VIDEO_PICTURE_RESOURCE_INFO_KHR;
    dstPictureResourceInfo.codedExtent = {.width = 480, .height = 360};
    dstPictureResourceInfo.codedOffset = {0, 0};
    dstPictureResourceInfo.baseArrayLayer = 0;
    dstPictureResourceInfo.imageViewBinding = dstImg.getImageView();
    StdVideoDecodeH264ReferenceInfo stdDecodeH264ReferenceInfo{};
    VkVideoDecodeH264DpbSlotInfoKHR decodeH264DpbSlotInfo{};
    decodeH264DpbSlotInfo.sType = VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_DPB_SLOT_INFO_KHR;
    decodeH264DpbSlotInfo.pStdReferenceInfo = &stdDecodeH264ReferenceInfo;
    VkVideoReferenceSlotInfoKHR setupReferenceSlotInfo{};
    setupReferenceSlotInfo.pNext = &decodeH264DpbSlotInfo;
    setupReferenceSlotInfo.sType = VK_STRUCTURE_TYPE_VIDEO_REFERENCE_SLOT_INFO_KHR;
    setupReferenceSlotInfo.slotIndex = 0;
    setupReferenceSlotInfo.pPictureResource = &dstPictureResourceInfo;
    VkVideoDecodeInfoKHR decodeInfo{};
    decodeInfo.sType = VK_STRUCTURE_TYPE_VIDEO_DECODE_INFO_KHR;
    decodeInfo.pNext = &decodeH264PictureInfo;
    decodeInfo.referenceSlotCount = 0;
    decodeInfo.srcBuffer = srcBuffer.getBuffer();
    decodeInfo.srcBufferRange = alignedBlockSize;
    decodeInfo.dstPictureResource = dstPictureResourceInfo;
    decodeInfo.pSetupReferenceSlot = &setupReferenceSlotInfo;
    vkCmdDecodeVideoKHR(cmdBuf, &decodeInfo);

    VkVideoEndCodingInfoKHR videoCodingEndInfo{};
    videoCodingEndInfo.sType = VK_STRUCTURE_TYPE_VIDEO_END_CODING_INFO_KHR;
    vkCmdEndVideoCodingKHR(cmdBuf, &videoCodingEndInfo);
    decodeCmdPool.submit(cmdBuf, true);

    cmdBuf = decodeCmdPool.getPrimaryCommandBuffer();
    dstImg.transitionImageLayout(VK_IMAGE_LAYOUT_VIDEO_DECODE_DPB_KHR, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, cmdBuf, vulDevice.getQueueFamilies().videoDecodeFamily, vulDevice.getQueueFamilies().mainFamily);
    decodeCmdPool.submit(cmdBuf, true);
    cmdBuf = cmdPool.getPrimaryCommandBuffer();
    dstImg.transitionImageLayout(VK_IMAGE_LAYOUT_VIDEO_DECODE_DPB_KHR, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, cmdBuf, vulDevice.getQueueFamilies().videoDecodeFamily, vulDevice.getQueueFamilies().mainFamily);
    cmdPool.submit(cmdBuf, true);
    dstImg.vulSampler = vul::VulSampler::createDefaultTexSampler(vulDevice);
    vul::VulDescriptorSet::Descriptor desc{};
    desc.type = vul::VulDescriptorSet::DescriptorType::combinedImgSampler;
    desc.stages = VK_SHADER_STAGE_FRAGMENT_BIT;
    desc.count = 1;
    desc.content = &dstImg;
    const std::unique_ptr<vul::VulDescriptorSet> descSet = vul::VulDescriptorSet::createDescriptorSet({desc}, *descPool);
    while (!vulWindow.shouldClose()) {
        glfwPollEvents();
        VkCommandBuffer cmdBuf = vulRenderer.beginFrame();
        vulGUi.startFrame();
        ImGui::Begin("Result img");
        ImGui::Image(descSet->getSet(), {480, 360});
        ImGui::End();
        vulRenderer.beginRendering(cmdBuf, vul::VulRenderer::SwapChainImageMode::clearPreviousStoreCurrent, vul::VulRenderer::DepthImageMode::noDepthImage, {}, {}, {1.0f, 1.0f, 1.0f, 1.0f}, {}, 0, 0, 1);
        vulGUi.endFrame(cmdBuf);
        vulRenderer.stopRendering(cmdBuf);
        vulRenderer.endFrame();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
 
    vulDevice.waitForIdle();
    vkDestroyVideoSessionParametersKHR(vulDevice.device(), sessionParameters, nullptr);
    for (const VkBindVideoSessionMemoryInfoKHR &bindInfo : videoSessionMemoryBinds) vkFreeMemory(vulDevice.device(), bindInfo.memory, nullptr);
    vkDestroyVideoSessionKHR(vulDevice.device(), videoSession, nullptr);

    return 0;
}
