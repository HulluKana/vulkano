#include "vul_buffer.hpp"
#include "vul_descriptors.hpp"
#include "vul_gltf_loader.hpp"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <iostream>
#include <memory>
#include <mesh_shading.hpp>
#include <host_device.hpp>
#include <vulkan/vulkan_core.h>
#include <meshoptimizer/src/meshoptimizer.h>

MeshResources createMeshShadingResources(vul::Scene &scene, const std::vector<std::shared_ptr<vul::VulImage>> &images, const vul::VulRenderer &vulRenderer, const vul::VulDescriptorPool &descPool, VkCommandBuffer cmdBuf, const vul::VulDevice &vulDevice)
{
    MeshResources meshResources;

    std::vector<uint32_t> indices = scene.indices;
    for (const vul::GltfLoader::GltfPrimMesh &mesh : scene.meshes)
        for (uint32_t i = mesh.firstIndex; i < mesh.firstIndex + mesh.indexCount; i++) indices[i] += mesh.vertexOffset;

    std::vector<MeshInfo> meshInfos;
    std::vector<VkDrawMeshTasksIndirectCommandEXT> indirectCommands;
    std::vector<Meshlet> meshlets;
    std::vector<MeshletBounds> meshletBounds;
    std::vector<uint32_t> meshletVertices;
    std::vector<uint8_t> meshletTriangles;
    std::vector<meshopt_Meshlet> localMeshlets;
    std::vector<uint32_t> localMeshletVertices;
    std::vector<uint8_t> localMeshletTriangles;
    for (const vul::GltfLoader::GltfNode &node : scene.nodes) {
        const vul::GltfLoader::GltfPrimMesh &mesh = scene.meshes[node.primMesh];

        const size_t maxMeshlets = meshopt_buildMeshletsBound(mesh.indexCount, MAX_MESHLET_VERTICES, MAX_MESHLET_TRIANGLES);
        localMeshlets.reserve(maxMeshlets);
        localMeshletVertices.reserve(maxMeshlets * MAX_MESHLET_VERTICES);
        localMeshletTriangles.reserve(maxMeshlets * MAX_MESHLET_TRIANGLES * 3);

        const size_t meshletCount = meshopt_buildMeshlets(localMeshlets.data(), localMeshletVertices.data(), localMeshletTriangles.data(),
                indices.data() + mesh.firstIndex, mesh.indexCount, reinterpret_cast<const float *>(scene.vertices.data()),
                scene.vertices.size(), sizeof(glm::vec3), MAX_MESHLET_VERTICES, MAX_MESHLET_TRIANGLES, 0.0f);

        meshInfos.emplace_back(MeshInfo{.modelMatrix = node.worldMatrix, .meshletOffset = static_cast<uint32_t>(meshlets.size()),
                .meshletCount = static_cast<uint32_t>(meshletCount), .matIdx = static_cast<uint32_t>(mesh.materialIndex),
                .padding1 = 69, .padding2 = {420, 024, 024, 420}, .padding3 = {41, 41, 14, 14}, .padding4 = {12, 345, 67, 890}});
        indirectCommands.emplace_back(VkDrawMeshTasksIndirectCommandEXT{((static_cast<uint32_t>(meshletCount) +
                (MESHLETS_PER_TASK_SHADER - 1)) & ~(MESHLETS_PER_TASK_SHADER - 1)) / MESHLETS_PER_TASK_SHADER, 1, 1});

        for (size_t i = 0; i < meshletCount; i++) {
            const meshopt_Meshlet &meshlet = localMeshlets[i];
            meshlets.emplace_back(Meshlet{.triangleOffset = static_cast<uint32_t>(meshlet.triangle_offset + meshletTriangles.size()),
                .vertexOffset = static_cast<uint32_t>(meshlet.vertex_offset + meshletVertices.size()), .vertexCount = static_cast<uint16_t>(meshlet.vertex_count),
                .triangleCount = static_cast<uint16_t>(meshlet.triangle_count)});
        }

        for (size_t i = 0; i < meshletCount; i++) {
            const meshopt_Meshlet &meshlet = localMeshlets[i];
            meshopt_Bounds bounds = meshopt_computeMeshletBounds(&localMeshletVertices[meshlet.vertex_offset], &localMeshletTriangles[meshlet.triangle_offset],
                    meshlet.triangle_count, reinterpret_cast<const float *>(scene.vertices.data()), scene.vertices.size(), sizeof(glm::vec3));
            meshletBounds.emplace_back(MeshletBounds{.center = glm::vec3(bounds.center[0], bounds.center[1], bounds.center[2]), .radius = bounds.radius,
                .coneAxis = glm::vec<3, int8_t>(bounds.cone_axis_s8[0], bounds.cone_axis_s8[1], bounds.cone_axis_s8[2]), .coneCutoff = bounds.cone_cutoff_s8});
        }

        const uint32_t maxVert = localMeshlets[meshletCount - 1].vertex_offset + localMeshlets[meshletCount - 1].vertex_count;
        const uint32_t maxTriangle = localMeshlets[meshletCount - 1].triangle_offset + ((localMeshlets[meshletCount - 1].triangle_count * 3 + 3) & ~3); // May wanna check on this last part
        meshletVertices.insert(meshletVertices.end(), localMeshletVertices.begin(), localMeshletVertices.begin() + maxVert);
        meshletTriangles.insert(meshletTriangles.end(), localMeshletTriangles.begin(), localMeshletTriangles.begin() + maxTriangle);
    }

    meshResources.meshletVerticesBuffer = std::make_unique<vul::VulBuffer>(sizeof(*meshletVertices.begin()), meshletVertices.size(), true,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, vulDevice);
    meshResources.meshletVerticesBuffer->writeVector(meshletVertices, 0, cmdBuf);
    meshResources.meshletTrianglesBuffer = std::make_unique<vul::VulBuffer>(sizeof(*meshletTriangles.begin()), meshletTriangles.size(), true,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, vulDevice);
    meshResources.meshletTrianglesBuffer->writeVector(meshletTriangles, 0, cmdBuf);

    meshResources.meshletBuffer = std::make_unique<vul::VulBuffer>(sizeof(Meshlet), meshlets.size(), true,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, vulDevice);
    meshResources.meshletBuffer->writeVector(meshlets, 0, cmdBuf);
    meshResources.meshletBoundsBuffer = std::make_unique<vul::VulBuffer>(sizeof(MeshletBounds), meshletBounds.size(), true,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, vulDevice);
    meshResources.meshletBoundsBuffer->writeVector(meshletBounds, 0, cmdBuf);

    meshResources.meshInfoBuffer = std::make_unique<vul::VulBuffer>(sizeof(MeshInfo), meshInfos.size(), true,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, vulDevice);
    meshResources.meshInfoBuffer->writeVector(meshInfos, 0, cmdBuf);
    meshResources.drawMeshTasksIndirectBuffer = std::make_unique<vul::VulBuffer>(sizeof(VkDrawMeshTasksIndirectCommandEXT),
            indirectCommands.size(), true, VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, vulDevice);
    meshResources.drawMeshTasksIndirectBuffer->writeVector(indirectCommands, 0, cmdBuf);

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
        desc.count = images.size();
        desc.content = images.data();
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
        desc.content = meshResources.meshletVerticesBuffer.get();
        descs.push_back(desc);
        desc.content = meshResources.meshletTrianglesBuffer.get();
        descs.push_back(desc);
        desc.content = meshResources.meshletBuffer.get();
        descs.push_back(desc);

        desc.stages = VK_SHADER_STAGE_TASK_BIT_EXT;
        desc.content = meshResources.meshletBoundsBuffer.get();
        descs.push_back(desc);
        desc.content = meshResources.meshInfoBuffer.get();
        descs.push_back(desc);

        meshResources.descSets[i] = vul::VulDescriptorSet::createDescriptorSet(descs, descPool);
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
    res.pipeline->meshShadeIndirect(res.drawMeshTasksIndirectBuffer->getBuffer(), 0,
            res.drawMeshTasksIndirectBuffer->getBufferSize() / sizeof(VkDrawMeshTasksIndirectCommandEXT),
            sizeof(VkDrawMeshTasksIndirectCommandEXT), nullptr, 0, {res.descSets[vulRenderer.getFrameIndex()]->getSet()}, cmdBuf);
}
