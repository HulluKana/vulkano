#include "vul_buffer.hpp"
#include "vul_descriptors.hpp"
#include "vul_gltf_loader.hpp"
#include "vul_pipeline.hpp"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <iostream>
#include <memory>
#include <mesh_shading.hpp>
#include <host_device.hpp>
#include <vulkan/vulkan_core.h>
#include <meshoptimizer/src/meshoptimizer.h>

MeshResources createMeshShadingResources(const vul::VulMeshletScene &scene, const vul::VulImage &cubeMap, const vul::VulRenderer &vulRenderer, const vul::VulDescriptorPool &descPool, VkCommandBuffer cmdBuf, const vul::VulDevice &vulDevice)
{
    MeshResources meshResources;

    for (int i = 0; i < vul::VulSwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        meshResources.ubos[i] = std::make_unique<vul::VulBuffer>(sizeof(Ubo), 1, false, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, vulDevice);

        std::vector<vul::VulDescriptorSet::Descriptor> descs;
        vul::VulDescriptorSet::Descriptor desc;
        desc.type = vul::VulDescriptorSet::DescriptorType::uniformBuffer;
        desc.stages = VK_SHADER_STAGE_TASK_BIT_EXT | VK_SHADER_STAGE_MESH_BIT_EXT | VK_SHADER_STAGE_FRAGMENT_BIT;
        desc.count = 1;
        desc.content = meshResources.ubos[i].get();
        descs.push_back(desc);

        desc.type = vul::VulDescriptorSet::DescriptorType::spCombinedImgSampler;
        desc.stages = VK_SHADER_STAGE_FRAGMENT_BIT;
        desc.count = scene.images.size();
        desc.content = scene.images.data();
        descs.push_back(desc);

        desc.type = vul::VulDescriptorSet::DescriptorType::storageBuffer;
        desc.count = 1;
        desc.content = scene.materialBuffer.get();
        descs.push_back(desc);

        desc.stages = VK_SHADER_STAGE_MESH_BIT_EXT | VK_SHADER_STAGE_TASK_BIT_EXT;
        desc.content = scene.vertexBuffer.get();
        descs.push_back(desc);

        desc.stages = VK_SHADER_STAGE_MESH_BIT_EXT;
        desc.content = scene.normalBuffer.get();
        descs.push_back(desc);
        desc.content = scene.tangentBuffer.get();
        descs.push_back(desc);
        desc.content = scene.uvBuffer.get();
        descs.push_back(desc);

        desc.stages = VK_SHADER_STAGE_MESH_BIT_EXT | VK_SHADER_STAGE_TASK_BIT_EXT;
        desc.content = scene.vertIndexBuffer.get();
        descs.push_back(desc);
        desc.content = scene.triIndexBuffer.get();
        descs.push_back(desc);
        desc.content = scene.meshletBuffer.get();
        descs.push_back(desc);

        desc.stages = VK_SHADER_STAGE_TASK_BIT_EXT;
        desc.content = scene.meshletBoundsBuffer.get();
        descs.push_back(desc);
        desc.content = scene.meshBuffer.get();
        descs.push_back(desc);

        meshResources.descSets[i] = vul::VulDescriptorSet::createDescriptorSet(descs, descPool);

        desc.stages = VK_SHADER_STAGE_FRAGMENT_BIT;
        desc.type = vul::VulDescriptorSet::DescriptorType::combinedImgSampler;
        desc.content = &cubeMap;
        meshResources.cubeMapDescSets[i] = vul::VulDescriptorSet::createDescriptorSet({desc}, descPool);
    }

    vul::VulMeshPipeline::PipelineConfigInfo configInfo{};
    configInfo.colorAttachmentFormats = {vulRenderer.getSwapChainColorFormat()};
    configInfo.depthAttachmentFormat = vulRenderer.getDepthFormat();
    configInfo.setLayouts = {meshResources.descSets[0]->getLayout()->getDescriptorSetLayout()};

    meshResources.pipeline = std::make_unique<vul::VulMeshPipeline>(vulDevice, "mesh.task.spv", "mesh.mesh.spv", "mesh.frag.spv", configInfo);

    vul::VulPipeline::PipelineConfigInfo cubeMapConfigInfo{};
    cubeMapConfigInfo.colorAttachmentFormats = configInfo.colorAttachmentFormats;
    cubeMapConfigInfo.depthAttachmentFormat = configInfo.depthAttachmentFormat;
    cubeMapConfigInfo.setLayouts = {meshResources.cubeMapDescSets[0]->getLayout()->getDescriptorSetLayout()};
    cubeMapConfigInfo.attributeDescriptions = {VkVertexInputAttributeDescription{0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0}};
    cubeMapConfigInfo.bindingDescriptions = {VkVertexInputBindingDescription{0, sizeof(glm::vec3), VK_VERTEX_INPUT_RATE_VERTEX}};
    cubeMapConfigInfo.cullMode = VK_CULL_MODE_FRONT_BIT;
    meshResources.cubeMapPipeline = std::make_unique<vul::VulPipeline>(vulDevice, "cubeMap.vert.spv", "cubeMap.frag.spv", cubeMapConfigInfo);

    return meshResources;
}

void updateMeshUbo(const MeshResources &res, const vul::VulMeshletScene &scene, const vul::VulCamera &camera, const vul::VulRenderer &vulRenderer, const glm::vec4 &ambientLightColor)
{
    Ubo ubo;
    ubo.projectionMatrix = camera.getProjection();
    ubo.viewMatrix = camera.getView();
    ubo.cameraPosition = glm::vec4(camera.pos, 69.0f);
    ubo.ambientLightColor = ambientLightColor;
    ubo.lightCount = scene.lights.size();
    for (uint32_t i = 0; i < ubo.lightCount; i++) {
        ubo.lightPositions[i] = glm::vec4(scene.lights[i].position, scene.lights[i].range);
        ubo.lightColors[i] = glm::vec4(scene.lights[i].color, scene.lights[i].intensity);
    }
    res.ubos[vulRenderer.getFrameIndex()]->writeData(&ubo, sizeof(ubo), 0, VK_NULL_HANDLE);
}

void meshShade(const MeshResources &res, const vul::VulMeshletScene &scene, const vul::Scene &cubeMapScene, vul::VulCamera &camera, const vul::VulRenderer &vulRenderer, const glm::vec4 &ambientLightColor, VkCommandBuffer cmdBuf)
{
    vulRenderer.beginRendering(cmdBuf, {}, vul::VulRenderer::SwapChainImageMode::clearPreviousStoreCurrent,
            vul::VulRenderer::DepthImageMode::clearPreviousStoreCurrent, ambientLightColor, 1.0f, 0, 0);
    res.pipeline->meshShadeIndirect(scene.indirectDrawCommandsBuffer->getBuffer(), 0, scene.indirectDrawCommands.size(),
            sizeof(VkDrawMeshTasksIndirectCommandEXT), nullptr, 0, {res.descSets[vulRenderer.getFrameIndex()]->getSet()}, cmdBuf);

    const glm::vec3 pos = camera.pos;
    camera.pos = glm::vec3(0.0f);
    camera.updateXYZ();
    std::shared_ptr<CubeMapPushConstant> push = std::make_shared<CubeMapPushConstant>();
    push->projectionMatrix = camera.getProjection();
    push->originViewMatrix = camera.getView();
    camera.pos = pos;

    res.cubeMapPipeline->draw(cmdBuf, {res.cubeMapDescSets[vulRenderer.getFrameIndex()]->getSet()},
            {cubeMapScene.vertexBuffer->getBuffer()}, cubeMapScene.indexBuffer->getBuffer(), {vul::VulPipeline::DrawData{.indexCount
            = static_cast<uint32_t>(cubeMapScene.indices.size()), .pPushData = std::shared_ptr<void>(push), .pushDataSize = sizeof(*push)}});
}
