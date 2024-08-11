#include "vul_buffer.hpp"
#include "vul_descriptors.hpp"
#include "vul_gltf_loader.hpp"
#include <GLFW/glfw3.h>
#include <iostream>
#include <memory>
#include <mesh_shading.hpp>
#include <host_device.hpp>
#include <vulkan/vulkan_core.h>
#include <meshoptimizer/src/meshoptimizer.h>

MeshResources createMeshShadingResources(const vul::Scene &scene, const vul::VulRenderer &vulRenderer, const vul::VulDescriptorPool &descPool, VkCommandBuffer cmdBuf, const vul::VulDevice &vulDevice)
{
    MeshResources meshResources;

    std::vector<uint32_t> indices = scene.indices;
    for (const vul::GltfLoader::GltfPrimMesh &mesh : scene.meshes)
        for (uint32_t i = mesh.firstIndex; i < mesh.firstIndex + mesh.indexCount; i++) indices[i] += mesh.vertexOffset;

    const size_t maxMeshlets = meshopt_buildMeshletsBound(scene.indices.size(), MAX_MESHLET_VERTICES, MAX_MESHLET_TRIANGLES);
    std::vector<meshopt_Meshlet> localMeshlets(maxMeshlets);
    std::vector<uint32_t> meshletVertices(maxMeshlets * MAX_MESHLET_VERTICES);
    std::vector<uint8_t> meshletTriangles(maxMeshlets * MAX_MESHLET_TRIANGLES * 3);
    const size_t meshletCount = meshopt_buildMeshlets(localMeshlets.data(), meshletVertices.data(), meshletTriangles.data(), indices.data(), indices.size(),
            reinterpret_cast<const float *>(scene.vertices.data()), scene.vertices.size(), sizeof(glm::vec3), MAX_MESHLET_VERTICES, MAX_MESHLET_TRIANGLES, 0.0f);

    std::vector<Meshlet> meshlets(meshletCount);
    for (size_t i = 0; i < meshletCount; i++) {
        const meshopt_Meshlet &meshlet = localMeshlets[i];
        meshlets[i] = Meshlet{.triangleOffset = static_cast<uint32_t>(meshlet.triangle_offset), .vertexOffset = static_cast<uint32_t>(meshlet.vertex_offset),
                .vertexCount = static_cast<uint16_t>(meshlet.vertex_count), .triangleCount = static_cast<uint16_t>(meshlet.triangle_count)};
    }

    std::vector<MeshletBounds> meshletBounds(meshletCount);
    for (size_t i = 0; i < meshletCount; i++) {
        const meshopt_Meshlet &meshlet = localMeshlets[i];
        meshopt_Bounds bounds = meshopt_computeMeshletBounds(&meshletVertices[meshlet.vertex_offset], &meshletTriangles[meshlet.triangle_offset],
                meshlet.triangle_count, reinterpret_cast<const float *>(scene.vertices.data()), scene.vertices.size(), sizeof(glm::vec3));
        meshletBounds[i] = MeshletBounds{.center = glm::vec3(bounds.center[0], bounds.center[1], bounds.center[2]), .radius = bounds.radius,
            .coneAxis = glm::vec<3, int8_t>(bounds.cone_axis_s8[0], bounds.cone_axis_s8[1], bounds.cone_axis_s8[2]), .coneCutoff = bounds.cone_cutoff_s8};
    }

    // const uint32_t maxVert = meshlets[meshlets.size() - 1].vertexCount + meshlets[meshlets.size() - 1].vertexOffset;
    // const uint32_t maxTriangle = meshlets[meshlets.size() - 1].triangleOffset + ((meshlets[meshlets.size() - 1].triangleCount * 3 + 3) & ~3);
    const uint32_t maxVert = meshletVertices.size();
    const uint32_t maxTriangle = meshletTriangles.size();

    meshResources.meshletVerticesBuffer = std::make_unique<vul::VulBuffer>(sizeof(uint32_t), maxVert, true,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, vulDevice);
    meshResources.meshletVerticesBuffer->writeData(meshletVertices.data(), maxVert * sizeof(uint32_t), 0, cmdBuf);
    meshResources.meshletTrianglesBuffer = std::make_unique<vul::VulBuffer>(sizeof(uint8_t), maxTriangle, true,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, vulDevice);
    meshResources.meshletTrianglesBuffer->writeData(meshletTriangles.data(), maxTriangle * sizeof(uint8_t), 0, cmdBuf);

    meshResources.meshletBuffer = std::make_unique<vul::VulBuffer>(sizeof(Meshlet), meshlets.size(), true,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, vulDevice);
    meshResources.meshletBuffer->writeVector(meshlets, 0, cmdBuf);
    meshResources.meshletBoundsBuffer = std::make_unique<vul::VulBuffer>(sizeof(MeshletBounds), meshletBounds.size(), true,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, vulDevice);
    meshResources.meshletBoundsBuffer->writeVector(meshletBounds, 0, cmdBuf);

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

        desc.stages = VK_SHADER_STAGE_MESH_BIT_EXT | VK_SHADER_STAGE_TASK_BIT_EXT;
        desc.content = scene.indexBuffer.get();
        descs.push_back(desc);
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
        desc.content = meshResources.meshletVerticesBuffer.get();
        descs.push_back(desc);
        desc.content = meshResources.meshletTrianglesBuffer.get();
        descs.push_back(desc);
        desc.content = meshResources.meshletBuffer.get();
        descs.push_back(desc);

        desc.stages = VK_SHADER_STAGE_TASK_BIT_EXT;
        desc.content = meshResources.meshletBoundsBuffer.get();
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
    res.pipeline->meshShade(res.meshletBuffer->getBufferSize() / sizeof(Meshlet) / MESHLETS_PER_TASK_SHADER, 1, 1, &res.pushConstants[0], sizeof(res.pushConstants[0]), {res.descSets[vulRenderer.getFrameIndex()]->getSet()}, cmdBuf);
}
