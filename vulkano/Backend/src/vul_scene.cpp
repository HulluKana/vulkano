#include"../Headers/vul_scene.hpp"
#include"../Headers/vul_host_device.hpp"
#include <memory>
#include <stdexcept>
#include <vulkan/vulkan_core.h>
#include <iostream>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include"../../../3rdParty/tiny_gltf.h"

using namespace vulB;
namespace vul
{

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

    GltfLoader gltfLoader;
    gltfLoader.importMaterials(model);
    gltfLoader.importTextures(model, m_vulDevice);
    gltfLoader.importDrawableNodes(   model, GltfLoader::gltfAttribOr(GltfLoader::GltfAttributes::Normal, 
                                        GltfLoader::GltfAttributes::TexCoord));

    struct alignedPrimInfo{
        uint32_t indexOffset;
        uint32_t vertexOffset;
        int materialIndex;
        int padding;
    };
    std::vector<alignedPrimInfo> primLookup;
    for (GltfLoader::GltfPrimMesh &primMesh : gltfLoader.primMeshes){
        alignedPrimInfo primInfo;
        primInfo.indexOffset = primMesh.firstIndex;
        primInfo.vertexOffset = primMesh.vertexOffset;
        primInfo.materialIndex = primMesh.materialIndex;
        primInfo.padding = 69;
        primLookup.push_back(primInfo);
    }

    std::vector<PackedMaterial> packedMaterials;
    for (const GltfLoader::Material &mat : gltfLoader.materials){
        PackedMaterial packedMat;
        packedMat.colorFactor = mat.colorFactor;
        packedMat.emissiveFactor = glm::vec4(mat.emissiveFactor, mat.emissionStrength);
        packedMat.roughness = mat.roughness;
        packedMat.metalliness = mat.metalliness;
        packedMat.colorTextureIndex = mat.colorTextureIndex;
        packedMat.ior = mat.ior;
        packedMaterials.push_back(packedMat);
    }

    indexBuffer = vulB::VulBuffer::createLocalBufferFromData(m_vulDevice, gltfLoader.indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT); 
    vertexBuffer = vulB::VulBuffer::createLocalBufferFromData(m_vulDevice, gltfLoader.positions, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT); 
    normalBuffer = vulB::VulBuffer::createLocalBufferFromData(m_vulDevice, gltfLoader.normals, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    uvBuffer = vulB::VulBuffer::createLocalBufferFromData(m_vulDevice, gltfLoader.uvCoords, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    materialBuffer = vulB::VulBuffer::createLocalBufferFromData(m_vulDevice, packedMaterials, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    primInfoBuffer = vulB::VulBuffer::createLocalBufferFromData(m_vulDevice, primLookup, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

    lights = gltfLoader.lights;
    nodes = gltfLoader.nodes;
    meshes = gltfLoader.primMeshes;
    materials = gltfLoader.materials;
    images = gltfLoader.images;
}

}
