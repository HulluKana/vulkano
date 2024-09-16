#include "host_device.hpp"
#include "vul_gltf_loader.hpp"
#include "vul_meshlet_scene.hpp"
#include "vul_transform.hpp"
#include <cmath>
#include <glm/ext/matrix_transform.hpp>
#include <iostream>
#include <mesh_shading.hpp>
#include <vulkan/vulkan_core.h>


void renderShadowMaps(vul::VulRenderer &vulRenderer, vul::VulImage &shadowMapPoint, vul::VulImage &shadowMapDir, const vul::VulMeshletScene &scene, const MeshResources &meshResources)
{
    std::array<glm::vec3, LAYERS_IN_SHADOW_MAP> shadowMapDirFaceRotations = {glm::vec3(0.0f, M_PI_2, 0.0f), glm::vec3(0.0f, -M_PI_2, 0.0f),
        glm::vec3(M_PI_2, 0.0f, 0.0f), glm::vec3(-M_PI_2, 0.0f, 0.0f), glm::vec3(0.0f, M_PI, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f)};

    VkCommandBuffer cmdBuf = vulRenderer.beginFrame();
    vul::VulCamera cam{};
    ShadowUbo shadowUbo; 
    uint32_t viewMatIdx = 0;
    std::vector<uint32_t> directionalLightIdxs;
    std::vector<uint32_t> pointLightIdxs;
    std::vector<uint32_t> directionalLightViewMatIdxs;
    std::vector<uint32_t> pointLightViewMatIdxs;
    for (uint32_t i = 0; i < scene.lights.size(); i++) {
        if (scene.lights[i].type == vul::GltfLoader::GltfLightType::point) {
            for (uint32_t j = 0; j < LAYERS_IN_SHADOW_MAP; j++) {
                cam.pos = glm::vec3(scene.lights[i].position);
                cam.rot = shadowMapDirFaceRotations[j];
                cam.updateXYZ();
                shadowUbo.viewMatrixes[viewMatIdx] = cam.getView();
                pointLightIdxs.push_back(i);
                pointLightViewMatIdxs.push_back(viewMatIdx);
                viewMatIdx++;
            }
        } else {
            cam.pos = glm::vec3(0.0f);
            cam.setViewDirection(scene.lights[i].direction);
            shadowUbo.viewMatrixes[viewMatIdx] = cam.getView();
            directionalLightIdxs.push_back(i);
            directionalLightViewMatIdxs.push_back(viewMatIdx);
            viewMatIdx++;
        }
    }
    meshResources.shadowUbos[0]->writeData(&shadowUbo, sizeof(shadowUbo), 0, VK_NULL_HANDLE);

    shadowMapDir.attachmentPreservePreviousContents = false;
    shadowMapDir.attachmentStoreCurrentContents = true;
    vulRenderer.beginRendering(cmdBuf, vul::VulRenderer::SwapChainImageMode::noSwapChainImage,
            vul::VulRenderer::DepthImageMode::customDepthImage, {}, shadowMapDir.getAttachmentInfo({{{1.0f}}}),
            {}, 1.0f, shadowMapDir.getBaseWidth(), shadowMapDir.getBaseHeight(), shadowMapDir.getArrayCount());
    for (uint32_t i = 0; i < directionalLightIdxs.size(); i++) {
        ShadowPushConstant push;
        cam.setOrthographicProjection(-50.0f, 50.0f, -50.0f, 50.0f, -50.0f, 50.0f);
        push.projectionMatrix = glm::scale(cam.getProjection(), glm::vec3(1.0f, 1.0f, -1.0f));
        push.cameraPosition = scene.lights[directionalLightIdxs[i]].position;
        push.layerIdx = i;
        push.viewMatIdx = directionalLightViewMatIdxs[i];
        meshResources.shadowPipeline->meshShadeIndirect(scene.indirectDrawCommandsBuffer->getBuffer(), 0, scene.indirectDrawCommands.size(),
                sizeof(VkDrawMeshTasksIndirectCommandEXT), &push, sizeof(push), {meshResources.shadowDescSets[0]->getSet()}, cmdBuf);
    }
    vulRenderer.stopRendering(cmdBuf);
    shadowMapDir.transitionImageLayout(VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, cmdBuf);

    shadowMapPoint.attachmentPreservePreviousContents = false;
    shadowMapPoint.attachmentStoreCurrentContents = true;
    vulRenderer.beginRendering(cmdBuf, vul::VulRenderer::SwapChainImageMode::noSwapChainImage,
            vul::VulRenderer::DepthImageMode::customDepthImage, {}, shadowMapPoint.getAttachmentInfo({{{1.0f}}}),
            {}, 1.0f, shadowMapPoint.getBaseWidth(), shadowMapPoint.getBaseHeight(), shadowMapPoint.getArrayCount());
    for (uint32_t i = 0; i < pointLightIdxs.size(); i++) {
        ShadowPushConstant push;
        cam.setPerspectiveProjection(M_PI_2, 1.0f, 0.01f, scene.lights[pointLightIdxs[i]].range);
        if ((i % LAYERS_IN_SHADOW_MAP == 2 || i % LAYERS_IN_SHADOW_MAP == 3)) push.projectionMatrix = glm::scale(cam.getProjection(), glm::vec3(-1.0f, 1.0f, 1.0f));
        else push.projectionMatrix = glm::scale(cam.getProjection(), glm::vec3(1.0f, 1.0f, -1.0f));
        push.cameraPosition = scene.lights[pointLightIdxs[i]].position;
        push.layerIdx = i;
        push.viewMatIdx = pointLightViewMatIdxs[i];
        meshResources.shadowPipeline->meshShadeIndirect(scene.indirectDrawCommandsBuffer->getBuffer(), 0, scene.indirectDrawCommands.size(),
                sizeof(VkDrawMeshTasksIndirectCommandEXT), &push, sizeof(push), {meshResources.shadowDescSets[0]->getSet()}, cmdBuf);
    }
    vulRenderer.stopRendering(cmdBuf);
    shadowMapPoint.transitionImageLayout(VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, cmdBuf);
    vulRenderer.endFrame();
}

MeshResources createMeshShadingResources(const vul::VulMeshletScene &scene, vul::VulImage &cubeMap, vul::VulImage &shadowMapPoint,
        vul::VulImage &shadowMapDir, vul::VulRenderer &vulRenderer, const vul::VulDescriptorPool &descPool, const vul::VulDevice &vulDevice)
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

        desc.type = vul::VulDescriptorSet::DescriptorType::combinedImgSampler;
        desc.stages = VK_SHADER_STAGE_FRAGMENT_BIT;
        desc.content = &shadowMapPoint;
        descs.push_back(desc);
        desc.content = &shadowMapDir;
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
    configInfo.setLayouts = {meshResources.shadowDescSets[0]->getLayout()->getDescriptorSetLayout()};
    meshResources.shadowPipeline = std::make_unique<vul::VulMeshPipeline>(vulDevice, "shadow.task.spv", "shadow.mesh.spv", "shadow.frag.spv", configInfo);

    vul::VulPipeline::PipelineConfigInfo cubeMapPointConfigInfo{};
    cubeMapPointConfigInfo.colorAttachmentFormats = {vulRenderer.getSwapChainColorFormat()};
    cubeMapPointConfigInfo.depthAttachmentFormat = vulRenderer.getDepthFormat();
    cubeMapPointConfigInfo.setLayouts = {meshResources.cubeMapDescSets[0]->getLayout()->getDescriptorSetLayout()};
    cubeMapPointConfigInfo.attributeDescriptions = {VkVertexInputAttributeDescription{0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0}};
    cubeMapPointConfigInfo.bindingDescriptions = {VkVertexInputBindingDescription{0, sizeof(glm::vec3), VK_VERTEX_INPUT_RATE_VERTEX}};
    cubeMapPointConfigInfo.cullMode = VK_CULL_MODE_FRONT_BIT;
    meshResources.cubeMapPipeline = std::make_unique<vul::VulPipeline>(vulDevice, "cubeMap.vert.spv", "cubeMap.frag.spv", cubeMapPointConfigInfo);

    return meshResources;
}

void updateMeshUbo(const MeshResources &res, const vul::VulMeshletScene &scene, const vul::VulCamera &camera, const vul::VulRenderer &vulRenderer, const glm::vec4 &ambientLightColor)
{
    vul::VulCamera cam{};
    cam.pos = glm::vec3(0.0f);
    cam.setViewDirection(scene.lights[0].direction);
    cam.setOrthographicProjection(-50.0f, 50.0f, -50.0f, 50.0f, -50.0f, 50.0f);
    Ubo ubo;
    ubo.projectionMatrix = camera.getProjection();
    ubo.viewMatrix = camera.getView();
    ubo.sunProjViewMatrix = glm::scale(cam.getProjection(), glm::vec3(1.0f, 1.0f, -1.0f)) * cam.getView();
    ubo.cameraPosition = glm::vec4(camera.pos, 69.0f);
    ubo.ambientLightColor = ambientLightColor;
    ubo.pointLightCount = 0;
    ubo.directionalLightCount = 0;
    for (size_t i = 0; i < scene.lights.size(); i++) {
        if (scene.lights[i].type == vul::GltfLoader::GltfLightType::point && ubo.pointLightCount < MAX_POINT_LIGHTS) {
            ubo.lightPositions[ubo.pointLightCount] = glm::vec4(scene.lights[i].position, scene.lights[i].range);
            ubo.pointLightColors[ubo.pointLightCount] = glm::vec4(scene.lights[i].color, scene.lights[i].intensity);
            ubo.pointLightCount++;
        }
        if (scene.lights[i].type == vul::GltfLoader::GltfLightType::directional && ubo.directionalLightCount < MAX_DIRECTIONAL_LIGHTS) {
            ubo.lightDirections[ubo.directionalLightCount] = glm::vec4(scene.lights[i].direction, scene.lights[i].range);
            ubo.directionalLightColors[ubo.directionalLightCount] = glm::vec4(scene.lights[i].color, scene.lights[i].intensity);
            ubo.directionalLightCount++;
        }
    }
    res.ubos[vulRenderer.getFrameIndex()]->writeData(&ubo, sizeof(ubo), 0, VK_NULL_HANDLE);
}

void meshShade(const MeshResources &res, const vul::VulMeshletScene &scene, const vul::Scene &cubeMapScene,
        const vul::VulCamera &camera, const vul::VulRenderer &vulRenderer, const glm::vec4 &ambientLightColor, VkCommandBuffer cmdBuf)
{
    vulRenderer.beginRendering(cmdBuf, vul::VulRenderer::SwapChainImageMode::clearPreviousStoreCurrent,
            vul::VulRenderer::DepthImageMode::clearPreviousStoreCurrent, {}, {}, ambientLightColor, 1.0f, 0, 0, 1);
    res.pipeline->meshShadeIndirect(scene.indirectDrawCommandsBuffer->getBuffer(), 0, scene.indirectDrawCommands.size(),
            sizeof(VkDrawMeshTasksIndirectCommandEXT), nullptr, 0, {res.descSets[vulRenderer.getFrameIndex()]->getSet()}, cmdBuf);

    vul::VulCamera cam{};
    cam.pos = glm::vec3(0.0f);
    cam.rot = camera.rot;
    cam.updateXYZ();
    std::shared_ptr<CubeMapPushConstant> push = std::make_shared<CubeMapPushConstant>();
    push->projectionMatrix = camera.getProjection();
    push->originViewMatrix = cam.getView();

    res.cubeMapPipeline->draw(cmdBuf, {res.cubeMapDescSets[vulRenderer.getFrameIndex()]->getSet()},
            {cubeMapScene.vertexBuffer->getBuffer()}, cubeMapScene.indexBuffer->getBuffer(), {vul::VulPipeline::DrawData{.indexCount
            = static_cast<uint32_t>(cubeMapScene.indices.size()), .pPushData = std::shared_ptr<void>(push), .pushDataSize = sizeof(*push)}});
}
