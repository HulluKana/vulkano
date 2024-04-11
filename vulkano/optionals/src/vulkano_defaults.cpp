#include "vul_attachment_image.hpp"
#include "vul_gltf_loader.hpp"
#include "vul_host_device.hpp"
#include "vul_pipeline.hpp"
#include "vul_profiler.hpp"
#include "vul_swap_chain.hpp"
#include <vulkan/vulkan_core.h>
#include<vulkano_defaults.hpp>

#include <cstdint>
#include <cstring>
#include <glm/common.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/gtc/constants.hpp>
#include <memory>
#include<stdexcept>

using namespace vulB;
namespace vul
{

defaults::Default3dInputData defaults::createDefault3dInputData(Vulkano &vulkano)
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

    Default3dInputData output{};
    output.multipleBounce2dImage = std::make_shared<VulImage>(vulkano.getVulDevice());
    output.multipleBounce2dImage->loadData(&results[0][0], LIGHT_DIR_COUNT, ROUGHNESS_COUNT, 1);
    output.multipleBounce2dImage->createImage(true, true, false, 2);

    output.multipleBounce1dImage = std::make_shared<VulImage>(vulkano.getVulDevice());
    output.multipleBounce1dImage->loadData(&otherResults[0], ROUGHNESS_COUNT, 1, 1);
    output.multipleBounce1dImage->createImage(true, true, false, 1);
    return output;
}

defaults::DefaultRenderDataInputData defaults::createDefaultDescriptors(Vulkano &vulkano, Default3dInputData inputData)
{
    for (int i = 0; i < VulSwapChain::MAX_FRAMES_IN_FLIGHT; i++){
        std::unique_ptr<VulBuffer> globalBuffer = std::make_unique<VulBuffer>(vulkano.getVulDevice());
        globalBuffer->keepEmpty(sizeof(GlobalUbo), 1);
        globalBuffer->createBuffer(false, VulBuffer::usage_ubo);
        globalBuffer->mapAll();
        vulkano.buffers.push_back(std::move(globalBuffer));
    }

    DefaultRenderDataInputData returnValue{};
    returnValue.renderDataIdx = vulkano.renderDatas.size();
    returnValue.descriptorSetLayoutIdx = vulkano.descriptorSetLayouts.size();
    vulkano.renderDatas.push_back({});
    vulkano.descriptorSetLayouts.push_back({});
    for (int i = 0; i < VulSwapChain::MAX_FRAMES_IN_FLIGHT; i++){
        std::vector<Vulkano::Descriptor> descs;
        Vulkano::Descriptor ubo{}; 
        ubo.type = Vulkano::DescriptorType::ubo;
        ubo.content = vulkano.buffers[i].get();
        ubo.stages = {Vulkano::ShaderStage::vert, Vulkano::ShaderStage::frag};
        descs.push_back(ubo);

        Vulkano::Descriptor image{};
        image.type = Vulkano::DescriptorType::spCombinedTexSampler;
        image.content = &vulkano.images[0];
        image.count = MAX_TEXTURES;
        image.stages = {Vulkano::ShaderStage::frag};
        descs.push_back(image);

        if (vulkano.hasScene){
            Vulkano::Descriptor matBuf{};
            matBuf.type = Vulkano::DescriptorType::ssbo;
            matBuf.content = vulkano.scene.materialBuffer.get();
            matBuf.stages = {Vulkano::ShaderStage::frag};
            descs.push_back(matBuf);
        }
        if (inputData.multipleBounce2dImage != nullptr){
            Vulkano::Descriptor mb2dImg{};
            mb2dImg.type = Vulkano::DescriptorType::combinedTexSampler;
            mb2dImg.content = inputData.multipleBounce2dImage.get();
            mb2dImg.stages = {Vulkano::ShaderStage::frag};
            descs.push_back(mb2dImg);
        }
        if (inputData.multipleBounce1dImage != nullptr){
            Vulkano::Descriptor mb1dImg{};
            mb1dImg.type = Vulkano::DescriptorType::combinedTexSampler;
            mb1dImg.content = inputData.multipleBounce1dImage.get();
            mb1dImg.stages = {Vulkano::ShaderStage::frag};
            descs.push_back(mb1dImg);
        }

        Vulkano::descSetReturnVal retVal = vulkano.createDescriptorSet(descs);
        if (!retVal.succeeded) throw std::runtime_error("Failed to create default descriptor sets");
        vulkano.renderDatas[returnValue.renderDataIdx].descriptorSets[i].push_back(std::move(retVal.set));
        vulkano.descriptorSetLayouts[returnValue.descriptorSetLayoutIdx] = std::move(retVal.layout);
    }
    return returnValue;
}

void defaults::createDefault3dRenderSystem(Vulkano &vulkano, DefaultRenderDataInputData inputData)
{
    VulPipeline::PipelineConfigInfo config{};
    config.attributeDescriptions = {{0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0}, {1, 1, VK_FORMAT_R32G32B32_SFLOAT, 0}, {2, 2, VK_FORMAT_R32G32_SFLOAT}};
    config.bindingDescriptions = {  {0, sizeof(glm::vec3), VK_VERTEX_INPUT_RATE_VERTEX}, {1, sizeof(glm::vec3), VK_VERTEX_INPUT_RATE_VERTEX},
                                    {2, sizeof(glm::vec2), VK_VERTEX_INPUT_RATE_VERTEX}};
    config.setLayouts = {vulkano.descriptorSetLayouts[inputData.descriptorSetLayoutIdx]->getDescriptorSetLayout()};
    config.colorAttachmentFormats = {vulkano.vulRenderer.getSwapChainColorFormat(), vulkano.vulRenderer.getSwapChainColorFormat()};
    config.depthAttachmentFormat = vulkano.vulRenderer.getDepthFormat();
    config.cullMode = VK_CULL_MODE_NONE;
    vulkano.renderDatas[inputData.renderDataIdx].pipeline = std::make_shared<VulPipeline>(vulkano.getVulDevice(), "default3D.vert.spv", "default3D.frag.spv", config);
    for (const vulB::GltfLoader::GltfNode &node : vulkano.scene.nodes) {
        const vulB::GltfLoader::GltfPrimMesh &mesh = vulkano.scene.meshes[node.primMesh];
        vulB::VulPipeline::DrawData drawData{};
        drawData.firstIndex = mesh.firstIndex;
        drawData.indexCount = mesh.indexCount;
        drawData.vertexOffset = mesh.vertexOffset;
        drawData.pPushData = std::make_shared<PushConstant>();
        drawData.pushDataSize = sizeof(PushConstant);
        vulkano.renderDatas[inputData.renderDataIdx].drawDatas.push_back(drawData);
    }
}

void defaults::createDefaultAttachmentImages(Vulkano &vulkano, DefaultRenderDataInputData inputData)
{
    for (int i = 0; i < VulSwapChain::MAX_FRAMES_IN_FLIGHT; i++){
        vulkano.renderDatas[inputData.renderDataIdx].attachmentImages[i].resize(1);
        std::shared_ptr<VulAttachmentImage> attachmentImage = std::make_shared<VulAttachmentImage>(vulkano.getVulDevice());
        attachmentImage->createEmptyImage(VulAttachmentImage::ImageType::colorAttachment, vulkano.vulRenderer.getSwapChainColorFormat(), vulkano.vulRenderer.getSwapChainExtent());
        vulkano.renderDatas[inputData.renderDataIdx].attachmentImages[i] = {attachmentImage};
    }
}

void defaults::updateDefault3dInputValues(Vulkano &vulkano, DefaultRenderDataInputData inputData)
{
    VUL_PROFILE_FUNC()
    Scene &scene = vulkano.scene;

    GlobalUbo ubo{};
    ubo.projectionMatrix = vulkano.camera.getProjection();
    ubo.viewMatrix = vulkano.camera.getView();
    ubo.cameraPosition = glm::vec4(vulkano.cameraTransform.pos, 0.0f);

    ubo.ambientLightColor = glm::vec4(1.0f, 1.0f, 1.0f, 0.02f);
    ubo.numLights = scene.lights.size();
    for (int i = 0; i < std::min(static_cast<int>(scene.lights.size()), MAX_LIGHTS); i++){
        ubo.lightPositions[i] = glm::vec4(scene.lights[i].position, 69.0f);
        ubo.lightColors[i] = glm::vec4(scene.lights[i].color, scene.lights[i].intensity);
    }
    vulkano.buffers[vulkano.getFrameIdx()]->writeData(&ubo, sizeof(ubo), 0);

    for (size_t i = 0; i < scene.nodes.size(); i++){
        const GltfLoader::GltfNode &node = scene.nodes[i];
        const GltfLoader::GltfPrimMesh &mesh = scene.meshes[node.primMesh];

        PushConstant *pushData = static_cast<PushConstant *>(vulkano.renderDatas[inputData.renderDataIdx].drawDatas[i].pPushData.get());
        pushData->modelMatrix = node.worldMatrix;
        pushData->normalMatrix = glm::mat4(1.0f);
        pushData->matIdx = mesh.materialIndex;
    }
}
    
}
