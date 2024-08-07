#include "vul_buffer.hpp"
#include "vul_transform.hpp"
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <glm/ext/vector_float4.hpp>
#include <string>
#include<vul_debug_tools.hpp>
#include<vul_gltf_loader.hpp>
#include<vul_scene.hpp>
#include <memory>
#include <stdexcept>
#include <iostream>

#include<tiny_gltf.h>


namespace vul
{

Scene::Scene(vul::VulDevice &vulDevice) : m_vulDevice{vulDevice}
{

}

void Scene::loadCubes(const std::vector<Cube> &cubes, const std::vector<vul::GltfLoader::Material> &mats, WantedBuffers wantedBuffers)
{
    const uint32_t oldIdxCount = indexBuffer.get() ? indexBuffer->getBufferSize() / sizeof(uint32_t) : 0;
    const uint32_t oldVertexCount = vertexBuffer.get() ? vertexBuffer->getBufferSize() / sizeof(glm::vec3) : 0;

    std::vector<glm::vec3> lVertices;
    std::vector<glm::vec3> lNormals;
    std::vector<uint32_t> lIndices;
    std::vector<GltfLoader::GltfNode> nods;
    for (size_t i = 0; i < cubes.size(); i++) {
        constexpr uint32_t FACES_PER_CUBE = 6;
        constexpr uint32_t VERTS_PER_FACE = 4;
        constexpr uint32_t VERTS_PER_CUBE = VERTS_PER_FACE * FACES_PER_CUBE;
        constexpr uint32_t IDXS_PER_FACE = 6;
        constexpr uint32_t IDXS_PER_CUBE = IDXS_PER_FACE * FACES_PER_CUBE;
        const glm::vec3 &pos = cubes[i].centerPos;
        const glm::vec3 halfDims = cubes[i].dimensions / 2.0f;

        // Top face
        lVertices.emplace_back(pos + glm::vec3{ halfDims.x,  halfDims.y,  halfDims.z});
        lVertices.emplace_back(pos + glm::vec3{-halfDims.x,  halfDims.y,  halfDims.z});
        lVertices.emplace_back(pos + glm::vec3{-halfDims.x,  halfDims.y, -halfDims.z});
        lVertices.emplace_back(pos + glm::vec3{ halfDims.x,  halfDims.y, -halfDims.z});
        for (uint32_t j = 0; j < VERTS_PER_FACE; j++) lNormals.emplace_back(glm::vec3{0.0f, 1.0f, 0.0f});

        // Bottom face
        lVertices.emplace_back(pos + glm::vec3{-halfDims.x, -halfDims.y, -halfDims.z});
        lVertices.emplace_back(pos + glm::vec3{-halfDims.x, -halfDims.y,  halfDims.z});
        lVertices.emplace_back(pos + glm::vec3{ halfDims.x, -halfDims.y,  halfDims.z});
        lVertices.emplace_back(pos + glm::vec3{ halfDims.x, -halfDims.y, -halfDims.z});
        for (uint32_t j = 0; j < VERTS_PER_FACE; j++) lNormals.emplace_back(glm::vec3{0.0f, -1.0f, 0.0f});

        // Front face
        lVertices.emplace_back(pos + glm::vec3{-halfDims.x, -halfDims.y,  halfDims.z});
        lVertices.emplace_back(pos + glm::vec3{-halfDims.x,  halfDims.y,  halfDims.z});
        lVertices.emplace_back(pos + glm::vec3{ halfDims.x,  halfDims.y,  halfDims.z});
        lVertices.emplace_back(pos + glm::vec3{ halfDims.x, -halfDims.y,  halfDims.z});
        for (uint32_t j = 0; j < VERTS_PER_FACE; j++) lNormals.emplace_back(glm::vec3{0.0f, 0.0f, 1.0f});

        // Back face
        lVertices.emplace_back(pos + glm::vec3{ halfDims.x,  halfDims.y, -halfDims.z});
        lVertices.emplace_back(pos + glm::vec3{-halfDims.x,  halfDims.y, -halfDims.z});
        lVertices.emplace_back(pos + glm::vec3{-halfDims.x, -halfDims.y, -halfDims.z});
        lVertices.emplace_back(pos + glm::vec3{ halfDims.x, -halfDims.y, -halfDims.z});
        for (uint32_t j = 0; j < VERTS_PER_FACE; j++) lNormals.emplace_back(glm::vec3{0.0f, -1.0f, -1.0f});

        // Right face
        lVertices.emplace_back(pos + glm::vec3{ halfDims.x,  halfDims.y,  halfDims.z});
        lVertices.emplace_back(pos + glm::vec3{ halfDims.x,  halfDims.y, -halfDims.z});
        lVertices.emplace_back(pos + glm::vec3{ halfDims.x, -halfDims.y, -halfDims.z});
        lVertices.emplace_back(pos + glm::vec3{ halfDims.x, -halfDims.y,  halfDims.z});
        for (uint32_t j = 0; j < VERTS_PER_FACE; j++) lNormals.emplace_back(glm::vec3{1.0f, 0.0f, 0.0f});

        // Left face
        lVertices.emplace_back(pos + glm::vec3{-halfDims.x,  halfDims.y, -halfDims.z});
        lVertices.emplace_back(pos + glm::vec3{-halfDims.x,  halfDims.y,  halfDims.z});
        lVertices.emplace_back(pos + glm::vec3{-halfDims.x, -halfDims.y,  halfDims.z});
        lVertices.emplace_back(pos + glm::vec3{-halfDims.x, -halfDims.y, -halfDims.z});
        for (uint32_t j = 0; j < VERTS_PER_FACE; j++) lNormals.emplace_back(glm::vec3{-1.0f, 0.0f, 0.0f});

        for (uint32_t j = 0; j < FACES_PER_CUBE; j++) {
            const uint32_t vertOffset = i * VERTS_PER_CUBE + j * VERTS_PER_FACE;
            lIndices.push_back(vertOffset);
            lIndices.push_back(vertOffset + 1);
            lIndices.push_back(vertOffset + 2);
            lIndices.push_back(vertOffset + 2);
            lIndices.push_back(vertOffset + 3);
            lIndices.push_back(vertOffset);
        }

        GltfLoader::GltfPrimMesh mesh;
        mesh.name = "Procedural cube mesh " + std::to_string(i);
        mesh.posMin = pos - halfDims;
        mesh.posMax = pos + halfDims;
        mesh.firstIndex = IDXS_PER_CUBE * i + oldIdxCount;
        mesh.indexCount = IDXS_PER_CUBE;
        mesh.vertexOffset = oldVertexCount;
        mesh.vertexCount = VERTS_PER_CUBE;
        mesh.materialIndex = cubes[i].matIdx + materials.size();
        meshes.push_back(mesh);

        transform3D transform;
        transform.pos = glm::vec3(0.0f);
        transform.scale = glm::vec3(1.0f);
        transform.rot = glm::vec3(0.0f);

        GltfLoader::GltfNode node;
        node.name = "Procedural cube " + std::to_string(i);
        node.primMesh = meshes.size() - 1;
        node.position = pos;
        node.worldMatrix = transform.transformMat();
        node.normalMatrix = transform.normalMat();
        nods.push_back(node);
    }
    nodes.insert(nodes.end(), nods.begin(), nods.end());
    materials.insert(materials.end(), mats.begin(), mats.end());
    
    std::vector<glm::vec4> uselessTangents(lVertices.size());
    std::vector<glm::vec2> uselessUvs(lVertices.size());

    createBuffers(lIndices, lVertices, lNormals, uselessTangents, uselessUvs, mats, nods, wantedBuffers);
}

void Scene::loadSpheres(const std::vector<Sphere> &spheres, const std::vector<vul::GltfLoader::Material> &mats, WantedBuffers wantedBuffers)
{
    const size_t oldIdxCount = indexBuffer.get() ? indexBuffer->getBufferSize() / sizeof(uint32_t) : 0;
    const size_t oldVertexCount = vertexBuffer.get() ? vertexBuffer->getBufferSize() / sizeof(glm::vec3) : 0;

    std::vector<glm::vec3> lVertices;
    std::vector<uint32_t> lIndices;
    std::vector<GltfLoader::GltfNode> nods;

    for (size_t i = 0; i < spheres.size(); i++) {
        std::vector<glm::vec3> unitVertices;
        const uint32_t lastVertCount = lVertices.size();
        const uint32_t lastIdxCount = lIndices.size();
        const glm::vec3 pos = spheres[i].centerPos;
        const float rad = spheres[i].radius;
        const uint32_t vertsPerRow = std::pow(2, spheres[i].subdivisionCount) + 1;

        uint32_t vertCount = vertsPerRow * vertsPerRow;
        for (uint32_t j = 0; j < vertsPerRow; j++) {
            const float a2 = M_PI_4f - M_PI_2f * j / (vertsPerRow - 1);
            const glm::vec3 n2 = glm::vec3(-sin(a2), cos(a2), 0);
            for (uint32_t k = 0; k < vertsPerRow; k++) {
                const float a1 = -M_PI_4f + M_PI_2f * k / (vertsPerRow - 1);
                const glm::vec3 n1 = glm::vec3(-sin(a1), 0, -cos(a1));
                unitVertices.emplace_back(glm::normalize(glm::cross(n1, n2)));
            }
        }

        // +X face
        uint32_t idxCount = 0;
        for (uint32_t j = 0; j < vertsPerRow; j++) {
            uint32_t currIdx = j * vertsPerRow;
            uint32_t nextIdx = currIdx + vertsPerRow;
            for (uint32_t k = 0; k < vertsPerRow; k++, currIdx++, nextIdx++) {
                lVertices.emplace_back(unitVertices[j * vertsPerRow + k]);
                if (j < vertsPerRow - 1 && k < vertsPerRow - 1) {
                    lIndices.push_back(nextIdx);
                    lIndices.push_back(currIdx);
                    lIndices.push_back(currIdx + 1);
                    lIndices.push_back(currIdx + 1);
                    lIndices.push_back(nextIdx + 1);
                    lIndices.push_back(nextIdx);
                    idxCount += 6;
                }
            }
        }

        // -X face
        uint32_t startIdx = vertCount;
        for (uint32_t j = lastVertCount; j < vertCount + lastVertCount; j++) lVertices.emplace_back(glm::vec3(-lVertices[j].x, lVertices[j].y, -lVertices[j].z));
        for (uint32_t j = lastIdxCount; j < idxCount + lastIdxCount; j++) lIndices.push_back(startIdx + lIndices[j]);
        // +Y face
        startIdx += vertCount;
        for (uint32_t j = lastVertCount; j < vertCount + lastVertCount; j++) lVertices.emplace_back(glm::vec3(-lVertices[j].z, lVertices[j].x, -lVertices[j].y));
        for (uint32_t j = lastIdxCount; j < idxCount + lastIdxCount; j++) lIndices.push_back(startIdx + lIndices[j]);
        // -Y face
        startIdx += vertCount;
        for (uint32_t j = lastVertCount; j < vertCount + lastVertCount; j++) lVertices.emplace_back(glm::vec3(-lVertices[j].z, -lVertices[j].x, lVertices[j].y));
        for (uint32_t j = lastIdxCount; j < idxCount + lastIdxCount; j++) lIndices.push_back(startIdx + lIndices[j]);
        // +Z face
        startIdx += vertCount;
        for (uint32_t j = lastVertCount; j < vertCount + lastVertCount; j++) lVertices.emplace_back(glm::vec3(-lVertices[j].z, lVertices[j].y, lVertices[j].x));
        for (uint32_t j = lastIdxCount; j < idxCount + lastIdxCount; j++) lIndices.push_back(startIdx + lIndices[j]);
        // -Z face
        startIdx += vertCount;
        for (uint32_t j = lastVertCount; j < vertCount + lastVertCount; j++) lVertices.emplace_back(glm::vec3(lVertices[j].z, lVertices[j].y, -lVertices[j].x));
        for (uint32_t j = lastIdxCount; j < idxCount + lastIdxCount; j++) lIndices.push_back(startIdx + lIndices[j]);

        GltfLoader::GltfPrimMesh mesh;
        mesh.name = "Procedural sphere mesh " + std::to_string(i);
        mesh.posMin = pos - glm::vec3(rad);
        mesh.posMax = pos + glm::vec3(rad);
        mesh.indexCount = idxCount * 6;
        mesh.firstIndex = lIndices.size() - mesh.indexCount + oldIdxCount;
        mesh.vertexCount = vertCount * 6;
        mesh.vertexOffset = lVertices.size() - mesh.vertexCount + oldVertexCount;
        mesh.materialIndex = spheres[i].matIdx + materials.size();
        meshes.push_back(mesh);

        transform3D transform;
        transform.pos = pos;
        transform.scale = glm::vec3(rad);
        transform.rot = glm::vec3(0.0f);

        GltfLoader::GltfNode node;
        node.name = "Procedural sphere " + std::to_string(i);
        node.primMesh = meshes.size() - 1;
        node.position = pos;
        node.worldMatrix = transform.transformMat();
        node.normalMatrix = transform.normalMat();
        nods.push_back(node);
    }
    nodes.insert(nodes.end(), nods.begin(), nods.end());
    materials.insert(materials.end(), mats.begin(), mats.end());

    std::vector<glm::vec4> uselessTangents(lVertices.size());
    std::vector<glm::vec2> uselessUvs(lVertices.size());

    createBuffers(lIndices, lVertices, lVertices, uselessTangents, uselessUvs, mats, nods, wantedBuffers);
}

void Scene::loadPlanes(const std::vector<Plane> &planes, const std::vector<GltfLoader::Material> &mats, WantedBuffers wantedBuffers)
{
    std::vector<uint32_t> lIndices;
    std::vector<glm::vec3> lVertices;
    std::vector<glm::vec2> lUvs;
    std::vector<GltfLoader::GltfNode> nods;
    for (size_t i = 0; i < planes.size(); i++) {
        lVertices.push_back(planes[i].topLeftCorner);    
        lVertices.push_back(planes[i].topRightCorner);    
        lVertices.push_back(planes[i].bottomLeftCorner);    
        lVertices.push_back(planes[i].bottomRightCorner);    
        
        lUvs.emplace_back(glm::vec2{0.0f, 0.0f});
        lUvs.emplace_back(glm::vec2{1.0f, 0.0f});
        lUvs.emplace_back(glm::vec2{0.0f, 1.0f});
        lUvs.emplace_back(glm::vec2{1.0f, 1.0f});

        lIndices.push_back(0);
        lIndices.push_back(1);
        lIndices.push_back(2);
        lIndices.push_back(2);
        lIndices.push_back(1);
        lIndices.push_back(3);

        GltfLoader::GltfPrimMesh mesh;
        mesh.name = "Procedural sphere mesh " + std::to_string(i);
        mesh.posMin = glm::min(glm::min(glm::min(planes[i].topLeftCorner, planes[i].topRightCorner), planes[i].bottomLeftCorner), planes[i].bottomRightCorner);
        mesh.posMax = glm::max(glm::max(glm::max(planes[i].topLeftCorner, planes[i].topRightCorner), planes[i].bottomLeftCorner), planes[i].bottomRightCorner);
        mesh.indexCount = 6;
        mesh.firstIndex = lIndices.size() - mesh.indexCount + indices.size();
        mesh.vertexCount = 4;
        mesh.vertexOffset = lVertices.size() - mesh.vertexCount + vertices.size();
        mesh.materialIndex = planes[i].matIdx + materials.size();
        meshes.push_back(mesh);

        transform3D transform{};

        GltfLoader::GltfNode node;
        node.name = "Procedural plane " + std::to_string(i);
        node.primMesh = meshes.size() - 1;
        node.position = {};
        node.worldMatrix = transform.transformMat();
        node.normalMatrix = transform.normalMat();
        nods.push_back(node);
    }
    nodes.insert(nodes.end(), nods.begin(), nods.end());
    materials.insert(materials.end(), mats.begin(), mats.end());

    std::vector<glm::vec4> uselessTangents(lVertices.size());
    std::vector<glm::vec3> uselessNormals(lVertices.size());

    createBuffers(lIndices, lVertices, uselessNormals, uselessTangents, lUvs, mats, nods, wantedBuffers);
}

void Scene::loadScene(const std::string &fileName, std::string textureDirectory, WantedBuffers wantedBuffers)
{
    tinygltf::Model model;
    tinygltf::TinyGLTF context;
    std::string warn, err;
    
    if (!context.LoadASCIIFromFile(&model, &err, &warn, fileName)) 
        throw std::runtime_error("Failed to load scene from file: " + err);

    if (textureDirectory[textureDirectory.length() - 1] != '/') textureDirectory += '/';

    GltfLoader gltfLoader;
    gltfLoader.importMaterials(model);
    gltfLoader.importTextures(model, textureDirectory, m_vulDevice);
    gltfLoader.importDrawableNodes(model, GltfLoader::gltfAttribOr(GltfLoader::gltfAttribOr(GltfLoader::GltfAttributes::Normal,
                    GltfLoader::GltfAttributes::Tangent), GltfLoader::GltfAttributes::TexCoord));

    const uint32_t oldIdxCount = indexBuffer.get() ? indexBuffer->getBufferSize() / sizeof(uint32_t) : 0;
    const uint32_t oldVertCount = vertexBuffer.get() ? vertexBuffer->getBufferSize() / sizeof(glm::vec3) : 0;
    const uint32_t oldMatCount = materials.size();
    const uint32_t oldMeshCount = meshes.size();
    const uint32_t oldImgCount = images.size();
    for (GltfLoader::GltfPrimMesh &mesh : gltfLoader.primMeshes) {
        mesh.firstIndex += oldIdxCount;
        mesh.vertexOffset += oldVertCount;
        mesh.materialIndex += oldMatCount;
    }
    for (GltfLoader::GltfNode &node : gltfLoader.nodes) node.primMesh += oldMeshCount;
    for (GltfLoader::Material &mat : gltfLoader.materials) {
        if (mat.colorTextureIndex >= 0) mat.colorTextureIndex += oldImgCount;
        if (mat.normalTextureIndex >= 0) mat.normalTextureIndex += oldImgCount;
        if (mat.roughnessMetallinessTextureIndex >= 0) mat.roughnessMetallinessTextureIndex += oldImgCount;
    }

    lights.insert(lights.end(), gltfLoader.lights.begin(), gltfLoader.lights.end());
    nodes.insert(nodes.end(), gltfLoader.nodes.begin(), gltfLoader.nodes.end());
    meshes.insert(meshes.end(), gltfLoader.primMeshes.begin(), gltfLoader.primMeshes.end());
    materials.insert(materials.end(), gltfLoader.materials.begin(), gltfLoader.materials.end());
    images.insert(images.end(), gltfLoader.images.begin(), gltfLoader.images.end());

    createBuffers(gltfLoader.indices, gltfLoader.positions, gltfLoader.normals, gltfLoader.tangents, gltfLoader.uvCoords, gltfLoader.materials, gltfLoader.nodes, wantedBuffers);
}

void Scene::createBuffers(const std::vector<uint32_t> &lIndices, const std::vector<glm::vec3> &lVertices,
                const std::vector<glm::vec3> &lNormals, const std::vector<glm::vec4> &lTangents, const std::vector<glm::vec2> &lUvs,
                const std::vector<vul::GltfLoader::Material> &mats, const std::vector<vul::GltfLoader::GltfNode> &nods, WantedBuffers wantedBuffers)
{
    std::vector<PackedMaterial> packedMaterials;
    for (const GltfLoader::Material &mat : mats) {
        PackedMaterial packedMat;
        packedMat.colorFactor = mat.colorFactor;
        packedMat.emissiveFactor = glm::vec4(mat.emissiveFactor, mat.emissionStrength);
        packedMat.roughness = mat.roughness;
        packedMat.metalliness = mat.metalliness;
        packedMat.colorTextureIndex = mat.colorTextureIndex;
        packedMat.normalTextureIndex = mat.normalTextureIndex;
        packedMat.roughnessMetallicTextureIndex = mat.roughnessMetallinessTextureIndex;
        packedMat.padding1 = 42;
        packedMat.padding2 = 69;
        packedMat.padding3 = 420;
        packedMaterials.push_back(packedMat);
    }
    std::vector<PrimInfo> primInfos;
    for (const GltfLoader::GltfNode &node : nods) {
        const GltfLoader::GltfPrimMesh &mesh = meshes[node.primMesh];

        PrimInfo primInfo;
        primInfo.transformMatrix = node.worldMatrix;
        primInfo.normalMatrix = node.normalMatrix;
        primInfo.firstIndex = mesh.firstIndex;
        primInfo.vertexOffset = mesh.vertexOffset;
        primInfo.materialIndex = mesh.materialIndex;
        primInfo.padding = 69;
        primInfos.push_back(primInfo);
    }

    VulBuffer::Usage optionalFlags = VulBuffer::usage_none;
    if (wantedBuffers.enableAddressTaking) optionalFlags =  VulBuffer::usage_getAddress;
    if (wantedBuffers.enableUsageForAccelerationStructures) optionalFlags = static_cast<VulBuffer::Usage>(optionalFlags | VulBuffer::usage_accelerationStructureBuildRead);

    if (lIndices.size() > 0 && wantedBuffers.index) {
        if (indexBuffer.get() == nullptr) {
            indexBuffer = std::make_unique<vul::VulBuffer>(m_vulDevice);
            indexBuffer->loadVector(lIndices);
            indexBuffer->createBuffer(true, static_cast<VulBuffer::Usage>(VulBuffer::usage_indexBuffer |
                        VulBuffer::usage_transferDst | VulBuffer::usage_transferSrc | VulBuffer::usage_ssbo | optionalFlags));
        } else indexBuffer->appendVector(lIndices);
        indices.insert(indices.end(), lIndices.begin(), lIndices.end());
        VUL_NAME_VK(indexBuffer->getBuffer())
    }
    if (lVertices.size() > 0 && wantedBuffers.vertex) {
        if (vertexBuffer.get() == nullptr) {
            vertexBuffer = std::make_unique<vul::VulBuffer>(m_vulDevice);
            vertexBuffer->loadVector(lVertices);
            vertexBuffer->createBuffer(true, static_cast<VulBuffer::Usage>(VulBuffer::usage_vertexBuffer |
                        VulBuffer::usage_transferDst | VulBuffer::usage_transferSrc | VulBuffer::usage_ssbo | optionalFlags));
        } else vertexBuffer->appendVector(lVertices);
        vertices.insert(vertices.end(), lVertices.begin(), lVertices.end());
        VUL_NAME_VK(vertexBuffer->getBuffer())
    }
    if (lNormals.size() > 0 && wantedBuffers.normal) {
        if (normalBuffer.get() == nullptr) {
            normalBuffer = std::make_unique<vul::VulBuffer>(m_vulDevice);
            normalBuffer->loadVector(lNormals);
            normalBuffer->createBuffer(true, static_cast<VulBuffer::Usage>(VulBuffer::usage_vertexBuffer |
                        VulBuffer::usage_transferDst | VulBuffer::usage_transferSrc | VulBuffer::usage_ssbo));
        } else normalBuffer->appendVector(lNormals);
        normals.insert(normals.end(), lNormals.begin(), lNormals.end());
        VUL_NAME_VK(normalBuffer->getBuffer())
    }
    if (lTangents.size() > 0 && wantedBuffers.tangent) {
        if (tangentBuffer.get() == nullptr) {
            tangentBuffer = std::make_unique<vul::VulBuffer>(m_vulDevice);
            tangentBuffer->loadVector(lTangents);
            tangentBuffer->createBuffer(true, static_cast<VulBuffer::Usage>(VulBuffer::usage_vertexBuffer |
                        VulBuffer::usage_transferDst | VulBuffer::usage_transferSrc | VulBuffer::usage_ssbo));
        } else tangentBuffer->appendVector(lTangents);
        tangents.insert(tangents.end(), lTangents.begin(), lTangents.end());
        VUL_NAME_VK(tangentBuffer->getBuffer())
    }
    if (lUvs.size() > 0 && wantedBuffers.uv) {
        if (uvBuffer.get() == nullptr) {
            uvBuffer = std::make_unique<vul::VulBuffer>(m_vulDevice);
            uvBuffer->loadVector(lUvs);
            uvBuffer->createBuffer(true, static_cast<VulBuffer::Usage>(VulBuffer::usage_vertexBuffer |
                        VulBuffer::usage_transferDst | VulBuffer::usage_transferSrc | VulBuffer::usage_ssbo));
        } else uvBuffer->appendVector(lUvs);
        uvs.insert(uvs.end(), lUvs.begin(), lUvs.end());
        VUL_NAME_VK(uvBuffer->getBuffer())
    }
    if (packedMaterials.size() > 0 && wantedBuffers.material) {
        if (materialBuffer.get() == nullptr) {
            materialBuffer = std::make_unique<vul::VulBuffer>(m_vulDevice);
            materialBuffer->loadVector(packedMaterials);
            materialBuffer->createBuffer(true, static_cast<VulBuffer::Usage>(VulBuffer::usage_ssbo |
                        VulBuffer::usage_transferDst | VulBuffer::usage_transferSrc));
        } else materialBuffer->appendVector(packedMaterials);
        VUL_NAME_VK(materialBuffer->getBuffer())
    }
    if (primInfos.size() > 0 && wantedBuffers.primInfo) {
        if (primInfoBuffer.get() == nullptr) {
            primInfoBuffer = std::make_unique<vul::VulBuffer>(m_vulDevice);
            primInfoBuffer->loadVector(primInfos);
            primInfoBuffer->createBuffer(true, static_cast<VulBuffer::Usage>(VulBuffer::usage_ssbo |
                        VulBuffer::usage_transferDst | VulBuffer::usage_transferSrc));
        } else primInfoBuffer->appendVector(primInfos);
        VUL_NAME_VK(primInfoBuffer->getBuffer())
    }
}

}
