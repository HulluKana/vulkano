#include "vul_buffer.hpp"
#include "vul_descriptors.hpp"
#include "vul_gltf_loader.hpp"
#include "vul_pipeline.hpp"
#include "vul_renderer.hpp"
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

        meshResources.shadowUbos[i] = std::make_unique<vul::VulBuffer>(sizeof(ShadowUbo), 1, false, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, vulDevice);

        descs.clear();
        desc.stages = VK_SHADER_STAGE_MESH_BIT_EXT | VK_SHADER_STAGE_TASK_BIT_EXT;
        desc.type = vul::VulDescriptorSet::DescriptorType::uniformBuffer;
        desc.content = meshResources.shadowUbos[i].get();
        descs.push_back(desc);

        desc.type = vul::VulDescriptorSet::DescriptorType::storageBuffer;
        desc.content = scene.vertexBuffer.get();
        descs.push_back(desc);
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

        meshResources.shadowDescSets[i] = vul::VulDescriptorSet::createDescriptorSet(descs, descPool);

        desc.type = vul::VulDescriptorSet::DescriptorType::combinedImgSampler;
        desc.stages = VK_SHADER_STAGE_FRAGMENT_BIT;
        desc.content = &cubeMap;

        meshResources.cubeMapDescSets[i] = vul::VulDescriptorSet::createDescriptorSet({desc}, descPool);
    }

    vul::VulMeshPipeline::PipelineConfigInfo configInfo{};
    configInfo.colorAttachmentFormats = {vulRenderer.getSwapChainColorFormat()};
    configInfo.depthAttachmentFormat = vulRenderer.getDepthFormat();
    configInfo.setLayouts = {meshResources.descSets[0]->getLayout()->getDescriptorSetLayout()};
    meshResources.pipeline = std::make_unique<vul::VulMeshPipeline>(vulDevice, "mesh.task.spv", "mesh.mesh.spv", "mesh.frag.spv", configInfo);

    configInfo.colorAttachmentFormats.clear();
    configInfo.depthAttachmentFormat = vulRenderer.getDepthFormat();
    configInfo.setLayouts = {meshResources.shadowDescSets[0]->getLayout()->getDescriptorSetLayout()};
    meshResources.shadowPipeline = std::make_unique<vul::VulMeshPipeline>(vulDevice, "shadow.task.spv", "shadow.mesh.spv", "shadow.frag.spv", configInfo);

    vul::VulPipeline::PipelineConfigInfo cubeMapConfigInfo{};
    cubeMapConfigInfo.colorAttachmentFormats = {vulRenderer.getSwapChainColorFormat()};
    cubeMapConfigInfo.depthAttachmentFormat = vulRenderer.getDepthFormat();
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
    /*
    ubo.lightCount = scene.lights.size();
    for (uint32_t i = 0; i < ubo.lightCount; i++) {
        ubo.lightPositions[i] = glm::vec4(scene.lights[i].position, scene.lights[i].range);
        ubo.lightColors[i] = glm::vec4(scene.lights[i].color, scene.lights[i].intensity);
    }
    */
    ubo.lightCount = 1;
    ubo.lightPositions[0] = glm::vec4(scene.lights[2].position, scene.lights[2].range);
    ubo.lightColors[0] = glm::vec4(scene.lights[2].color, scene.lights[2].intensity);
    res.ubos[vulRenderer.getFrameIndex()]->writeData(&ubo, sizeof(ubo), 0, VK_NULL_HANDLE);

    vul::VulCamera cam{};
    cam.pos = glm::vec3(ubo.lightPositions[0]);
    cam.setPerspectiveProjection(M_PI_2, 1.0f, 0.001f, ubo.lightPositions[0].w);
    std::array<glm::vec3, VIEWS_PER_SHADOW_MAP> shadowMapFaceRotations = {glm::vec3(0.0f, -M_PI_2, 0.0f), glm::vec3(0.0f, M_PI_2, 0.0f),
        glm::vec3(M_PI_2, M_PI, 0.0f), glm::vec3(-M_PI_2, M_PI, 0.0f), glm::vec3(0.0f, M_PI, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f)};

    ShadowUbo shadowUbo;
    shadowUbo.projectionMatrix = cam.getProjection();
    for (uint32_t i = 0; i < shadowMapFaceRotations.size(); i++) {
        cam.rot = shadowMapFaceRotations[i];
        cam.updateXYZ();
        shadowUbo.viewMatrixes[i] = cam.getView();
    }
    shadowUbo.cameraPosition = glm::vec4(cam.pos, 69.0f);
    res.shadowUbos[vulRenderer.getFrameIndex()]->writeData(&shadowUbo, sizeof(shadowUbo), 0, VK_NULL_HANDLE);
}

void meshShade(const MeshResources &res, const vul::VulMeshletScene &scene, const vul::Scene &cubeMapScene, vul::VulImage &cubeMap,
        const vul::VulCamera &camera, const vul::VulRenderer &vulRenderer, const glm::vec4 &ambientLightColor, VkCommandBuffer cmdBuf)
{
    vulRenderer.beginRendering(cmdBuf, vul::VulRenderer::SwapChainImageMode::clearPreviousStoreCurrent,
            vul::VulRenderer::DepthImageMode::clearPreviousStoreCurrent, {}, {}, ambientLightColor, 1.0f, 0, 0, 1);
    res.pipeline->meshShadeIndirect(scene.indirectDrawCommandsBuffer->getBuffer(), 0, scene.indirectDrawCommands.size(),
            sizeof(VkDrawMeshTasksIndirectCommandEXT), nullptr, 0, {res.descSets[vulRenderer.getFrameIndex()]->getSet()}, cmdBuf);
    vulRenderer.stopRendering(cmdBuf);

    cubeMap.transitionImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, cmdBuf);
    cubeMap.attachmentPreservePreviousContents = false;
    cubeMap.attachmentStoreCurrentContents = false;
    vulRenderer.beginRendering(cmdBuf, vul::VulRenderer::SwapChainImageMode::noSwapChainImage,
            vul::VulRenderer::DepthImageMode::customDepthImage, {}, cubeMap.getAttachmentInfo({{{1.0f}}}),
            {}, 1.0f, cubeMap.getBaseWidth(), cubeMap.getBaseHeight(), VIEWS_PER_SHADOW_MAP);
    for (uint32_t i = 0; i < VIEWS_PER_SHADOW_MAP; i++)
        res.shadowPipeline->meshShadeIndirect(scene.indirectDrawCommandsBuffer->getBuffer(), 0, scene.indirectDrawCommands.size(),
                sizeof(VkDrawMeshTasksIndirectCommandEXT), &i, sizeof(i), {res.shadowDescSets[vulRenderer.getFrameIndex()]->getSet()}, cmdBuf);
    vulRenderer.stopRendering(cmdBuf);

    vul::VulCamera cam{};
    cam.pos = glm::vec3(0.0f);
    cam.rot = camera.rot;
    cam.updateXYZ();
    std::shared_ptr<CubeMapPushConstant> push = std::make_shared<CubeMapPushConstant>();
    push->projectionMatrix = camera.getProjection();
    push->originViewMatrix = cam.getView();

    cubeMap.transitionImageLayout(VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, cmdBuf);
    vulRenderer.beginRendering(cmdBuf, vul::VulRenderer::SwapChainImageMode::preservePreviousStoreCurrent,
            vul::VulRenderer::DepthImageMode::preservePreviousStoreCurrent, {}, {}, ambientLightColor, 1.0f, 0, 0, 1);
    res.cubeMapPipeline->draw(cmdBuf, {res.cubeMapDescSets[vulRenderer.getFrameIndex()]->getSet()},
            {cubeMapScene.vertexBuffer->getBuffer()}, cubeMapScene.indexBuffer->getBuffer(), {vul::VulPipeline::DrawData{.indexCount
            = static_cast<uint32_t>(cubeMapScene.indices.size()), .pPushData = std::shared_ptr<void>(push), .pushDataSize = sizeof(*push)}});
}
