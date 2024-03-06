#include"../Headers/vul_scene.hpp"
#include"../Headers/vul_host_device.hpp"
#include <memory>
#include <stdexcept>
#include <vulkan/vulkan_core.h>
#include <iostream>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include"../../../3rdParty/tiny_gltf.h"

Scene::Scene(vulB::VulDevice &vulDevice) : m_vulDevice{vulDevice}
{

}

void Scene::loadScene(std::string fileName)
{
    tinygltf::Model model;
    tinygltf::TinyGLTF context;
    std::string warn, err;
    
    if (!context.LoadBinaryFromFile(&model, &err, &warn, fileName)) 
        throw std::runtime_error(std::string("Failed to load scene from file. File name is ") + fileName);

    m_gltfLoader.importMaterials(model);
    m_gltfLoader.importDrawableNodes(   model, GltfLoader::gltfAttribOr(GltfLoader::GltfAttributes::Normal, 
                                        GltfLoader::GltfAttributes::TexCoord));

    std::vector<glm::vec4> alignedPositions(m_gltfLoader.positions.size());
    std::vector<glm::vec4> alignedNormals(m_gltfLoader.normals.size());
    for (size_t i = 0; i < m_gltfLoader.positions.size(); i++){
        alignedPositions[i] = glm::vec4(m_gltfLoader.positions[i], 0.0f);
        alignedNormals[i] = glm::vec4(m_gltfLoader.normals[i], 0.0f);
    }

    struct alignedPrimInfo{
        uint32_t indexOffset;
        uint32_t vertexOffset;
        int materialIndex;
        int padding;
    };
    std::vector<alignedPrimInfo> primLookup;
    for (GltfLoader::GltfPrimMesh &primMesh : m_gltfLoader.primMeshes){
        alignedPrimInfo primInfo;
        primInfo.indexOffset = primMesh.firstIndex;
        primInfo.vertexOffset = primMesh.vertexOffset;
        primInfo.materialIndex = primMesh.materialIndex;
        primInfo.padding = 69;
        primLookup.push_back(primInfo);
    }

    std::vector<PackedMaterial> packedMaterials;
    for (const GltfLoader::Material &mat : m_gltfLoader.materials){
        PackedMaterial packedMat;
        packedMat.colorFactor = mat.colorFactor;
        packedMat.emissiveFactor = glm::vec4(mat.emissiveFactor, mat.emissionStrength);
        packedMat.roughness = mat.roughness;
        packedMat.colorTextureIndex = mat.colorTextureIndex;
        packedMat.ior = mat.ior;
        packedMat.padding = 69;
        packedMaterials.push_back(packedMat);
    }

    indexBuffer = vulB::VulBuffer::createLocalBufferFromData(m_vulDevice, m_gltfLoader.indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT); 
    vertexBuffer = vulB::VulBuffer::createLocalBufferFromData(m_vulDevice, alignedPositions, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT); 
    normalBuffer = vulB::VulBuffer::createLocalBufferFromData(m_vulDevice, alignedNormals, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    uvBuffer = vulB::VulBuffer::createLocalBufferFromData(m_vulDevice, m_gltfLoader.uvCoords, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    materialBuffer = vulB::VulBuffer::createLocalBufferFromData(m_vulDevice, packedMaterials, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    primInfoBuffer = vulB::VulBuffer::createLocalBufferFromData(m_vulDevice, primLookup, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

    lights = m_gltfLoader.lights;
    nodes = m_gltfLoader.nodes;
    meshes = m_gltfLoader.primMeshes;
    materials = m_gltfLoader.materials;

    m_gltfLoader.lights.clear();
    m_gltfLoader.nodes.clear();
    m_gltfLoader.primMeshes.clear();
    m_gltfLoader.materials.clear();
    m_gltfLoader.lights.shrink_to_fit();
    m_gltfLoader.nodes.shrink_to_fit();
    m_gltfLoader.primMeshes.shrink_to_fit();
    m_gltfLoader.materials.shrink_to_fit();
}
