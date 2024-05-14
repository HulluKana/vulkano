#include "host_device.hpp"
#include "vul_gltf_loader.hpp"
#include <GLFW/glfw3.h>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <resources.hpp>
#include <vul_debug_tools.hpp>
#include <random>
#include <iostream>
#include <functional>

float partialBrdf(const glm::vec3 &lightVec, float lightStrength)
{
    /*
    std::function<float(const glm::vec3 &, const glm::vec3 &, float)> lambda = [](const glm::vec3 &someVector, const glm::vec3 &normal, float roughness)
    {
        const float dotP = dot(normal, someVector);
        const float aPow2 = (dotP * dotP) / (roughness * roughness * (1.0 - dotP * dotP));
        return (sqrt(1.0 + 1.0 / aPow2) - 1.0) / 2.0;
    };
    std::function<glm::vec3(const glm::vec3 &, const glm::vec3 &, const glm::vec3 &, const glm::vec3 &, float)> BRDF =
        [&](const glm::vec3 & surfaceNormal, const glm::vec3 & viewDirection, const glm::vec3 & lightDirection, const glm::vec3 & specularColor, float roughness)
    {
        const glm::vec3 halfVector = glm::normalize(lightDirection + viewDirection);
        const float dotHalfNorm = glm::dot(halfVector, surfaceNormal);
        const glm::vec3 freshnelColor = specularColor + (glm::vec3(1.0f) - specularColor) * std::pow((1.0f - glm::dot(halfVector, lightDirection)), 5.0f);
        const float visibleFraction = 1.0f / (1.0f + lambda(viewDirection, surfaceNormal, roughness) + lambda(lightDirection, surfaceNormal, roughness));
        const float roughnessPow2 = roughness * roughness;
        const float whatDoICallThis = 1.0f + dotHalfNorm * dotHalfNorm * (roughnessPow2 - 1.0f);
        const float ggx = roughnessPow2 / (M_PIf * whatDoICallThis * whatDoICallThis);
        return (freshnelColor * visibleFraction * ggx) / (4.0f * dot(surfaceNormal, lightDirection) * dot(surfaceNormal, viewDirection));
    };
    std::function<glm::vec3(const glm::vec3 &, const glm::vec3 &, const glm::vec3 &, const glm::vec3 &, const glm::vec3 &)> diffBRDF =
        [](const glm::vec3 & normal, const glm::vec3 & viewDirection, const glm::vec3 & lightDirection, const glm::vec3 & specularColor, const glm::vec3 & diffuseColor)
    {
        const float nl = dot(normal, lightDirection);
        const float nv = dot(normal, viewDirection);
        return 21.0f / (20.0f * M_PIf) * (glm::vec3(1.0f) - specularColor) * diffuseColor * (1.0f - std::pow(1.0f - nl, 5.0f)) * (1.0f - std::pow(1.0f - nv, 5.0f)); 
    };
    
    const glm::vec3 lightDir = glm::normalize(lightVec);
    const glm::vec3 &camDir = lightDir;
    const glm::vec3 &normal = lightDir;
    constexpr glm::vec3 specularColor = glm::vec3(0.03f);
    constexpr glm::vec3 diffuseColor = glm::vec3(0.8f);
    constexpr float roughness = 0.5f;
    const glm::vec3 color = BRDF(normal, camDir, lightDir, specularColor, roughness) + diffBRDF(normal, camDir, lightDir, specularColor, diffuseColor);
    const float colorVecLen = sqrt(glm::dot(color, color)) / sqrt(1.0f + 1.0f + 1.0f);
    const float lightDstSquared = glm::dot(lightDir, lightDir);
    return colorVecLen * lightStrength / lightDstSquared;
    */

    // The following is a compacted and optimized version of the commented code
    return 0.258908f * lightStrength / (lightVec.x * lightVec.x + lightVec.y * lightVec.y + lightVec.z * lightVec.z);
}

ReservoirGrid createReservoirGrid(const vul::Scene &scene, const vulB::VulDevice &device)
{
    const int minX = std::floor(scene.minPos.x);
    const int minY = std::floor(scene.minPos.y);
    const int minZ = std::floor(scene.minPos.z);

    const uint32_t width = std::ceil(scene.maxPos.x - scene.minPos.x);
    const uint32_t height = std::ceil(scene.maxPos.y - scene.minPos.y);
    const uint32_t depth = std::ceil(scene.maxPos.z - scene.minPos.z);

    std::random_device randomDevice{};
    std::mt19937 rng{randomDevice()};
    std::uniform_int_distribution<> dist{std::numeric_limits<int>::min(), std::numeric_limits<int>::max()};
    uint32_t state = static_cast<int64_t>(dist(rng)) + std::numeric_limits<int>::min();

    std::function<uint32_t(uint32_t &)> randomUint = [](uint32_t &state) {
        state = state * 747796405 + 2891336453;
        uint result = ((state >> ((state >> 28) + 4)) ^ state) * 277803737;
        result = (result >> 22) ^ result;
        return result;
    };
    std::function<float(uint32_t &)> randomFloat = [&](uint32_t &state) {
        return static_cast<float>(randomUint(state)) / 4294967295.0f;
    };

    std::vector<bool> occupiedCells(width * height * depth);
    for (const vulB::GltfLoader::GltfPrimMesh &mesh: scene.meshes) {
        for (float x = mesh.posMin.x; x < mesh.posMax.x; x++)  for (float y = mesh.posMin.y; y < mesh.posMax.y; y++) for (float z = mesh.posMin.z; z < mesh.posMax.z; z++) {
            uint gridX = floor(x) - minX;
            uint gridY = floor(y) - minY;
            uint gridZ = floor(z) - minZ;
            occupiedCells[gridZ * height * width + gridY * width + gridX] = true;
        }
    }

    constexpr uint32_t BAD_SAMPLES = 32;
    constexpr float CELL_SIZE = 1.0f;
    std::vector<Reservoir> reservoirs(width * height * depth * RESERVOIRS_PER_CELL);
    for (uint32_t zOff = 0; zOff < depth; zOff++) {
        for (uint32_t yOff = 0; yOff < height; yOff++) {
            for (uint32_t xOff = 0; xOff < width; xOff++) {
                if (!occupiedCells[zOff * height * width + yOff * width + xOff]) continue;
                const float x = static_cast<float>(minX) + static_cast<float>(xOff) + CELL_SIZE * 0.5f;
                const float y = static_cast<float>(minY) + static_cast<float>(yOff) + CELL_SIZE * 0.5f;
                const float z = static_cast<float>(minZ) + static_cast<float>(zOff) + CELL_SIZE * 0.5f;
                for (uint32_t i = 0; i < RESERVOIRS_PER_CELL; i++) {
                    Reservoir reservoir{};
                    for (uint32_t j = 0; j < BAD_SAMPLES; j++) {
                        const uint32_t idx = randomUint(state) % scene.lights.size();
                        const vulB::GltfLoader::GltfLight light = scene.lights[idx];
                        const float sourcePdf = 1.0f / static_cast<float>(scene.lights.size());

                        const float lightXDiff = std::max(light.position.x - x, CELL_SIZE * 0.5f);
                        const float lightYDiff = std::max(light.position.y - y, CELL_SIZE * 0.5f);
                        const float lightZDiff = std::max(light.position.z - z, CELL_SIZE * 0.5f);
                        const float lightStrength = glm::length(light.color) / /*sqrt(1.0f + 1.0f + 1.0f)*/ 1.73205f * light.intensity;
                        const float targetPdf = partialBrdf(glm::vec3(lightXDiff, lightYDiff, lightZDiff), lightStrength);

                        const float risWeight = targetPdf / sourcePdf;
                        reservoir.averageWeight += risWeight;
                        if (randomFloat(state) < risWeight / reservoir.averageWeight) {
                            reservoir.lightIdx = idx;
                            reservoir.targetPdf = targetPdf;
                        }
                    }
                    reservoir.averageWeight /= static_cast<float>(BAD_SAMPLES);
                    reservoirs[zOff * height * width * RESERVOIRS_PER_CELL + yOff * width * RESERVOIRS_PER_CELL + xOff * RESERVOIRS_PER_CELL + i] = reservoir;
                }
            }
        }
    }

    glm::vec<4, int> minPos = {minX, minY, minZ, -69};
    glm::vec<4, uint32_t> dims = {width, height, depth, 420};
    uint8_t *data = static_cast<uint8_t *>(malloc(sizeof(minPos) + sizeof(dims) + sizeof(Reservoir) * reservoirs.size()));
    memcpy(data, &minPos, sizeof(minPos));
    memcpy(data + sizeof(minPos), &dims, sizeof(dims));
    memcpy(data + sizeof(minPos) + sizeof(dims), reservoirs.data(), sizeof(Reservoir) * reservoirs.size());

    ReservoirGrid reservoirGrid{};
    reservoirGrid.reservoirs = reservoirs;
    reservoirGrid.width = width;
    reservoirGrid.height = height;
    reservoirGrid.depth = depth;
    reservoirGrid.minX = minX;
    reservoirGrid.minY = minY;
    reservoirGrid.minZ = minZ;
    reservoirGrid.buffer = std::make_unique<vulB::VulBuffer>(device);
    reservoirGrid.buffer->loadData(data, sizeof(minPos) + sizeof(dims) + sizeof(Reservoir) * reservoirs.size(), 1);
    assert(reservoirGrid.buffer->createBuffer(true, static_cast<vulB::VulBuffer::Usage>(vulB::VulBuffer::usage_ssbo | vulB::VulBuffer::usage_transferDst)) == VK_SUCCESS);

    free(data);
    return reservoirGrid;
}

std::unique_ptr<vulB::VulDescriptorSet> createRtDescSet(const vul::Vulkano &vulkano, const vul::VulAs &as, const std::unique_ptr<vul::VulImage> &rtImg, const std::unique_ptr<vulB::VulBuffer> &ubo, const std::unique_ptr<vul::VulImage> &enviromentMap, const std::unique_ptr<vulB::VulBuffer> &reservoirsBuffer)
{
    std::vector<vul::Vulkano::Descriptor> descriptors;
    vul::Vulkano::Descriptor desc{};
    desc.count = 1;

    desc.type = vul::Vulkano::DescriptorType::storageImage;
    desc.stages = {vul::Vulkano::ShaderStage::rgen, vul::Vulkano::ShaderStage::frag};
    desc.content = rtImg.get();
    descriptors.push_back(desc);

    desc.type = vul::Vulkano::DescriptorType::ubo;
    desc.stages = {vul::Vulkano::ShaderStage::rgen, vul::Vulkano::ShaderStage::rmiss, vul::Vulkano::ShaderStage::rchit};
    desc.content = ubo.get();
    descriptors.push_back(desc);

    desc.type = vul::Vulkano::DescriptorType::ssbo;
    desc.stages = {vul::Vulkano::ShaderStage::rchit, vul::Vulkano::ShaderStage::rahit};
    desc.content = vulkano.scene.indexBuffer.get();
    descriptors.push_back(desc);
    desc.content = vulkano.scene.vertexBuffer.get();
    descriptors.push_back(desc);

    desc.stages = {vul::Vulkano::ShaderStage::rchit};
    desc.content = vulkano.scene.indexBuffer.get();
    desc.content = vulkano.scene.normalBuffer.get();
    descriptors.push_back(desc);
    desc.content = vulkano.scene.tangentBuffer.get();
    descriptors.push_back(desc);

    desc.stages = {vul::Vulkano::ShaderStage::rchit, vul::Vulkano::ShaderStage::rahit};
    desc.content = vulkano.scene.indexBuffer.get();
    desc.content = vulkano.scene.uvBuffer.get();
    descriptors.push_back(desc);
    desc.content = vulkano.scene.materialBuffer.get();
    descriptors.push_back(desc);
    desc.content = vulkano.scene.primInfoBuffer.get();
    descriptors.push_back(desc);
    desc.stages = {vul::Vulkano::ShaderStage::rchit};
    desc.content = vulkano.scene.lightsBuffer.get();
    descriptors.push_back(desc);
    desc.content = reservoirsBuffer.get();
    descriptors.push_back(desc);

    // desc.type = vul::Vulkano::DescriptorType::spCombinedImgSampler;
    // desc.content = vulkano.scene.images.data();
    // desc.count = vulkano.scene.images.size();
    desc.type = vul::Vulkano::DescriptorType::combinedImgSampler; 
    desc.stages = {vul::Vulkano::ShaderStage::rchit, vul::Vulkano::ShaderStage::rahit};
    desc.content = enviromentMap.get();
    descriptors.push_back(desc);

    desc.type = vul::Vulkano::DescriptorType::combinedImgSampler;
    desc.stages = {vul::Vulkano::ShaderStage::rmiss};
    desc.content = enviromentMap.get();
    desc.count = 1;
    descriptors.push_back(desc);

    desc.type = vul::Vulkano::DescriptorType::accelerationStructure;
    desc.stages = {vul::Vulkano::ShaderStage::rgen, vul::Vulkano::ShaderStage::rchit};
    desc.count = 1;
    desc.content = &as;
    descriptors.push_back(desc);

    return vulkano.createDescriptorSet(descriptors);
}

vul::Vulkano::RenderData createRenderData(const vul::Vulkano &vulkano, const vulB::VulDevice &device, const std::array<std::shared_ptr<vulB::VulDescriptorSet>, vulB::VulSwapChain::MAX_FRAMES_IN_FLIGHT> &descSets)
{
    vulB::VulPipeline::PipelineConfigInfo pipConf{};
    pipConf.attributeDescriptions = {{0, 0, VK_FORMAT_R32G32_SFLOAT, 0}, {1, 1, VK_FORMAT_R32G32_SFLOAT, 0}};
    pipConf.bindingDescriptions = {{0, sizeof(glm::vec2), VK_VERTEX_INPUT_RATE_VERTEX}, {1, sizeof(glm::vec2), VK_VERTEX_INPUT_RATE_VERTEX}};
    pipConf.colorAttachmentFormats = {vulkano.vulRenderer.getSwapChainColorFormat()};
    pipConf.depthAttachmentFormat = vulkano.vulRenderer.getDepthFormat();
    pipConf.setLayouts = {descSets[0]->getLayout()->getDescriptorSetLayout()};
    pipConf.enableColorBlending = false;
    pipConf.cullMode = VK_CULL_MODE_NONE;

    vul::Vulkano::RenderData renderData{};
    renderData.is3d = false;
    renderData.depthImageMode = vulB::VulRenderer::DepthImageMode::noDepthImage;
    renderData.swapChainImageMode = vulB::VulRenderer::SwapChainImageMode::clearPreviousStoreCurrent;
    renderData.sampleFromDepth = false;
    for (int i = 0; i < vulB::VulSwapChain::MAX_FRAMES_IN_FLIGHT; i++) renderData.descriptorSets[i].push_back(descSets[i]);
    renderData.pipeline = std::make_shared<vulB::VulPipeline>(device, "../bin/raytrace.vert.spv", "../bin/raytrace.frag.spv", pipConf);

    return renderData;
}

void resizeRtImgs(vul::Vulkano &vulkano, std::array<std::unique_ptr<vul::VulImage>, vulB::VulSwapChain::MAX_FRAMES_IN_FLIGHT> &rtImgs)
{
    VkCommandBuffer cmdBuf = vulkano.getVulDevice().beginSingleTimeCommands();
    for (size_t i = 0; i < rtImgs.size(); i++) {
        rtImgs[i] = std::make_unique<vul::VulImage>(vulkano.getVulDevice());
        rtImgs[i]->keepRegularRaw2d32bitRgbaEmpty(vulkano.getSwapChainExtent().width, vulkano.getSwapChainExtent().height);
        rtImgs[i]->createDefaultImage(vul::VulImage::ImageType::storage2d, cmdBuf);
    }
    vulkano.getVulDevice().endSingleTimeCommands(cmdBuf);

    for (size_t i = 0; i < rtImgs.size(); i++) {
        vulkano.renderDatas[0].descriptorSets[i][0]->descriptorInfos[0].imageInfos[0].imageView = rtImgs[i]->getImageView();
        vulkano.renderDatas[0].descriptorSets[i][0]->update();
    }
}

void updateUbo(const vul::Vulkano &vulkano, std::unique_ptr<vulB::VulBuffer> &ubo)
{
    GlobalUbo uboData{};
    uboData.inverseViewMatrix = glm::inverse(vulkano.camera.getView());
    uboData.inverseProjectionMatrix = glm::inverse(vulkano.camera.getProjection());
    uboData.cameraPosition = glm::vec4(vulkano.cameraTransform.pos, 0.0f);

    uboData.ambientLightColor = glm::vec4(1.0f, 1.0f, 1.0f, 0.3f);

    // Pixel spread angle is from equation 30 from
    // https://media.contentapi.ea.com/content/dam/ea/seed/presentations/2019-ray-tracing-gems-chapter-20-akenine-moller-et-al.pdf
    uboData.pixelSpreadAngle = atan((2.0f * tan(vul::settings::cameraProperties.fovY / 2.0f)) / static_cast<float>(vulkano.getSwapChainExtent().height));
    uboData.padding1 = 69;

    ubo->writeData(&uboData, sizeof(GlobalUbo), 0);
}
