#include "vul_descriptors.hpp"
#include <mesh_shading.hpp>
#include <host_device.hpp>
#include <vulkan/vulkan_core.h>

MeshResources createMeshShadingResources(const vul::Scene &scene, const vul::VulRenderer &vulRenderer, const vul::VulDescriptorPool &descPool, VkCommandBuffer cmdBuf, const vul::VulDevice &vulDevice)
{
    MeshResources meshResources;

    for (int i = 0; i < vul::VulSwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        meshResources.ubos[i] = std::make_unique<vul::VulBuffer>(sizeof(Ubo), 1, false, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, vulDevice);

        std::vector<vul::VulDescriptorSet::Descriptor> descs;
        vul::VulDescriptorSet::Descriptor desc;
        desc.type = vul::VulDescriptorSet::DescriptorType::uniformBuffer;
        desc.stages = VK_SHADER_STAGE_MESH_BIT_EXT | VK_SHADER_STAGE_FRAGMENT_BIT;
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

        desc.stages = VK_SHADER_STAGE_MESH_BIT_EXT;
        desc.content = scene.indexBuffer.get();
        descs.push_back(desc);
        desc.content = scene.vertexBuffer.get();
        descs.push_back(desc);
        desc.content = scene.normalBuffer.get();
        descs.push_back(desc);
        desc.content = scene.tangentBuffer.get();
        descs.push_back(desc);
        desc.content = scene.uvBuffer.get();
        descs.push_back(desc);

        meshResources.descSets[i] = vul::VulDescriptorSet::createDescriptorSet(descs, descPool);
    }

    for (const vul::GltfLoader::GltfNode &node : scene.nodes) {
        const vul::GltfLoader::GltfPrimMesh &mesh = scene.meshes[node.primMesh];
        PushConstant pc;
        pc.modelMatrix = node.worldMatrix;
        pc.triangleCount = mesh.indexCount / 3;
        pc.firstIndex = mesh.firstIndex;
        pc.vertexOffset = mesh.vertexOffset;
        pc.matIdx = mesh.materialIndex;
        meshResources.pushConstants.push_back(pc);
    }

    vul::VulMeshPipeline::PipelineConfigInfo configInfo{};
    configInfo.colorAttachmentFormats = {vulRenderer.getSwapChainColorFormat()};
    configInfo.depthAttachmentFormat = vulRenderer.getDepthFormat();
    configInfo.setLayouts = {meshResources.descSets[0]->getLayout()->getDescriptorSetLayout()};

    meshResources.pipeline = std::make_unique<vul::VulMeshPipeline>(vulDevice, "mesh.task.spv", "mesh.mesh.spv", "mesh.frag.spv", configInfo);

    return meshResources;
}

void updateMeshUbo(const MeshResources &res, const vul::Scene &scene, const vul::VulCamera &camera, const vul::VulRenderer &vulRenderer, const glm::vec4 &ambientLightColor)
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

void meshShade(const MeshResources &res, const vul::Scene &scene, const vul::VulRenderer &vulRenderer, const glm::vec4 &ambientLightColor, VkCommandBuffer cmdBuf)
{
    vulRenderer.beginRendering(cmdBuf, {}, vul::VulRenderer::SwapChainImageMode::clearPreviousStoreCurrent,
            vul::VulRenderer::DepthImageMode::clearPreviousStoreCurrent, ambientLightColor, 1.0f, 0, 0);
    for (size_t i = 0; i < scene.nodes.size(); i++) res.pipeline->meshShade(1, 1, 1, &res.pushConstants[i], sizeof(res.pushConstants[i]), {res.descSets[vulRenderer.getFrameIndex()]->getSet()}, cmdBuf);
}
