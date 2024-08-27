#include "vul_gltf_loader.hpp"
#include <functional>
#include <thread>
#include <vul_meshlet_scene.hpp>
#include <vul_scene.hpp>
#include <meshoptimizer/src/meshoptimizer.h>

namespace vul {

void VulMeshletScene::loadGltf(const std::string &fileName, const std::string &textureDir, uint32_t maxTriangles,
        uint32_t maxVertices, uint32_t maxMeshletsPerWorkgroup, const WantedBuffers &wantedBuffers,
        VulCmdPool &cmdPool, const VulDevice &vulDevice)
{
    vul::Scene scene(vulDevice); 
    scene.loadSceneSync(fileName, textureDir, vul::Scene::WantedBuffers{.vertex = true,
            .index = true, .normal = wantedBuffers.normal, .tangent = wantedBuffers.tangent,
            .uv = wantedBuffers.uv, .material = wantedBuffers.material, .primInfo = false,
            .enableAddressTaking = false, .enableUsageForAccelerationStructures = false}, cmdPool);

    std::vector<uint32_t> indices = scene.indices;
    for (const vul::GltfLoader::GltfPrimMesh &mesh : scene.meshes)
        for (uint32_t i = mesh.firstIndex; i < mesh.firstIndex + mesh.indexCount; i++) indices[i] += mesh.vertexOffset;

    size_t maxMeshlets = 0;
    size_t maxMeshletsInSingleMesh = 0;
    size_t maxTotalVertices = 0;
    size_t maxTotalTriangles = 0;
    for (const vul::GltfLoader::GltfNode &node : scene.nodes) {
        const vul::GltfLoader::GltfPrimMesh &mesh = scene.meshes[node.primMesh];
        const size_t maxLocalMeshlets = meshopt_buildMeshletsBound(mesh.indexCount, maxVertices, maxTriangles);
        maxMeshlets += maxLocalMeshlets;
        maxMeshletsInSingleMesh = std::max(maxMeshletsInSingleMesh, maxLocalMeshlets);
        maxTotalVertices += maxLocalMeshlets * maxVertices;
        maxTotalTriangles += maxLocalMeshlets * maxTriangles * 3;
    }

    meshes.resize(scene.nodes.size());
    indirectDrawCommands.resize(meshes.size());
    meshlets.resize(maxMeshlets);
    meshletBounds.resize(maxMeshlets);
    vertIndices.resize(maxTotalVertices);
    triIndices.resize(maxTotalTriangles);
    std::atomic_uint32_t atomicMeshIdx = 0;
    std::atomic_uint32_t atomicMeshletIdx = 0;
    std::atomic_uint32_t atomicVertIdx = 0;
    std::atomic_uint32_t atomicTriIdx = 0;
    std::function<void()> createMeshlets = [&]() {
        std::vector<meshopt_Meshlet> localMeshlets(maxMeshletsInSingleMesh);
        std::vector<uint32_t> localMeshletVertices(maxMeshletsInSingleMesh * maxVertices);
        std::vector<uint8_t> localMeshletTriangles(maxMeshletsInSingleMesh * maxTriangles * 3);
        while (true) {
            const uint32_t meshIdx = atomicMeshIdx++;
            if (meshIdx >= scene.nodes.size()) break;
            const vul::GltfLoader::GltfNode &node = scene.nodes[meshIdx];
            const vul::GltfLoader::GltfPrimMesh &mesh = scene.meshes[node.primMesh];

            const size_t meshletCount = meshopt_buildMeshlets(localMeshlets.data(), localMeshletVertices.data(), localMeshletTriangles.data(),
                    indices.data() + mesh.firstIndex, mesh.indexCount, reinterpret_cast<const float *>(scene.vertices.data()),
                    scene.vertices.size(), sizeof(glm::vec3), maxVertices, maxTriangles, 0.0f);

            const uint32_t meshletIdx = atomicMeshletIdx.fetch_add(meshletCount);
            meshes[meshIdx] = MeshInfo{.modelMatrix = node.worldMatrix, .meshletOffset = meshletIdx,
                    .meshletCount = static_cast<uint32_t>(meshletCount), .matIdx = static_cast<uint32_t>(mesh.materialIndex)};
            indirectDrawCommands[meshIdx] = VkDrawMeshTasksIndirectCommandEXT{((static_cast<uint32_t>(meshletCount) +
                            (maxMeshletsPerWorkgroup - 1)) & ~(maxMeshletsPerWorkgroup - 1)) / maxMeshletsPerWorkgroup, 1, 1};

            const uint32_t maxVert = localMeshlets[meshletCount - 1].vertex_offset + localMeshlets[meshletCount - 1].vertex_count;
            const uint32_t maxTriangle = localMeshlets[meshletCount - 1].triangle_offset + ((localMeshlets[meshletCount - 1].triangle_count * 3 + 3) & ~3);
            const uint32_t vertIdx = atomicVertIdx.fetch_add(maxVert);
            const uint32_t triIdx = atomicTriIdx.fetch_add(maxTriangle);
            for (size_t i = 0; i < meshletCount; i++) {
                const meshopt_Meshlet &meshlet = localMeshlets[i];
                meshlets[meshletIdx + i] = Meshlet{.triangleOffset = meshlet.triangle_offset + triIdx, .vertexOffset = meshlet.vertex_offset + vertIdx,
                    .vertexCount = static_cast<uint16_t>(meshlet.vertex_count), .triangleCount = static_cast<uint16_t>(meshlet.triangle_count)};
                meshopt_Bounds bounds = meshopt_computeMeshletBounds(&localMeshletVertices[meshlet.vertex_offset], &localMeshletTriangles[meshlet.triangle_offset],
                        meshlet.triangle_count, reinterpret_cast<const float *>(scene.vertices.data()), scene.vertices.size(), sizeof(glm::vec3));
                meshletBounds[meshletIdx + i] = MeshletBounds{.center = glm::vec3(bounds.center[0], bounds.center[1], bounds.center[2]), .radius = bounds.radius,
                        .coneAxis = glm::vec<3, int8_t>(bounds.cone_axis_s8[0], bounds.cone_axis_s8[1], bounds.cone_axis_s8[2]), .coneCutoff = bounds.cone_cutoff_s8};
            }

            memcpy(&vertIndices[vertIdx], localMeshletVertices.data(), maxVert * sizeof(*vertIndices.data()));
            memcpy(&triIndices[triIdx], localMeshletTriangles.data(), maxTriangle * sizeof(*triIndices.data()));
        }
    };

    std::vector<std::thread> threads(std::thread::hardware_concurrency());
    for (size_t i = 0; i < threads.size(); i++) threads[i] = std::thread(createMeshlets);
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(10ms);
    for (size_t i = 0; i < threads.size(); i++) threads[i].join();
    meshlets.resize(atomicMeshletIdx);
    meshletBounds.resize(meshlets.size());
    vertIndices.resize(atomicVertIdx);
    triIndices.resize(atomicTriIdx);

    lights = scene.lights;
    materials = scene.materials;
    images = scene.images;

    VkCommandBuffer cmdBuf = cmdPool.getPrimaryCommandBuffer();
    if (wantedBuffers.vertex) vertexBuffer = std::move(scene.vertexBuffer);
    if (wantedBuffers.normal) normalBuffer = std::move(scene.normalBuffer);
    if (wantedBuffers.tangent) tangentBuffer = std::move(scene.tangentBuffer);
    if (wantedBuffers.uv) uvBuffer = std::move(scene.uvBuffer);
    if (wantedBuffers.vertIdxs) {
        vertIndexBuffer = std::make_unique<vul::VulBuffer>(sizeof(*vertIndices.begin()), vertIndices.size(), true,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, vulDevice);
        vertIndexBuffer->writeVector(vertIndices, 0, cmdBuf);
    }
    if (wantedBuffers.triIdxs) {
        triIndexBuffer = std::make_unique<vul::VulBuffer>(sizeof(*triIndices.begin()), triIndices.size(), true,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, vulDevice);
        triIndexBuffer->writeVector(triIndices, 0, cmdBuf);
    }
    if (wantedBuffers.material) materialBuffer = std::move(scene.materialBuffer);
    if (wantedBuffers.meshlets) {
        meshletBuffer = std::make_unique<vul::VulBuffer>(sizeof(Meshlet), meshlets.size(), true,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, vulDevice);
        meshletBuffer->writeVector(meshlets, 0, cmdBuf);
    }
    if (wantedBuffers.meshletBounds) {
        meshletBoundsBuffer = std::make_unique<vul::VulBuffer>(sizeof(MeshletBounds), meshletBounds.size(), true,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, vulDevice);
        meshletBoundsBuffer->writeVector(meshletBounds, 0, cmdBuf);
    }
    if (wantedBuffers.meshes) {
        meshBuffer = std::make_unique<vul::VulBuffer>(sizeof(MeshInfo), meshes.size(), true,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, vulDevice);
        meshBuffer->writeVector(meshes, 0, cmdBuf);
    }
    if (wantedBuffers.indirectDrawCommands) {
        indirectDrawCommandsBuffer = std::make_unique<vul::VulBuffer>(sizeof(VkDrawMeshTasksIndirectCommandEXT),
                indirectDrawCommands.size(), true, VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, vulDevice);
        indirectDrawCommandsBuffer->writeVector(indirectDrawCommands, 0, cmdBuf);
    }
    cmdPool.submit(cmdBuf, true);
}

}
