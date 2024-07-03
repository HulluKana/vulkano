#include "imgui.h"
#include "vul_buffer.hpp"
#include "vul_comp_pipeline.hpp"
#include "vul_image.hpp"
#include "vulkano_program.hpp"
#include <GLFW/glfw3.h>
#include <cstring>
#include <iostream>
#include <limits>
#include <memory>
#include <mesh_shading.hpp>
#include <host_device.hpp>
#include <vector>
#include <vulkan/vulkan_core.h>

MeshResources createMeshShadingResources(const vul::Vulkano &vulkano)
{
    MeshResources meshResources;

    std::vector<float> initialUsableDepthImgData(vulkano.getSwapChainExtent().width * (vulkano.getSwapChainExtent().height));
    constexpr float initialDepthData = 1.0f;
    memset(initialUsableDepthImgData.data(), *reinterpret_cast<const int *>(&initialDepthData), initialUsableDepthImgData.size());
    std::vector<std::vector<void *>> initialUsableDepthImgMips(12);
    for (size_t i = 0; i < initialUsableDepthImgMips.size(); i++) initialUsableDepthImgMips[i].push_back(initialUsableDepthImgData.data());
    meshResources.usableDepthImgs.resize(vulkano.vulRenderer.getDepthImages().size());
    meshResources.mipCreationDescSets.resize(vulkano.vulRenderer.getDepthImages().size());
    for (size_t i = 0; i < meshResources.usableDepthImgs.size(); i++) {
        meshResources.usableDepthImgs[i] = std::make_unique<vul::VulImage>(vulkano.getVulDevice());
        meshResources.usableDepthImgs[i]->loadRawFromMemory(vulkano.getSwapChainExtent().width, vulkano.getSwapChainExtent().height, 1, initialUsableDepthImgMips, VK_FORMAT_R32_SFLOAT, 0, 0);
        meshResources.usableDepthImgs[i]->createCustomImageSingleTime(VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
        meshResources.usableDepthImgs[i]->createImageViewsForMipMaps();
        meshResources.usableDepthImgs[i]->vulSampler = vulkano.vulRenderer.getDepthImages()[0]->vulSampler;

        for (uint32_t j = 0; j < meshResources.usableDepthImgs[i]->getMipCount() - 1; j++) {
            std::vector<vul::Vulkano::Descriptor> descs;
            vul::Vulkano::Descriptor desc;
            desc.type = vul::Vulkano::DescriptorType::rawImageInfo;
            desc.stages = {vul::Vulkano::ShaderStage::comp};
            desc.count = 1;

            vul::Vulkano::RawImageDescriptorInfo inputDescInfo;
            inputDescInfo.descriptorInfo = meshResources.usableDepthImgs[i]->getMipDescriptorInfo(j);
            //inputDescInfo.descriptorInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            //inputDescInfo.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            inputDescInfo.descriptorInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            inputDescInfo.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            desc.content = &inputDescInfo;
            descs.push_back(desc);

            vul::Vulkano::RawImageDescriptorInfo outputDescInfo;
            outputDescInfo.descriptorInfo = meshResources.usableDepthImgs[i]->getMipDescriptorInfo(j + 1);
            outputDescInfo.descriptorInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            outputDescInfo.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            desc.content = &outputDescInfo;
            descs.push_back(desc);

            meshResources.mipCreationDescSets[i].push_back(vulkano.createDescriptorSet(descs));
        }
    }

    meshResources.mipCreationPipeline = std::make_unique<vul::VulCompPipeline>(std::vector<std::string>{"mipCreator.comp.spv"},
            std::vector<VkDescriptorSetLayout>{meshResources.mipCreationDescSets[0][0]->getLayout()->getDescriptorSetLayout()},
            vulkano.getVulDevice(), 1);

    meshResources.imageConverterDescSets.resize(meshResources.usableDepthImgs.size());
    for (size_t i = 0; i < meshResources.usableDepthImgs.size(); i++) {
        meshResources.debugImgs[i] = std::make_unique<vul::VulImage>(vulkano.getVulDevice());
        meshResources.debugImgs[i]->keepRegularRaw2d32bitRgbaEmpty(1200, 1400);
        meshResources.debugImgs[i]->createDefaultImageSingleTime(vul::VulImage::ImageType::storage2d);

        std::vector<vul::Vulkano::Descriptor> descs;
        vul::Vulkano::Descriptor desc;
        desc.type = vul::Vulkano::DescriptorType::rawImageInfo;
        desc.stages = {vul::Vulkano::ShaderStage::comp};
        desc.count = 1;

        vul::Vulkano::RawImageDescriptorInfo inputDescInfo;
        inputDescInfo.descriptorInfo = vulkano.vulRenderer.getDepthImages()[i]->getDescriptorInfo();
        inputDescInfo.descriptorInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        inputDescInfo.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        desc.content = &inputDescInfo;
        descs.push_back(desc);

        vul::Vulkano::RawImageDescriptorInfo outputDescInfo;
        outputDescInfo.descriptorInfo = meshResources.usableDepthImgs[i]->getMipDescriptorInfo(0);
        outputDescInfo.descriptorInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        outputDescInfo.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        desc.content = &outputDescInfo;
        descs.push_back(desc);

        desc.type = vul::Vulkano::DescriptorType::storageImage;
        desc.content = meshResources.debugImgs[i].get();
        descs.push_back(desc);

        meshResources.imageConverterDescSets[i] = vulkano.createDescriptorSet(descs);
    }

    meshResources.imageConverterPipeline = std::make_unique<vul::VulCompPipeline>(std::vector<std::string>{"imageConverter.comp.spv"},
            std::vector<VkDescriptorSetLayout>{meshResources.imageConverterDescSets[0]->getLayout()->getDescriptorSetLayout()},
            vulkano.getVulDevice(), 1);

    std::vector<ObjData> objDatas;
    std::vector<ChunkData> chunkDatas;
    uint32_t inChunkIdx = 0;
    glm::vec3 minChunkPos = glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 maxChunkPos = -glm::vec3(std::numeric_limits<float>::max());
    for (int x = -VOLUME_LEN / 2; x < VOLUME_LEN / 2; x++) {
        for (int y = -VOLUME_LEN / 2; y < VOLUME_LEN / 2; y++) {
            for (int z = -VOLUME_LEN / 2; z < VOLUME_LEN / 2; z++) {
                constexpr float SIDE_LENGTH = 0.5f;

                glm::vec3 pos = glm::vec3(x, y, z);
                const glm::vec3 offset = pos + glm::vec3(static_cast<float>(VOLUME_LEN) / 2.0f); 
                const glm::vec3 color = offset / static_cast<float>(VOLUME_LEN);
                objDatas.emplace_back(ObjData{glm::vec4{pos, SIDE_LENGTH}, glm::vec4{color, 1.0f}});

                minChunkPos = glm::min(minChunkPos, pos);
                maxChunkPos = glm::max(maxChunkPos, pos);
                inChunkIdx++;
                if (inChunkIdx >= CUBES_PER_MESH) {
                    chunkDatas.emplace_back(ChunkData{.minPos = minChunkPos - glm::vec3(SIDE_LENGTH / 2.0f), .maxPos = maxChunkPos + glm::vec3(SIDE_LENGTH / 2.0)}); 
                    minChunkPos = glm::vec3(std::numeric_limits<float>::max());
                    maxChunkPos = -glm::vec3(std::numeric_limits<float>::max());
                    inChunkIdx = 0;
                }
            }
        }
    }

    meshResources.cubeBuf = std::make_unique<vul::VulBuffer>(vulkano.getVulDevice());
    meshResources.cubeBuf->loadVector(objDatas);
    meshResources.cubeBuf->createBuffer(true, static_cast<vul::VulBuffer::Usage>(vul::VulBuffer::usage_ssbo | vul::VulBuffer::usage_transferDst));

    meshResources.chunksBuf = std::make_unique<vul::VulBuffer>(vulkano.getVulDevice());
    meshResources.chunksBuf->loadVector(chunkDatas);
    meshResources.chunksBuf->createBuffer(true, static_cast<vul::VulBuffer::Usage>(vul::VulBuffer::usage_ssbo | vul::VulBuffer::usage_transferDst));

    for (int i = 0; i < vul::VulSwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        meshResources.ubos[i] = std::make_unique<vul::VulBuffer>(vulkano.getVulDevice());
        meshResources.ubos[i]->keepEmpty(sizeof(MeshUbo), 1);
        meshResources.ubos[i]->createBuffer(false, vul::VulBuffer::usage_ubo);

        meshResources.cullCounters[i] = std::make_unique<vul::VulBuffer>(vulkano.getVulDevice());
        meshResources.cullCounters[i]->keepEmpty(sizeof(uint32_t), 1);
        meshResources.cullCounters[i]->createBuffer(false, static_cast<vul::VulBuffer::Usage>(vul::VulBuffer::usage_ssbo | vul::VulBuffer::usage_transferDst | vul::VulBuffer::usage_transferSrc));

        std::vector<vul::Vulkano::Descriptor> descs;
        vul::Vulkano::Descriptor desc;
        desc.type = vul::Vulkano::DescriptorType::upCombinedImgSampler;
        desc.stages = {vul::Vulkano::ShaderStage::task};
        desc.count = meshResources.usableDepthImgs.size();;
        desc.content = meshResources.usableDepthImgs.data();
        descs.push_back(desc);

        desc.type = vul::Vulkano::DescriptorType::ssbo;
        desc.stages = {vul::Vulkano::ShaderStage::mesh, vul::Vulkano::ShaderStage::task};
        desc.count = 1;
        desc.content = meshResources.cubeBuf.get();
        descs.push_back(desc);

        desc.stages = {vul::Vulkano::ShaderStage::task};
        desc.content = meshResources.chunksBuf.get();
        descs.push_back(desc);

        desc.type = vul::Vulkano::DescriptorType::ubo;
        desc.stages = {vul::Vulkano::ShaderStage::mesh, vul::Vulkano::ShaderStage::task};
        desc.content = meshResources.ubos[i].get();
        descs.push_back(desc);

        desc.type = vul::Vulkano::DescriptorType::storageImage;
        desc.stages = {vul::Vulkano::ShaderStage::task};
        desc.content = meshResources.debugImgs[i].get();
        descs.push_back(desc);

        desc.type = vul::Vulkano::DescriptorType::ssbo;
        desc.content = meshResources.cullCounters[i].get();
        descs.push_back(desc);

        meshResources.renderDescSets[i] = vulkano.createDescriptorSet(descs);
    }

    vul::VulMeshPipeline::PipelineConfigInfo configInfo{};
    configInfo.colorAttachmentFormats = {vulkano.vulRenderer.getSwapChainColorFormat()};
    configInfo.depthAttachmentFormat = vulkano.vulRenderer.getDepthFormat();
    configInfo.setLayouts = {meshResources.renderDescSets[0]->getLayout()->getDescriptorSetLayout()};
    configInfo.cullMode = VK_CULL_MODE_BACK_BIT;

    meshResources.renderPipeline = std::make_unique<vul::VulMeshPipeline>(vulkano.getVulDevice(), "mesh.task.spv", "mesh.mesh.spv", "mesh.frag.spv", configInfo);

    return meshResources;
}

void updateMeshUbo(const vul::Vulkano &vulkano, const MeshResources &res, uint32_t prevImgIdx)
{
    MeshUbo ubo;
    ubo.projectionMatrix = vulkano.camera.getProjection();
    ubo.viewMatrix = vulkano.camera.getView();
    ubo.screenDims = glm::vec<2, uint32_t>(vulkano.getSwapChainExtent().width, vulkano.getSwapChainExtent().height);
    ubo.depthImgIndex = prevImgIdx;

    res.ubos[vulkano.getFrameIdx()]->writeData(&ubo, sizeof(ubo), 0);
}

void meshShade(const vul::Vulkano &vulkano, const MeshResources &res, VkCommandBuffer cmdBuf)
{
    vulkano.vulRenderer.beginRendering(cmdBuf, {}, vul::VulRenderer::SwapChainImageMode::clearPreviousStoreCurrent,
            vul::VulRenderer::DepthImageMode::clearPreviousStoreCurrent, 0, 0);
    res.renderPipeline->meshShade(VOLUME_VOLUME / CUBES_PER_MESH / MESH_PER_TASK, 1, 1, nullptr, 0, {res.renderDescSets[vulkano.getFrameIdx()]->getSet()}, cmdBuf);
    vulkano.vulRenderer.stopRendering(cmdBuf);

    const std::unique_ptr<vul::VulImage> &depthImg = res.usableDepthImgs[vulkano.vulRenderer.getImageIndex()];

    double initialLayoutConvertingStartTime = glfwGetTime();
    VkCommandBuffer cmdBuf2 = vulkano.getVulDevice().beginSingleTimeCommands();
    vulkano.vulRenderer.getDepthImages()[vulkano.vulRenderer.getImageIndex()]->transitionWholeImageLayout(VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, cmdBuf2);
    depthImg->transitionWholeImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, cmdBuf2);
    vulkano.getVulDevice().endSingleTimeCommands(cmdBuf2);
    double initialLayoutConvertingTime = glfwGetTime() - initialLayoutConvertingStartTime;

    double imageConvertingStartTime = glfwGetTime();
    MeshPc meshPc;
    meshPc.mipSize = glm::vec<2, uint32_t>(vulkano.getSwapChainExtent().width, vulkano.getSwapChainExtent().height);
    res.imageConverterPipeline->pPushData = &meshPc;
    res.imageConverterPipeline->pushSize = sizeof(meshPc);
    res.imageConverterPipeline->begin({res.imageConverterDescSets[vulkano.vulRenderer.getImageIndex()]->getSet()});
    res.imageConverterPipeline->dispatchAll(std::ceil(static_cast<float>(meshPc.mipSize.x) / 8.0f), std::ceil(static_cast<float>(meshPc.mipSize.y) / 8.0f), 1);
    res.imageConverterPipeline->end(true);
    double imageConvertingTime = glfwGetTime() - imageConvertingStartTime;

    double mipCreationStartTime = glfwGetTime();
    res.mipCreationPipeline->begin({});
    for (uint32_t i = 0; i < depthImg->getMipCount() - 1; i++) {
        /*
        cmdBuf2 = vulkano.getVulDevice().beginSingleTimeCommands();
        depthImg->transitionImageLayout(VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, i, 1, cmdBuf2);
        vulkano.getVulDevice().endSingleTimeCommands(cmdBuf2);
        */

        meshPc.mipSize = glm::vec<2, uint32_t>(depthImg->getMipSize(i).width, depthImg->getMipSize(i).height);
        res.mipCreationPipeline->pPushData = &meshPc;
        res.mipCreationPipeline->pushSize = sizeof(meshPc);
        res.mipCreationPipeline->bindDescSets({res.mipCreationDescSets[vulkano.vulRenderer.getImageIndex()][i]->getSet()});
        res.mipCreationPipeline->dispatchAll(std::ceil(static_cast<float>(meshPc.mipSize.x) / 2.0f / 8.0f), std::ceil(static_cast<float>(meshPc.mipSize.y) / 2.0f / 8.0f), 1);
    }
    res.mipCreationPipeline->end(true);
    double mipCreationTime = glfwGetTime() - mipCreationStartTime;

    double endingLayoutConvertingStartTime = glfwGetTime();
    cmdBuf2 = vulkano.getVulDevice().beginSingleTimeCommands();
    //depthImg->transitionImageLayout(VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, depthImg->getMipCount() - 1, 1, cmdBuf2);
    depthImg->transitionWholeImageLayout(VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, cmdBuf2);
    vulkano.vulRenderer.getDepthImages()[vulkano.vulRenderer.getImageIndex()]->transitionWholeImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, cmdBuf2);
    vulkano.getVulDevice().endSingleTimeCommands(cmdBuf2);
    double endingLayoutConvertingTime = glfwGetTime() - endingLayoutConvertingStartTime;

    ImGui::Begin("Compute performance info");
    ImGui::Text("Image converting time: %lfms\nMip creation time: %lfms\nInitial image layout converting time %lfms\nEnding image layout converting time %lfms",
            imageConvertingTime * 1000.0, mipCreationTime * 1000.0, initialLayoutConvertingTime * 1000.0, endingLayoutConvertingTime * 1000.0);
    ImGui::End();
}

void resizeUsableDepthImgs(const vul::Vulkano &vulkano, MeshResources &res)
{
    std::vector<float> initialUsableDepthImgData(vulkano.getSwapChainExtent().width * (vulkano.getSwapChainExtent().height));
    constexpr float initialDepthData = 1.0f;
    memset(initialUsableDepthImgData.data(), *reinterpret_cast<const int *>(&initialDepthData), initialUsableDepthImgData.size());
    std::vector<std::vector<void *>> initialUsableDepthImgMips(12);
    for (size_t i = 0; i < initialUsableDepthImgMips.size(); i++) initialUsableDepthImgMips[i].push_back(initialUsableDepthImgData.data());
    std::vector<vul::VulImage *> oldDepthImgs(res.usableDepthImgs.size());
    for (size_t i = 0; i < res.usableDepthImgs.size(); i++) {
        oldDepthImgs[i] = res.usableDepthImgs[i].release();
        res.usableDepthImgs[i] = std::make_unique<vul::VulImage>(vulkano.getVulDevice());
        res.usableDepthImgs[i]->loadRawFromMemory(vulkano.getSwapChainExtent().width, vulkano.getSwapChainExtent().height, 1, initialUsableDepthImgMips, VK_FORMAT_R32_SFLOAT, 0, 0);
        res.usableDepthImgs[i]->createCustomImageSingleTime(VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
        res.usableDepthImgs[i]->createImageViewsForMipMaps();
        res.usableDepthImgs[i]->vulSampler = vulkano.vulRenderer.getDepthImages()[0]->vulSampler;
    }

    for (size_t i = 0; i < res.renderDescSets.size(); i++) {
        for (size_t j = 0; j < res.usableDepthImgs.size(); j++) {
            res.renderDescSets[i]->descriptorInfos[0].imageInfos[j].imageView = res.usableDepthImgs[j]->getImageView();
        }
        res.renderDescSets[i]->update();
    }

    for (size_t i = 0; i < res.mipCreationDescSets.size(); i++) {
        for (uint32_t j = 0; j < res.usableDepthImgs[i]->getMipCount() - 1; j++) {
            res.mipCreationDescSets[i][j]->descriptorInfos[0].imageInfos[0].imageView = res.usableDepthImgs[i]->getImageViewForMipLevel(j);
            res.mipCreationDescSets[i][j]->descriptorInfos[1].imageInfos[0].imageView = res.usableDepthImgs[i]->getImageViewForMipLevel(j + 1);
            res.mipCreationDescSets[i][j]->update();
        }
    }

    for (size_t i = 0; i < res.imageConverterDescSets.size(); i++) {
            res.imageConverterDescSets[i]->descriptorInfos[0].imageInfos[0].imageView = vulkano.vulRenderer.getDepthImages()[i]->getImageView();
            res.imageConverterDescSets[i]->descriptorInfos[1].imageInfos[0].imageView = res.usableDepthImgs[i]->getImageViewForMipLevel(0);
            res.imageConverterDescSets[i]->update();
    }

    for (size_t i = 0; i < oldDepthImgs.size(); i++) delete oldDepthImgs[i];
}
