#include "vul_buffer.hpp"
#include "vul_settings.hpp"
#include <cstdlib>
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

void Scene::loadScene(const std::string &fileName, std::string textureDirectory)
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

    std::vector<LightInfo> lightInfos;
    for (const GltfLoader::GltfLight &light : gltfLoader.lights) {
        LightInfo lightInfo;
        lightInfo.lightPosition = glm::vec4(light.position, light.range);
        lightInfo.lightColor = glm::vec4(light.color, light.intensity);
        lightInfos.push_back(lightInfo);
    }

    indexBuffer = std::make_unique<vulB::VulBuffer>(m_vulDevice);
    vertexBuffer = std::make_unique<vulB::VulBuffer>(m_vulDevice);
    normalBuffer = std::make_unique<vulB::VulBuffer>(m_vulDevice);
    tangentBuffer = std::make_unique<vulB::VulBuffer>(m_vulDevice);
    uvBuffer = std::make_unique<vulB::VulBuffer>(m_vulDevice);
    materialBuffer = std::make_unique<vulB::VulBuffer>(m_vulDevice);
    primInfoBuffer = std::make_unique<vulB::VulBuffer>(m_vulDevice);
    lightsBuffer = std::make_unique<vulB::VulBuffer>(m_vulDevice);

    indexBuffer->loadVector(gltfLoader.indices);
    vertexBuffer->loadVector(gltfLoader.positions);
    normalBuffer->loadVector(gltfLoader.normals);
    tangentBuffer->loadVector(gltfLoader.tangents);
    uvBuffer->loadVector(gltfLoader.uvCoords);
    materialBuffer->loadVector(packedMaterials);
    primInfoBuffer->loadVector(primInfos);
    lightsBuffer->loadVector(lightInfos);

    VulBuffer::Usage rtFlags = VulBuffer::usage_none;
    if (settings::deviceInitConfig.enableRaytracingSupport)
        rtFlags = static_cast<VulBuffer::Usage>(VulBuffer::usage_getAddress | VulBuffer::usage_accelerationStructureBuildRead);

    indexBuffer->createBuffer(true, static_cast<VulBuffer::Usage>(VulBuffer::usage_indexBuffer |
                VulBuffer::usage_transferDst | VulBuffer::usage_ssbo | rtFlags));
    vertexBuffer->createBuffer(true, static_cast<VulBuffer::Usage>(VulBuffer::usage_vertexBuffer |
                VulBuffer::usage_transferDst | VulBuffer::usage_ssbo | rtFlags));
    normalBuffer->createBuffer(true, static_cast<VulBuffer::Usage>(VulBuffer::usage_vertexBuffer | VulBuffer::usage_transferDst | VulBuffer::usage_ssbo));
    tangentBuffer->createBuffer(true, static_cast<VulBuffer::Usage>(VulBuffer::usage_vertexBuffer | VulBuffer::usage_transferDst | VulBuffer::usage_ssbo));
    uvBuffer->createBuffer(true, static_cast<VulBuffer::Usage>(VulBuffer::usage_vertexBuffer | VulBuffer::usage_transferDst | VulBuffer::usage_ssbo));
    materialBuffer->createBuffer(true, static_cast<VulBuffer::Usage>(VulBuffer::usage_ssbo | VulBuffer::usage_transferDst));
    primInfoBuffer->createBuffer(true, static_cast<VulBuffer::Usage>(VulBuffer::usage_ssbo | VulBuffer::usage_transferDst));
    lightsBuffer->createBuffer(true, static_cast<VulBuffer::Usage>(VulBuffer::usage_ssbo | VulBuffer::usage_transferDst));

    lights = gltfLoader.lights;
    nodes = gltfLoader.nodes;
    meshes = gltfLoader.primMeshes;
    materials = gltfLoader.materials;
    images = gltfLoader.images;

    VUL_NAME_VK(indexBuffer->getBuffer())
    VUL_NAME_VK(vertexBuffer->getBuffer())
    VUL_NAME_VK(normalBuffer->getBuffer())
    VUL_NAME_VK(uvBuffer->getBuffer())
    VUL_NAME_VK(materialBuffer->getBuffer())
    VUL_NAME_VK(primInfoBuffer->getBuffer())
    VUL_NAME_VK(lightsBuffer->getBuffer())
}

}
