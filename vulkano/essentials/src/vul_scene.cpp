#include "vul_buffer.hpp"
#include<vul_scene.hpp>
#include<vul_host_device.hpp>
#include <memory>
#include <stdexcept>
#include <vulkan/vulkan_core.h>
#include <iostream>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include<tiny_gltf.h>

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

    indexBuffer = std::make_unique<vulB::VulBuffer>(m_vulDevice);
    vertexBuffer = std::make_unique<vulB::VulBuffer>(m_vulDevice);
    normalBuffer = std::make_unique<vulB::VulBuffer>(m_vulDevice);
    uvBuffer = std::make_unique<vulB::VulBuffer>(m_vulDevice);
    materialBuffer = std::make_unique<vulB::VulBuffer>(m_vulDevice);

    indexBuffer->loadVector(gltfLoader.indices);
    vertexBuffer->loadVector(gltfLoader.positions);
    normalBuffer->loadVector(gltfLoader.normals);
    uvBuffer->loadVector(gltfLoader.uvCoords);
    materialBuffer->loadVector(packedMaterials);

    indexBuffer->createBuffer(true, static_cast<VulBuffer::Usage>(VulBuffer::usage_indexBuffer | VulBuffer::usage_transferDst));
    vertexBuffer->createBuffer(true, static_cast<VulBuffer::Usage>(VulBuffer::usage_vertexBuffer | VulBuffer::usage_transferDst));
    normalBuffer->createBuffer(true, static_cast<VulBuffer::Usage>(VulBuffer::usage_vertexBuffer | VulBuffer::usage_transferDst));
    uvBuffer->createBuffer(true, static_cast<VulBuffer::Usage>(VulBuffer::usage_vertexBuffer | VulBuffer::usage_transferDst));
    materialBuffer->createBuffer(true, static_cast<VulBuffer::Usage>(VulBuffer::usage_ssbo | VulBuffer::usage_transferDst));

    lights = gltfLoader.lights;
    nodes = gltfLoader.nodes;
    meshes = gltfLoader.primMeshes;
    materials = gltfLoader.materials;
    images = gltfLoader.images;
}

}
