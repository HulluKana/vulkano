#include "vul_buffer.hpp"
#include "vul_settings.hpp"
#include "vul_transform.hpp"
#include <cmath>
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

using namespace vulB;
namespace vul
{

Scene::Scene(vulB::VulDevice &vulDevice) : m_vulDevice{vulDevice}
{

}

void Scene::loadCubes(const std::vector<Cube> &cubes, WantedBuffers wantedBuffers)
{
    std::vector<glm::vec3> vertices;
    std::vector<uint> indices;
    std::vector<PrimInfo> primInfos;
    for (size_t i = 0; i < cubes.size(); i++) {
        const glm::vec3 &pos = cubes[i].centerPos;
        const glm::vec3 halfDims = cubes[i].dimensions / 2.0f;
        size_t vertOffset = i * 8;

        vertices.emplace_back(pos + glm::vec3{ halfDims.x,  halfDims.y,  halfDims.z});
        vertices.emplace_back(pos + glm::vec3{ halfDims.x,  halfDims.y, -halfDims.z});
        vertices.emplace_back(pos + glm::vec3{ halfDims.x, -halfDims.y,  halfDims.z});
        vertices.emplace_back(pos + glm::vec3{ halfDims.x, -halfDims.y, -halfDims.z});
        vertices.emplace_back(pos + glm::vec3{-halfDims.x,  halfDims.y,  halfDims.z});
        vertices.emplace_back(pos + glm::vec3{-halfDims.x,  halfDims.y, -halfDims.z});
        vertices.emplace_back(pos + glm::vec3{-halfDims.x, -halfDims.y,  halfDims.z});
        vertices.emplace_back(pos + glm::vec3{-halfDims.x, -halfDims.y, -halfDims.z});

        // Top face
        indices.push_back(vertOffset + 0);
        indices.push_back(vertOffset + 4);
        indices.push_back(vertOffset + 5);
        indices.push_back(vertOffset + 5);
        indices.push_back(vertOffset + 1);
        indices.push_back(vertOffset + 0);

        // Bottom face
        indices.push_back(vertOffset + 7);
        indices.push_back(vertOffset + 6);
        indices.push_back(vertOffset + 2);
        indices.push_back(vertOffset + 2);
        indices.push_back(vertOffset + 3);
        indices.push_back(vertOffset + 7);

        // Front face
        indices.push_back(vertOffset + 6);
        indices.push_back(vertOffset + 4);
        indices.push_back(vertOffset + 0);
        indices.push_back(vertOffset + 0);
        indices.push_back(vertOffset + 2);
        indices.push_back(vertOffset + 6);

        // Back face
        indices.push_back(vertOffset + 1);
        indices.push_back(vertOffset + 5);
        indices.push_back(vertOffset + 7);
        indices.push_back(vertOffset + 7);
        indices.push_back(vertOffset + 3);
        indices.push_back(vertOffset + 1);

        // Right face
        indices.push_back(vertOffset + 0);
        indices.push_back(vertOffset + 1);
        indices.push_back(vertOffset + 3);
        indices.push_back(vertOffset + 3);
        indices.push_back(vertOffset + 2);
        indices.push_back(vertOffset + 0);

        // Left face
        indices.push_back(vertOffset + 5);
        indices.push_back(vertOffset + 4);
        indices.push_back(vertOffset + 6);
        indices.push_back(vertOffset + 6);
        indices.push_back(vertOffset + 7);
        indices.push_back(vertOffset + 5);

        GltfLoader::GltfPrimMesh mesh;
        mesh.name = "Procedural cube mesh " + std::to_string(i);
        mesh.posMin = pos - halfDims;
        mesh.posMax = pos + halfDims;
        mesh.firstIndex = 36 * i;
        mesh.indexCount = 36;
        mesh.vertexOffset = 0;
        mesh.vertexCount = 8;
        mesh.materialIndex = 0;
        meshes.push_back(mesh);

        transform3D transform;
        transform.pos = glm::vec3(0.0f);
        transform.scale = glm::vec3(1.0f);
        transform.rot = glm::vec3(0.0f);

        GltfLoader::GltfNode node;
        node.name = "Procedural cube " + std::to_string(i);
        node.primMesh = i;
        node.position = pos;
        node.worldMatrix = transform.transformMat();
        node.normalMatrix = transform.normalMat();
        nodes.push_back(node);

        PrimInfo primInfo;
        primInfo.transformMatrix = node.worldMatrix;
        primInfo.normalMatrix = node.normalMatrix;
        primInfo.materialIndex = mesh.materialIndex;
        primInfo.firstIndex = mesh.firstIndex;
        primInfo.vertexOffset = mesh.vertexOffset;
        primInfo.padding = 69;
        primInfos.push_back(primInfo);
    }

    PackedMaterial packedMat;
    packedMat.colorFactor = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);
    packedMat.emissiveFactor = glm::vec4(0.0f);
    packedMat.roughness = 0.5f;
    packedMat.metalliness = 0.0f;
    packedMat.colorTextureIndex = -1;
    packedMat.normalTextureIndex = -1;
    packedMat.roughnessMetallicTextureIndex = -1;
    packedMat.padding1 = 42;
    packedMat.padding2 = 69;
    packedMat.padding3 = 420;

    createBuffers(indices, vertices, {}, {}, {}, {packedMat}, primInfos, wantedBuffers);
}

void Scene::loadSpheres(const std::vector<Sphere> &spheres, const std::vector<vulB::GltfLoader::Material> &mats, WantedBuffers wantedBuffers)
{
    const size_t oldIdxCount = indexBuffer.get() ? indexBuffer->getBufferSize() / sizeof(uint32_t) : 0;
    const size_t oldVertexCount = vertexBuffer.get() ? vertexBuffer->getBufferSize() / sizeof(glm::vec3) : 0;

    std::vector<glm::vec3> vertices;
    std::vector<uint32_t> indices;
    std::vector<PrimInfo> primInfos;

    for (size_t i = 0; i < spheres.size(); i++) {
        std::vector<glm::vec3> unitVertices;
        const uint32_t lastVertCount = vertices.size();
        const uint32_t lastIdxCount = indices.size();
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
                vertices.emplace_back(unitVertices[j * vertsPerRow + k]);
                if (j < vertsPerRow - 1 && j < vertsPerRow - 1) {
                    indices.push_back(nextIdx);
                    indices.push_back(currIdx);
                    indices.push_back(currIdx + 1);
                    indices.push_back(currIdx + 1);
                    indices.push_back(nextIdx + 1);
                    indices.push_back(nextIdx);
                    idxCount += 6;
                }
            }
        }

        // -X face
        uint32_t startIdx = vertCount;
        for (uint32_t j = lastVertCount; j < vertCount + lastVertCount; j++) vertices.emplace_back(glm::vec3(-vertices[j].x, vertices[j].y, -vertices[j].z));
        for (uint32_t j = lastIdxCount; j < idxCount + lastIdxCount; j++) indices.push_back(startIdx + indices[j]);
        // +Y face
        startIdx += vertCount;
        for (uint32_t j = lastVertCount; j < vertCount + lastVertCount; j++) vertices.emplace_back(glm::vec3(-vertices[j].z, vertices[j].x, -vertices[j].y));
        for (uint32_t j = lastIdxCount; j < idxCount + lastIdxCount; j++) indices.push_back(startIdx + indices[j]);
        // -Y face
        startIdx += vertCount;
        for (uint32_t j = lastVertCount; j < vertCount + lastVertCount; j++) vertices.emplace_back(glm::vec3(-vertices[j].z, -vertices[j].x, vertices[j].y));
        for (uint32_t j = lastIdxCount; j < idxCount + lastIdxCount; j++) indices.push_back(startIdx + indices[j]);
        // +Z face
        startIdx += vertCount;
        for (uint32_t j = lastVertCount; j < vertCount + lastVertCount; j++) vertices.emplace_back(glm::vec3(-vertices[j].z, vertices[j].y, vertices[j].x));
        for (uint32_t j = lastIdxCount; j < idxCount + lastIdxCount; j++) indices.push_back(startIdx + indices[j]);
        // -Z face
        startIdx += vertCount;
        for (uint32_t j = lastVertCount; j < vertCount + lastVertCount; j++) vertices.emplace_back(glm::vec3(vertices[j].z, vertices[j].y, -vertices[j].x));
        for (uint32_t j = lastIdxCount; j < idxCount + lastIdxCount; j++) indices.push_back(startIdx + indices[j]);

        GltfLoader::GltfPrimMesh mesh;
        mesh.name = "Procedural sphere mesh " + std::to_string(i);
        mesh.posMin = pos - glm::vec3(rad);
        mesh.posMax = pos + glm::vec3(rad);
        mesh.indexCount = idxCount * 6;
        mesh.firstIndex = indices.size() - mesh.indexCount + oldIdxCount;
        mesh.vertexCount = vertCount * 6;
        mesh.vertexOffset = vertices.size() - mesh.vertexCount + oldVertexCount;
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
        nodes.push_back(node);

        PrimInfo primInfo;
        primInfo.transformMatrix = node.worldMatrix;
        primInfo.normalMatrix = node.normalMatrix;
        primInfo.materialIndex = mesh.materialIndex;
        primInfo.firstIndex = mesh.firstIndex;
        primInfo.vertexOffset = mesh.vertexOffset;
        primInfo.padding = 69;
        primInfos.push_back(primInfo);
    }

    std::vector<PackedMaterial> packedMats(mats.size());
    for (size_t i = 0; i < mats.size(); i++) {
        const GltfLoader::Material &mat = mats[i];

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

        packedMats[i] = packedMat;
        materials.push_back(mat);
    }

    createBuffers(indices, vertices, vertices, {}, {}, packedMats, primInfos, wantedBuffers);
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

    std::vector<PackedMaterial> packedMaterials;
    for (const GltfLoader::Material &mat : gltfLoader.materials){
        PackedMaterial packedMat;
        packedMat.colorFactor = mat.colorFactor;
        packedMat.emissiveFactor = glm::vec4(mat.emissiveFactor, mat.emissionStrength);
        packedMat.roughness = mat.roughness;
        packedMat.metalliness = mat.metalliness;
        packedMat.colorTextureIndex = mat.colorTextureIndex;
        packedMat.normalTextureIndex = mat.normalTextureIndex;
        packedMat.roughnessMetallicTextureIndex = mat.roughnessMetallinessTextureIndex;
        packedMat.padding1 = 14;
        packedMat.padding2 = 69;
        packedMat.padding3 = 420;
        packedMaterials.push_back(packedMat);
    }

    std::vector<PrimInfo> primInfos;
    for (const GltfLoader::GltfNode &node : gltfLoader.nodes) {
        const GltfLoader::GltfPrimMesh &mesh = gltfLoader.primMeshes[node.primMesh];

        PrimInfo primInfo;
        primInfo.transformMatrix = node.worldMatrix;
        primInfo.normalMatrix = node.normalMatrix;
        primInfo.firstIndex = mesh.firstIndex;
        primInfo.vertexOffset = mesh.vertexOffset;
        primInfo.materialIndex = mesh.materialIndex;
        primInfo.padding = 69;
        primInfos.push_back(primInfo);
    } 

    lights = gltfLoader.lights;
    nodes = gltfLoader.nodes;
    meshes = gltfLoader.primMeshes;
    materials = gltfLoader.materials;
    images = gltfLoader.images;

    createBuffers(gltfLoader.indices, gltfLoader.positions, gltfLoader.normals, gltfLoader.tangents, gltfLoader.uvCoords, packedMaterials, primInfos, wantedBuffers);
}

void Scene::createBuffers(const std::vector<uint32_t> &indices, const std::vector<glm::vec3> &vertices,
                const std::vector<glm::vec3> &normals, const std::vector<glm::vec4> &tangents, const std::vector<glm::vec2> &uvs,
                const std::vector<PackedMaterial> &packedMaterials, const std::vector<PrimInfo> &primInfos, WantedBuffers wantedBuffers)
{
    VulBuffer::Usage rtFlags = VulBuffer::usage_none;
    if (settings::deviceInitConfig.enableRaytracingSupport)
        rtFlags = static_cast<VulBuffer::Usage>(VulBuffer::usage_getAddress | VulBuffer::usage_accelerationStructureBuildRead);

    if (indices.size() > 0 && wantedBuffers.index) {
        if (indexBuffer.get() == nullptr) {
            indexBuffer = std::make_unique<vulB::VulBuffer>(m_vulDevice);
            indexBuffer->loadVector(indices);
            indexBuffer->createBuffer(true, static_cast<VulBuffer::Usage>(VulBuffer::usage_indexBuffer |
                        VulBuffer::usage_transferDst | VulBuffer::usage_transferSrc | VulBuffer::usage_ssbo | rtFlags));
        } else indexBuffer->appendVector(indices);
        VUL_NAME_VK(indexBuffer->getBuffer())
    }
    if (vertices.size() > 0 && wantedBuffers.vertex) {
        if (vertexBuffer.get() == nullptr) {
            vertexBuffer = std::make_unique<vulB::VulBuffer>(m_vulDevice);
            vertexBuffer->loadVector(vertices);
            vertexBuffer->createBuffer(true, static_cast<VulBuffer::Usage>(VulBuffer::usage_vertexBuffer |
                        VulBuffer::usage_transferDst | VulBuffer::usage_transferSrc | VulBuffer::usage_ssbo | rtFlags));
        } else vertexBuffer->appendVector(vertices);
        VUL_NAME_VK(vertexBuffer->getBuffer())
    }
    if (normals.size() > 0 && wantedBuffers.normal) {
        if (normalBuffer.get() == nullptr) {
            normalBuffer = std::make_unique<vulB::VulBuffer>(m_vulDevice);
            normalBuffer->loadVector(normals);
            normalBuffer->createBuffer(true, static_cast<VulBuffer::Usage>(VulBuffer::usage_vertexBuffer |
                        VulBuffer::usage_transferDst | VulBuffer::usage_transferSrc | VulBuffer::usage_ssbo));
        } else normalBuffer->appendVector(normals);
        VUL_NAME_VK(normalBuffer->getBuffer())
    }
    if (tangents.size() > 0 && wantedBuffers.tangent) {
        if (tangentBuffer.get() == nullptr) {
            tangentBuffer = std::make_unique<vulB::VulBuffer>(m_vulDevice);
            tangentBuffer->loadVector(tangents);
            tangentBuffer->createBuffer(true, static_cast<VulBuffer::Usage>(VulBuffer::usage_vertexBuffer |
                        VulBuffer::usage_transferDst | VulBuffer::usage_transferSrc | VulBuffer::usage_ssbo));
        } else tangentBuffer->appendVector(tangents);
        VUL_NAME_VK(tangentBuffer->getBuffer())
    }
    if (uvs.size() > 0 && wantedBuffers.uv) {
        if (uvBuffer.get() == nullptr) {
            uvBuffer = std::make_unique<vulB::VulBuffer>(m_vulDevice);
            uvBuffer->loadVector(uvs);
            uvBuffer->createBuffer(true, static_cast<VulBuffer::Usage>(VulBuffer::usage_vertexBuffer |
                        VulBuffer::usage_transferDst | VulBuffer::usage_transferSrc | VulBuffer::usage_ssbo));
        } else uvBuffer->appendVector(uvs);
        VUL_NAME_VK(uvBuffer->getBuffer())
    }
    if (packedMaterials.size() > 0 && wantedBuffers.material) {
        if (materialBuffer.get() == nullptr) {
            materialBuffer = std::make_unique<vulB::VulBuffer>(m_vulDevice);
            materialBuffer->loadVector(packedMaterials);
            materialBuffer->createBuffer(true, static_cast<VulBuffer::Usage>(VulBuffer::usage_ssbo |
                        VulBuffer::usage_transferDst | VulBuffer::usage_transferSrc));
        } else materialBuffer->appendVector(packedMaterials);
        VUL_NAME_VK(materialBuffer->getBuffer())
    }
    if (primInfos.size() > 0 && wantedBuffers.primInfo) {
        if (primInfoBuffer.get() == nullptr) {
            primInfoBuffer = std::make_unique<vulB::VulBuffer>(m_vulDevice);
            primInfoBuffer->loadVector(primInfos);
            primInfoBuffer->createBuffer(true, static_cast<VulBuffer::Usage>(VulBuffer::usage_ssbo |
                        VulBuffer::usage_transferDst | VulBuffer::usage_transferSrc));
        } else primInfoBuffer->appendVector(primInfos);
        VUL_NAME_VK(primInfoBuffer->getBuffer())
    }
}

}
