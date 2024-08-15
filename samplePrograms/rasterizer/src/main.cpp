#include "vul_GUI.hpp"
#include "vul_acceleration_structure.hpp"
#include "vul_camera.hpp"
#include "vul_descriptors.hpp"
#include "vul_device.hpp"
#include "vul_pipeline.hpp"
#include "vul_renderer.hpp"
#include "vul_scene.hpp"
#include "vul_window.hpp"
#include <GLFW/glfw3.h>
#include <vul_swap_chain.hpp>
#include <vul_image.hpp>
#include <vul_command_pool.hpp>
#include<host_device.hpp>
#include<vul_debug_tools.hpp>

#include<imgui.h>
#include <iostream>
#include <memory>
#include <array>
#include <vulkan/vulkan_core.h>

struct Resources {
    std::unique_ptr<vul::VulImage> multipleBounce2dImage = nullptr;
    std::unique_ptr<vul::VulImage> multipleBounce1dImage = nullptr;
    std::unique_ptr<vul::VulBuffer> aBuffer = nullptr;
    std::unique_ptr<vul::VulImage> aBufferHeads = nullptr;
    std::unique_ptr<vul::VulBuffer> aBufferCounter = nullptr;
    std::array<std::unique_ptr<vul::VulBuffer>, vul::VulSwapChain::MAX_FRAMES_IN_FLIGHT> ubos;
    std::array<std::unique_ptr<vul::VulDescriptorSet>, vul::VulSwapChain::MAX_FRAMES_IN_FLIGHT> mainDescSets;
    std::array<std::unique_ptr<vul::VulDescriptorSet>, vul::VulSwapChain::MAX_FRAMES_IN_FLIGHT> oitDescSets;
    std::unique_ptr<vul::VulPipeline> mainPipeline;
    std::unique_ptr<vul::VulPipeline> oitColoringPipeline;
    std::unique_ptr<vul::VulPipeline> oitCompositingPipeline;
    std::vector<vul::VulPipeline::DrawData> mainDrawDatas;
    std::vector<vul::VulPipeline::DrawData> oitColoringDrawDatas;
};

Resources createReources(vul::VulCmdPool &cmdPool, const vul::VulDevice &vulDevice, VkExtent2D swapChainExtent)
{
    constexpr int ROUGHNESS_COUNT = 32;
    constexpr int LIGHT_DIR_COUNT = 32;
    std::array<std::array<uint8_t, LIGHT_DIR_COUNT>, ROUGHNESS_COUNT> results;

    /*
    constexpr int INTEGRATION_COUNT_X = 100;
    constexpr int INTEGRATION_COUNT_Y = 100;
    constexpr glm::vec3 SURFACE_NORMAL = glm::vec3(0.0f, 1.0f, 0.0f);
    constexpr glm::vec3 SPECULAR_COLOR = glm::vec3(1.0f);

    std::function<float(glm::vec3, float)> lambda = [&] (glm::vec3 someVector, float roughness)
    {
        const float dotP = dot(SURFACE_NORMAL, someVector);
        const float aPow2 = (dotP * dotP) / (roughness * roughness * (1.0 - dotP * dotP));
        return (sqrt(1.0 + 1.0 / aPow2) - 1.0) / 2.0;
    };
    for (int i = 1; i <= ROUGHNESS_COUNT; i++){
        const float roughness = static_cast<float>(i) / static_cast<float>(ROUGHNESS_COUNT);
        const float roughnessPow2 = roughness * roughness;
        for (int j = 1; j <= LIGHT_DIR_COUNT; j++){
            const float lightAngle = static_cast<float>(j) / static_cast<float>(LIGHT_DIR_COUNT) * glm::pi<float>() / 2.0f;
            const glm::vec3 lightDir = glm::vec3(0.0f, sin(lightAngle), cos(lightAngle));
            glm::vec3 result = glm::vec3(0.0f, 0.0f, 0.0f);
            const float lightDirLambdaPlus1 = lambda(lightDir, roughness) + 1.0f;
            const float resultDivisor = 4.0f * glm::dot(SURFACE_NORMAL, lightDir);
            for (int k = 1; k <= INTEGRATION_COUNT_X; k++){
                for (int l = 1; l <= INTEGRATION_COUNT_Y; l++){
                    const float viewAngle = static_cast<float>(k) / static_cast<float>(INTEGRATION_COUNT_X) * glm::pi<float>() / 2.0f;
                    const float viewAngle2 = acos(static_cast<float>(l) / static_cast<float>(INTEGRATION_COUNT_Y));
                    const glm::vec3 viewDir = glm::vec3(sin(viewAngle2) * cos(viewAngle), sin(viewAngle2) * sin(viewAngle), cos(viewAngle2));

                    const glm::vec3 halfVector = glm::normalize(lightDir + viewDir);
                    const float dotHalfNorm = glm::dot(halfVector, SURFACE_NORMAL);
                    const float visibleFraction = 1.0 / (lambda(viewDir, roughness) + lightDirLambdaPlus1);
                    const float whatDoICallThis = 1.0 + dotHalfNorm * dotHalfNorm * (roughnessPow2 - 1.0);
                    const float ggx = roughnessPow2 / (glm::pi<float>() * whatDoICallThis * whatDoICallThis);
                    result += (SPECULAR_COLOR * visibleFraction * ggx) / resultDivisor;
                }
            }
            results[j - 1][i - 1] = static_cast<uint8_t>(glm::clamp(result.x / static_cast<float>(INTEGRATION_COUNT_X * INTEGRATION_COUNT_Y) * 255.0f, 0.0f, 255.0f));
            std::cout << (int)results[j - 1][i - 1] << " ";
        }
        std::cout << "\n";
    }
    */

    // Output of the commented section.
    results = {{
        {0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 1 , 1 , 3 , 6 , 15, 41}, 
        {1 , 1 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 1 , 1 , 1 , 1 , 2 , 3 , 4 , 6 , 9 , 15, 25, 41}, 
        {3 , 2 , 1 , 1 , 1 , 1 , 1 , 1 , 0 , 0 , 0 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 2 , 2 , 3 , 3 , 4 , 6 , 8 , 11, 15, 21, 30, 40}, 
        {4 , 3 , 2 , 2 , 2 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 2 , 2 , 2 , 2 , 2 , 3 , 3 , 4 , 5 , 6 , 7 , 9 , 11, 15, 19, 25, 32, 40}, 
        {6 , 5 , 4 , 3 , 3 , 2 , 2 , 2 , 2 , 2 , 2 , 2 , 2 , 2 , 3 , 3 , 3 , 3 , 4 , 4 , 5 , 6 , 7 , 8 , 10, 12, 15, 18, 22, 27, 33, 39}, 
        {8 , 6 , 5 , 4 , 4 , 4 , 3 , 3 , 3 , 3 , 3 , 3 , 3 , 3 , 4 , 4 , 4 , 5 , 5 , 6 , 7 , 8 , 9 , 10, 12, 14, 17, 20, 24, 29, 33, 38}, 
        {9 , 8 , 7 , 6 , 5 , 5 , 4 , 4 , 4 , 4 , 4 , 4 , 4 , 5 , 5 , 5 , 6 , 6 , 7 , 8 , 8 , 10, 11, 12, 14, 16, 19, 22, 26, 29, 33, 38}, 
        {11, 9 , 8 , 7 , 6 , 6 , 6 , 5 , 5 , 5 , 5 , 5 , 6 , 6 , 6 , 7 , 7 , 8 , 8 , 9 , 10, 11, 13, 14, 16, 18, 21, 23, 26, 30, 33, 37}, 
        {12, 11, 9 , 8 , 8 , 7 , 7 , 7 , 7 , 6 , 7 , 7 , 7 , 7 , 7 , 8 , 8 , 9 , 10, 11, 12, 13, 14, 16, 17, 19, 22, 24, 27, 30, 33, 36}, 
        {14, 12, 11, 10, 9 , 9 , 8 , 8 , 8 , 8 , 8 , 8 , 8 , 8 , 9 , 9 , 10, 10, 11, 12, 13, 14, 15, 17, 18, 20, 22, 25, 27, 30, 32, 35}, 
        {15, 13, 12, 11, 10, 10, 9,  9,  9,  9,  9,  9,  9,  9,  10, 10, 11, 11, 12, 13, 14, 15, 16, 18, 19, 21, 23, 25, 27, 29, 31, 34}, 
        {17, 15, 13, 12, 12, 11, 11, 10, 10, 10, 10, 10, 10, 11, 11, 11, 12, 12, 13, 14, 15, 16, 17, 18, 20, 21, 23, 25, 27, 29, 30, 32}, 
        {18, 16, 15, 14, 13, 12, 12, 11, 11, 11, 11, 11, 11, 12, 12, 12, 13, 13, 14, 15, 16, 17, 18, 19, 20, 22, 23, 25, 26, 28, 30, 31}, 
        {19, 17, 16, 15, 14, 13, 13, 12, 12, 12, 12, 12, 12, 12, 13, 13, 14, 14, 15, 15, 16, 17, 18, 19, 20, 22, 23, 24, 26, 27, 29, 30}, 
        {21, 19, 17, 16, 15, 14, 14, 13, 13, 13, 13, 13, 13, 13, 14, 14, 14, 15, 15, 16, 17, 18, 18, 19, 20, 21, 23, 24, 25, 26, 27, 29}, 
        {22, 20, 18, 17, 16, 15, 15, 14, 14, 14, 14, 14, 14, 14, 14, 15, 15, 15, 16, 16, 17, 18, 19, 19, 20, 21, 22, 23, 24, 25, 26, 27}, 
        {23, 21, 19, 18, 17, 16, 16, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 16, 16, 17, 17, 18, 19, 19, 20, 21, 22, 23, 23, 24, 25, 26}, 
        {24, 22, 20, 19, 18, 17, 17, 16, 16, 15, 15, 15, 15, 15, 15, 16, 16, 16, 16, 17, 17, 18, 18, 19, 20, 20, 21, 22, 23, 23, 24, 25}, 
        {25, 23, 21, 20, 19, 18, 17, 17, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 17, 17, 17, 18, 18, 19, 19, 20, 20, 21, 22, 22, 23, 24}, 
        {26, 24, 22, 21, 19, 19, 18, 17, 17, 17, 16, 16, 16, 16, 16, 16, 16, 16, 17, 17, 17, 18, 18, 18, 19, 19, 20, 20, 21, 21, 22, 23}, 
        {27, 24, 23, 21, 20, 19, 19, 18, 17, 17, 17, 17, 16, 16, 16, 16, 16, 16, 17, 17, 17, 17, 18, 18, 18, 19, 19, 20, 20, 21, 21, 21}, 
        {28, 25, 23, 22, 21, 20, 19, 18, 18, 17, 17, 17, 17, 17, 16, 16, 16, 16, 17, 17, 17, 17, 17, 18, 18, 18, 19, 19, 19, 20, 20, 20}, 
        {28, 26, 24, 23, 21, 20, 19, 19, 18, 18, 17, 17, 17, 17, 17, 16, 16, 16, 16, 17, 17, 17, 17, 17, 17, 18, 18, 18, 18, 19, 19, 19}, 
        {29, 27, 25, 23, 22, 21, 20, 19, 19, 18, 18, 17, 17, 17, 17, 16, 16, 16, 16, 16, 16, 16, 17, 17, 17, 17, 17, 17, 18, 18, 18, 18}, 
        {30, 27, 25, 24, 22, 21, 20, 19, 19, 18, 18, 17, 17, 17, 17, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 17, 17, 17, 17, 17}, 
        {31, 28, 26, 24, 23, 21, 20, 20, 19, 18, 18, 17, 17, 17, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16}, 
        {31, 28, 26, 24, 23, 22, 21, 20, 19, 18, 18, 17, 17, 17, 16, 16, 16, 16, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 16, 16}, 
        {32, 29, 27, 25, 23, 22, 21, 20, 19, 18, 18, 17, 17, 17, 16, 16, 16, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15}, 
        {32, 29, 27, 25, 23, 22, 21, 20, 19, 19, 18, 17, 17, 16, 16, 16, 15, 15, 15, 15, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14}, 
        {33, 30, 27, 25, 24, 22, 21, 20, 19, 18, 18, 17, 17, 16, 16, 15, 15, 15, 14, 14, 14, 14, 14, 14, 13, 13, 13, 13, 13, 13, 13, 13}, 
        {33, 30, 28, 26, 24, 22, 21, 20, 19, 18, 18, 17, 16, 16, 15, 15, 15, 14, 14, 14, 14, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13}, 
        {34, 30, 28, 26, 24, 23, 21, 20, 19, 18, 18, 17, 16, 16, 15, 15, 14, 14, 14, 13, 13, 13, 13, 13, 12, 12, 12, 12, 12, 12, 12, 12}
    }};

    std::array<uint8_t, ROUGHNESS_COUNT> otherResults;
    for (int i = 0; i < ROUGHNESS_COUNT; i++){
        double result = 0;
        for (int j = 0; j < LIGHT_DIR_COUNT; j++){
            result += results[i][j] * static_cast<double>(j) / static_cast<double>(ROUGHNESS_COUNT);
        }
        otherResults[i] = 2 * result / ROUGHNESS_COUNT; 
    }

    VkCommandBuffer cmdBuf = cmdPool.getPrimaryCommandBuffer();
    Resources output{};
    output.multipleBounce2dImage = vul::VulImage::createDefaultWholeImageAllInOne(vulDevice, vul::VulImage::RawImageData{LIGHT_DIR_COUNT,
            ROUGHNESS_COUNT, 1, {{&results[0][0]}}}, VK_FORMAT_R8_UNORM, false, vul::VulImage::InputDataType::rawData, vul::VulImage::ImageType::texture2d, cmdBuf);
    output.multipleBounce2dImage->vulSampler = vul::VulSampler::createCustomSampler(vulDevice, VK_FILTER_LINEAR,
            VK_SAMPLER_ADDRESS_MODE_REPEAT, 0.0f, VK_BORDER_COLOR_INT_OPAQUE_BLACK, VK_SAMPLER_MIPMAP_MODE_NEAREST, false, VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE, 0.0f, 0.0f, 1.0f);

    output.multipleBounce1dImage = vul::VulImage::createDefaultWholeImageAllInOne(vulDevice, vul::VulImage::RawImageData{
            ROUGHNESS_COUNT, 1, 1, {{&otherResults[0]}}}, VK_FORMAT_R8_UNORM, false, vul::VulImage::InputDataType::rawData, vul::VulImage::ImageType::texture1d, cmdBuf);
    output.multipleBounce1dImage->vulSampler = output.multipleBounce2dImage->vulSampler;

    output.aBuffer = std::make_unique<vul::VulBuffer>(sizeof(ABuffer), 10'000'000, true, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, vulDevice);

    output.aBufferHeads = std::make_unique<vul::VulImage>(vulDevice);
    output.aBufferHeads->keepEmpty(swapChainExtent.width, swapChainExtent.height, 1, 1, 1, VK_FORMAT_R32_UINT);
    output.aBufferHeads->createDefaultImage(vul::VulImage::ImageType::storage2d, cmdBuf);
    
    output.aBufferCounter = std::make_unique<vul::VulBuffer>(sizeof(uint32_t), 1, true, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, vulDevice);
    output.aBufferCounter->addStagingBuffer();

    for (int i = 0; i < vul::VulSwapChain::MAX_FRAMES_IN_FLIGHT; i++){
        output.ubos[i] = std::make_unique<vul::VulBuffer>(sizeof(GlobalUbo), 1, false, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, vulDevice);
        output.ubos[i]->mapAll();

        VUL_NAME_VK(output.ubos[i]->getBuffer())
        VUL_NAME_VK(output.ubos[i]->getMemory())
    }
    cmdPool.submitAndWait(cmdBuf);

    VUL_NAME_VK(output.multipleBounce2dImage->getImage())
    VUL_NAME_VK(output.multipleBounce1dImage->getImage())
    VUL_NAME_VK(output.aBuffer->getBuffer())
    VUL_NAME_VK(output.aBufferHeads->getImage())
    VUL_NAME_VK(output.aBufferCounter->getBuffer())

    return output;
}

void createDescriptors(Resources &resources, const vul::Scene &scene, const vul::VulRenderer &vulRendered, const vul::VulDescriptorPool &descPool, const vul::VulDevice &vulDevice)
{
    for (int i = 0; i < vul::VulSwapChain::MAX_FRAMES_IN_FLIGHT; i++){
        std::vector<vul::VulDescriptorSet::Descriptor> descs;
        vul::VulDescriptorSet::Descriptor ubo{}; 
        ubo.type = vul::VulDescriptorSet::DescriptorType::uniformBuffer;
        ubo.content = resources.ubos[i].get();
        ubo.stages = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT;
        descs.push_back(ubo);

        vul::VulDescriptorSet::Descriptor image{};
        image.type = vul::VulDescriptorSet::DescriptorType::spCombinedImgSampler;
        image.content = scene.images.data();
        image.count = static_cast<uint32_t>(scene.images.size());
        image.stages = VK_SHADER_STAGE_FRAGMENT_BIT;
        descs.push_back(image);

        vul::VulDescriptorSet::Descriptor matBuf{};
        matBuf.type = vul::VulDescriptorSet::DescriptorType::storageBuffer;
        matBuf.content = scene.materialBuffer.get();
        matBuf.stages = VK_SHADER_STAGE_FRAGMENT_BIT;
        descs.push_back(matBuf);

        vul::VulDescriptorSet::Descriptor mb2dImg{};
        mb2dImg.type = vul::VulDescriptorSet::DescriptorType::combinedImgSampler;
        mb2dImg.content = resources.multipleBounce2dImage.get();
        mb2dImg.stages = VK_SHADER_STAGE_FRAGMENT_BIT;
        descs.push_back(mb2dImg);

        vul::VulDescriptorSet::Descriptor mb1dImg{};
        mb1dImg.type = vul::VulDescriptorSet::DescriptorType::combinedImgSampler;
        mb1dImg.content = resources.multipleBounce1dImage.get();
        mb1dImg.stages = VK_SHADER_STAGE_FRAGMENT_BIT;
        descs.push_back(mb1dImg);

        resources.mainDescSets[i] = vul::VulDescriptorSet::createDescriptorSet(descs, descPool);

        VUL_NAME_VK(resources.mainDescSets[i]->getSet())
        VUL_NAME_VK(resources.mainDescSets[i]->getLayout()->getDescriptorSetLayout())
    }
    
    for (int i = 0; i < vul::VulSwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        std::vector<vul::VulDescriptorSet::Descriptor> descs;
        vul::VulDescriptorSet::Descriptor aBuffer{};
        aBuffer.type = vul::VulDescriptorSet::DescriptorType::storageBuffer;
        aBuffer.content = resources.aBuffer.get();
        aBuffer.count = 1;
        aBuffer.stages = VK_SHADER_STAGE_FRAGMENT_BIT;
        descs.push_back(aBuffer);

        vul::VulDescriptorSet::Descriptor aBufferHeads{};
        aBufferHeads.type = vul::VulDescriptorSet::DescriptorType::storageImage;
        aBufferHeads.content = resources.aBufferHeads.get();
        aBufferHeads.count = 1;
        aBufferHeads.stages = VK_SHADER_STAGE_FRAGMENT_BIT;
        descs.push_back(aBufferHeads);

        vul::VulDescriptorSet::Descriptor aBufferCounter{};
        aBufferCounter.type = vul::VulDescriptorSet::DescriptorType::storageBuffer;
        aBufferCounter.content = resources.aBufferCounter.get();
        aBufferCounter.count = 1;
        aBufferCounter.stages = VK_SHADER_STAGE_FRAGMENT_BIT;
        descs.push_back(aBufferCounter);

        vul::VulDescriptorSet::Descriptor depthImages{};
        depthImages.type = vul::VulDescriptorSet::DescriptorType::upCombinedImgSampler;
        depthImages.content = vulRendered.getDepthImages().data();
        depthImages.count = vulRendered.getDepthImages().size();
        depthImages.stages = VK_SHADER_STAGE_FRAGMENT_BIT;
        descs.push_back(depthImages);

        resources.oitDescSets[i] = vul::VulDescriptorSet::createDescriptorSet(descs, descPool);
    }
}

void createPipelines(Resources &resources, const vul::Scene &scene, const vul::VulRenderer &vulRenderer, const vul::VulDevice &vulDevice)
{
    vul::VulPipeline::PipelineConfigInfo mainConfig{};
    mainConfig.attributeDescriptions = {{0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0}, {1, 1, VK_FORMAT_R32G32B32_SFLOAT, 0},
        {2, 2, VK_FORMAT_R32G32B32A32_SFLOAT, 0}, {3, 3, VK_FORMAT_R32G32_SFLOAT, 0}};
    mainConfig.bindingDescriptions = {{0, sizeof(glm::vec3), VK_VERTEX_INPUT_RATE_VERTEX}, {1, sizeof(glm::vec3), VK_VERTEX_INPUT_RATE_VERTEX},
        {2, sizeof(glm::vec4), VK_VERTEX_INPUT_RATE_VERTEX}, {3, sizeof(glm::vec2), VK_VERTEX_INPUT_RATE_VERTEX}};
    mainConfig.setLayouts = {resources.mainDescSets[0]->getLayout()->getDescriptorSetLayout()};
    mainConfig.colorAttachmentFormats = {vulRenderer.getSwapChainColorFormat()};
    mainConfig.depthAttachmentFormat = vulRenderer.getDepthFormat();
    mainConfig.cullMode = VK_CULL_MODE_NONE;
    mainConfig.enableColorBlending = false;
    resources.mainPipeline = std::make_unique<vul::VulPipeline>(vulDevice, "default3D.vert.spv", "default3D.frag.spv", mainConfig);

    vul::VulPipeline::PipelineConfigInfo oitColoringConfig{};
    oitColoringConfig.attributeDescriptions = {{0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0}, {1, 1, VK_FORMAT_R32G32B32_SFLOAT, 0},
        {2, 2, VK_FORMAT_R32G32B32A32_SFLOAT, 0}, {3, 3, VK_FORMAT_R32G32_SFLOAT, 0}};
    oitColoringConfig.bindingDescriptions = {  {0, sizeof(glm::vec3), VK_VERTEX_INPUT_RATE_VERTEX}, {1, sizeof(glm::vec3), VK_VERTEX_INPUT_RATE_VERTEX},
        {2, sizeof(glm::vec4), VK_VERTEX_INPUT_RATE_VERTEX}, {3, sizeof(glm::vec2), VK_VERTEX_INPUT_RATE_VERTEX}};
    oitColoringConfig.setLayouts = {resources.mainDescSets[0]->getLayout()->getDescriptorSetLayout(), resources.oitDescSets[0]->getLayout()->getDescriptorSetLayout()};
    oitColoringConfig.colorAttachmentFormats = {};
    oitColoringConfig.depthAttachmentFormat = vulRenderer.getDepthFormat();
    oitColoringConfig.cullMode = VK_CULL_MODE_NONE;
    oitColoringConfig.enableColorBlending = false;
    resources.oitColoringPipeline = std::make_unique<vul::VulPipeline>(vulDevice, "default3D.vert.spv", "oitColoring.frag.spv", oitColoringConfig);
    for (const vul::GltfLoader::GltfNode &node : scene.nodes) {
        const vul::GltfLoader::GltfPrimMesh &mesh = scene.meshes[node.primMesh];
        const vul::GltfLoader::Material &material = scene.materials[mesh.materialIndex];
        vul::VulPipeline::DrawData drawData{};
        drawData.firstIndex = mesh.firstIndex;
        drawData.indexCount = mesh.indexCount;
        drawData.vertexOffset = mesh.vertexOffset;
        if (material.alphaMode != vul::GltfLoader::GltfAlphaMode::blend) {
            drawData.pPushData = std::make_shared<DefaultPushConstant>();
            drawData.pushDataSize = sizeof(DefaultPushConstant);
            resources.mainDrawDatas.push_back(drawData);
        }
        else {
            drawData.pPushData = std::make_shared<OitPushConstant>();
            drawData.pushDataSize = sizeof(OitPushConstant);
            resources.oitColoringDrawDatas.push_back(drawData);
        }
    }

    vul::VulPipeline::PipelineConfigInfo oitCompositingConfig{};
    oitCompositingConfig.attributeDescriptions = {{0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0}, {1, 1, VK_FORMAT_R32G32_SFLOAT, 0}};
    oitCompositingConfig.bindingDescriptions = {{0, sizeof(glm::vec3), VK_VERTEX_INPUT_RATE_VERTEX}, {1, sizeof(glm::vec2), VK_VERTEX_INPUT_RATE_VERTEX}};
    oitCompositingConfig.setLayouts = {resources.oitDescSets[0]->getLayout()->getDescriptorSetLayout()};
    oitCompositingConfig.colorAttachmentFormats = {vulRenderer.getSwapChainColorFormat()};
    oitCompositingConfig.depthAttachmentFormat = vulRenderer.getDepthFormat();
    oitCompositingConfig.cullMode = VK_CULL_MODE_NONE;
    oitCompositingConfig.enableColorBlending = true;
    oitCompositingConfig.blendOp = VK_BLEND_OP_ADD;
    oitCompositingConfig.blendSrcFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    oitCompositingConfig.blendDstFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    resources.oitCompositingPipeline = std::make_unique<vul::VulPipeline>(vulDevice, "default2D.vert.spv", "oitCompositing.frag.spv", oitCompositingConfig);

    VUL_NAME_VK(resources.mainPipeline->getPipeline())
    VUL_NAME_VK(resources.oitColoringPipeline->getPipeline())
    VUL_NAME_VK(resources.oitCompositingPipeline->getPipeline())
}

size_t updateShaderInputs(const Resources &resources, const vul::Scene &scene, const vul::VulCamera &camera, const vul::VulRenderer &vulRenderer, vul::VulCmdPool &cmdPool, VkCommandBuffer cmdBuf)
{
    VUL_PROFILE_FUNC()

    GlobalUbo ubo{};
    ubo.projectionMatrix = camera.getProjection();
    ubo.viewMatrix = camera.getView();
    ubo.cameraPosition = glm::vec4(camera.pos, 0.0f);

    ubo.ambientLightColor = glm::vec4(1.0f, 1.0f, 1.0f, 0.6f);
    ubo.numLights = scene.lights.size();
    for (int i = 0; i < std::min(static_cast<int>(scene.lights.size()), MAX_LIGHTS); i++){
        ubo.lightPositions[i] = glm::vec4(scene.lights[i].position, scene.lights[i].range);
        ubo.lightColors[i] = glm::vec4(scene.lights[i].color, scene.lights[i].intensity);
    }
    resources.ubos[vulRenderer.getFrameIndex()]->writeData(&ubo, sizeof(ubo), 0, cmdBuf);

    size_t mainIdx = 0;
    size_t oitIdx = 0;
    for (size_t i = 0; i < scene.nodes.size(); i++){
        const vul::GltfLoader::GltfNode &node = scene.nodes[i];
        const vul::GltfLoader::GltfPrimMesh &mesh = scene.meshes[node.primMesh];
        const vul::GltfLoader::Material &material = scene.materials[mesh.materialIndex];

        if (material.alphaMode != vul::GltfLoader::GltfAlphaMode::blend) {
            DefaultPushConstant *pushData = static_cast<DefaultPushConstant *>(resources.mainDrawDatas[mainIdx].pPushData.get());
            pushData->modelMatrix = node.worldMatrix;
            pushData->normalMatrix = node.normalMatrix;
            pushData->matIdx = mesh.materialIndex;
            mainIdx++;
        } else {
            OitPushConstant *pushData = static_cast<OitPushConstant *>(resources.oitColoringDrawDatas[oitIdx].pPushData.get());
            pushData->modelMatrix = node.worldMatrix;
            pushData->normalMatrix = node.normalMatrix;
            pushData->matIdx = mesh.materialIndex;
            pushData->depthImageIdx = vulRenderer.getImageIndex();
            pushData->width = static_cast<float>(vulRenderer.getSwapChainExtent().width);
            pushData->height = static_cast<float>(vulRenderer.getSwapChainExtent().height);
            oitIdx++;
        }
    }

    uint32_t transparentFragmentsCount;
    resources.aBufferCounter->readData(&transparentFragmentsCount, sizeof(transparentFragmentsCount), 0, cmdPool);
    vkCmdFillBuffer(cmdBuf, resources.aBufferCounter->getBuffer(), 0, sizeof(uint32_t), 0);

    VkDeviceSize transFragSize = transparentFragmentsCount * sizeof(ABuffer);
    VkDeviceSize aBufSize = resources.aBuffer->getBufferSize();
    if (transFragSize >= static_cast<VkDeviceSize>(aBufSize * 0.9)) return static_cast<size_t>(aBufSize * 5);
    if (static_cast<VkDeviceSize>(aBufSize * 0.1) >= transFragSize) return std::max(static_cast<size_t>(aBufSize / 4), 10'000'000ul);
    return aBufSize;
}

void updateOitResources(const Resources &resources, const vul::VulRenderer &vulRenderer, size_t requiredABufferSize, const vul::VulDevice &vulDevice)
{
    if (vulRenderer.wasSwapChainRecreated()) {
        for (int i = 0; i < vul::VulSwapChain::MAX_FRAMES_IN_FLIGHT; i++){
            for (size_t j = 0; j < vulRenderer.getDepthImages().size(); j++) {
                resources.oitDescSets[i]->descriptorInfos[3].imageInfos[j].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                resources.oitDescSets[i]->descriptorInfos[3].imageInfos[j].imageView = vulRenderer.getDepthImages()[j]->getImageView();
                resources.oitDescSets[i]->descriptorInfos[3].imageInfos[j].sampler = vulRenderer.getDepthImages()[j]->vulSampler->getSampler();
            }
            resources.oitDescSets[i]->update();
        }
    }

    if (resources.aBuffer->getBufferSize() != requiredABufferSize) {
        vkQueueWaitIdle(vulDevice.mainQueue());
        for (int i = 0; i < vul::VulSwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
            resources.oitDescSets[i]->descriptorInfos[0].bufferInfos[0] = resources.aBufferCounter->getDescriptorInfo();
            resources.oitDescSets[i]->update();
        }
        resources.aBuffer->resizeBufferAsEmpty(sizeof(ABuffer), requiredABufferSize / sizeof(ABuffer));
        for (int i = 0; i < vul::VulSwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
            resources.oitDescSets[i]->descriptorInfos[0].bufferInfos[0] = resources.aBuffer->getDescriptorInfo(); 
            resources.oitDescSets[i]->update();
        }
    }
}

void GuiStuff(double frameTime) {
    ImGui::Begin("Performance");
    ImGui::Text("Fps: %f\nTotal frame time: %fms", 1.0f / frameTime, frameTime * 1000.0f);
    ImGui::End();
}

int main() {
    vul::VulWindow vulWindow(2560, 1440, "Vulkano");
    vul::VulDevice vulDevice(vulWindow, false, false);
    std::shared_ptr<vul::VulSampler> depthImgSampler = vul::VulSampler::createDefaultTexSampler(vulDevice, 1);
    vul::VulRenderer vulRenderer(vulWindow, vulDevice, depthImgSampler);
    vul::VulCmdPool cmdPool(vul::VulCmdPool::QueueType::main, 0, 0, vulDevice);
    std::unique_ptr<vul::VulDescriptorPool> descPool = vul::VulDescriptorPool::Builder(vulDevice).setMaxSets(32).setPoolFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT).build();
    vul::VulGUI vulGui(vulWindow.getGLFWwindow(), descPool->getDescriptorPoolReference(), vulRenderer, vulDevice, cmdPool);
    vul::VulCamera camera{};

    vul::Scene mainScene(vulDevice);
    mainScene.loadScene("../Models/room/Room.gltf", "../Models/room", {}, cmdPool);
    mainScene.loadSpheres({{{-3.0f, 3.0f, 2.0f}, 2.5f, 4, 0}, {{4.0f, 8.0f, 3.0f}, 3.5f, 2, 0}}, {{}}, {}, cmdPool);
    mainScene.loadCubes({{{7.0f, 5.0f, -4.0f}, {1.0f, 1.0f, 1.0f}, 0}, {{-5.5f, 9.0f, 0.0f}, {2.0f, 1.5f, 0.7f}, 0}}, {{}}, {}, cmdPool);
    mainScene.loadScene("../Models/sponza/sponza.gltf", "../Models/sponza", {}, cmdPool);
    vul::Scene fullScreenQuad(vulDevice);
    fullScreenQuad.loadPlanes({{{-1.0f, -1.0f, 0.5f}, {1.0f, -1.0f, 0.5f}, {-1.0f, 1.0f, 0.5f}, {1.0f, 1.0f, 0.5f}, 0}}, {}, {.normal = false, .tangent = false, .material = false}, cmdPool);

    Resources resources = createReources(cmdPool, vulDevice, vulRenderer.getSwapChainExtent());
    createDescriptors(resources, mainScene, vulRenderer, *descPool.get(), vulDevice);
    createPipelines(resources, mainScene, vulRenderer, vulDevice);

    double frameStartTime = glfwGetTime();
    while (!vulWindow.shouldClose()) {
        glfwPollEvents();
        VkCommandBuffer cmdBuf = vulRenderer.beginFrame();
        vulGui.startFrame();
        if (cmdBuf == nullptr) continue;

        size_t requiredABufferSize = updateShaderInputs(resources, mainScene, camera, vulRenderer, cmdPool, cmdBuf);

        double frameTime = glfwGetTime() - frameStartTime;
        frameStartTime = glfwGetTime();
        if (!camera.shouldHideGui()) GuiStuff(frameTime);

        camera.applyInputs(vulWindow.getGLFWwindow(), frameTime, vulRenderer.getSwapChainExtent().height);
        camera.updateXYZ();
            camera.setPerspectiveProjection(80.0f * (M_PI * 2.0f / 360.0f), vulRenderer.getAspectRatio(), 0.01f, 100.0f);

        vulRenderer.beginRendering(cmdBuf, {}, vul::VulRenderer::SwapChainImageMode::clearPreviousStoreCurrent, vul::VulRenderer::DepthImageMode::clearPreviousStoreCurrent,
                {0.0f, 0.0f, 0.0f, 0.0f}, 1.0f, vulRenderer.getSwapChainExtent().width, vulRenderer.getSwapChainExtent().height);
        resources.mainPipeline->draw(cmdBuf, {resources.mainDescSets[vulRenderer.getFrameIndex()]->getSet()}, {mainScene.vertexBuffer->getBuffer(), mainScene.normalBuffer->getBuffer(),
                mainScene.tangentBuffer->getBuffer(), mainScene.uvBuffer->getBuffer()}, mainScene.indexBuffer->getBuffer(), resources.mainDrawDatas);
        vulRenderer.stopRendering(cmdBuf);

        vulRenderer.getDepthImages()[vulRenderer.getImageIndex()]->transitionImageLayout(VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, cmdBuf);
        vulRenderer.beginRendering(cmdBuf, {}, vul::VulRenderer::SwapChainImageMode::preservePreviousStoreCurrent, vul::VulRenderer::DepthImageMode::noDepthImage, {}, {}, vulRenderer.getSwapChainExtent().width, vulRenderer.getSwapChainExtent().height);
        resources.oitColoringPipeline->draw(cmdBuf, {resources.mainDescSets[vulRenderer.getFrameIndex()]->getSet(), resources.oitDescSets[vulRenderer.getFrameIndex()]->getSet()},
                {mainScene.vertexBuffer->getBuffer(), mainScene.normalBuffer->getBuffer(), mainScene.tangentBuffer->getBuffer(), mainScene.uvBuffer->getBuffer()},
                mainScene.indexBuffer->getBuffer(), resources.oitColoringDrawDatas);
        resources.oitCompositingPipeline->draw(cmdBuf, {resources.oitDescSets[vulRenderer.getFrameIndex()]->getSet()}, {fullScreenQuad.vertexBuffer->getBuffer(),
                fullScreenQuad.uvBuffer->getBuffer()}, fullScreenQuad.indexBuffer->getBuffer(), {{.indexCount = static_cast<uint32_t>(fullScreenQuad.indices.size())}});
        vulGui.endFrame(cmdBuf);
        vulRenderer.stopRendering(cmdBuf);
        vulRenderer.getDepthImages()[vulRenderer.getImageIndex()]->transitionImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, cmdBuf);

        vulRenderer.endFrame();

        updateOitResources(resources, vulRenderer, requiredABufferSize, vulDevice);
}
    vulDevice.waitForIdle();

    return 0;
}
