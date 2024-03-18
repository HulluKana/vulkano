#include <cstddef>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/ext/quaternion_transform.hpp>
#include <limits>
#include <memory>
#include <set>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <iostream>

#include"../Headers/vul_gltf_loader.hpp"
#include"../Headers/vul_transform.hpp"

using namespace vul;
namespace vulB
{

void GltfLoader::importMaterials(const tinygltf::Model &model)
{
    materials.reserve(model.materials.size());

    for (const tinygltf::Material &tmat : model.materials){
        Material omat;
        
        omat.emissiveFactor = (tmat.emissiveFactor.size() == 3) ? glm::vec3(tmat.emissiveFactor[0], tmat.emissiveFactor[1], tmat.emissiveFactor[2]) : glm::vec3(0.0f);
        
        tinygltf::PbrMetallicRoughness pbr = tmat.pbrMetallicRoughness;
        omat.colorFactor = glm::vec4(pbr.baseColorFactor[0], pbr.baseColorFactor[1], pbr.baseColorFactor[2], pbr.baseColorFactor[3]);
        omat.colorTextureIndex = pbr.baseColorTexture.index;
        omat.roughness = pbr.roughnessFactor;
        omat.metalliness = pbr.metallicFactor;

        if (tmat.extensions.find("KHR_materials_ior") != tmat.extensions.end()){
            const auto &ext = tmat.extensions.find("KHR_materials_ior")->second;
            omat.ior = getFloat(ext, "ior");
        }
        if (tmat.extensions.find("KHR_materials_emissive_strength") != tmat.extensions.end()){
            const auto &ext = tmat.extensions.find("KHR_materials_emissive_strength")->second;
            omat.emissionStrength = getFloat(ext, "emissiveStrength");
        }
        
        materials.emplace_back(omat);
    }

    if (materials.empty()){
        Material mat;
        materials.emplace_back(mat);
    }
}

void GltfLoader::importTextures(const tinygltf::Model &model, VulDevice &device)
{
    for (const tinygltf::Image &image : model.images){
        std::shared_ptr<VulImage> vulImage = std::make_shared<VulImage>(device);
        vulImage->loadData(image.image.data(), image.width, image.height, image.image.size() / image.width / image.height);
        vulImage->createImage(true, true, false, 2);
        images.push_back(vulImage);
    }
}

void GltfLoader::importDrawableNodes(const tinygltf::Model &model, GltfAttributes requestedAttributes)
{
    const int defaultScene = model.defaultScene > -1 ? model.defaultScene : 0;    
    const auto &scene = model.scenes[defaultScene];

    std::set<uint32_t> usedMeshes;
    std::function<void(int)> findUsedMeshes = [&] (int nodeIdx)
    {
        const tinygltf::Node node = model.nodes[nodeIdx];
        if (node.mesh >= 0) usedMeshes.insert(node.mesh);
        for (int childIdx : node.children) findUsedMeshes(childIdx);
    };
    for (int nodeIdx : scene.nodes){
        findUsedMeshes(nodeIdx);
    }

    uint32_t indexCnt = 0;
    uint32_t primCnt = 0;
    for (uint32_t meshIdx : usedMeshes){
        tinygltf::Mesh mesh = model.meshes[meshIdx]; 
        std::vector<uint32_t> primitives;
        for (tinygltf::Primitive prim: mesh.primitives){
            if (prim.mode != 4) throw std::runtime_error("Why is the primitive mode not 4? Find an answer to that."); // I think the mode 4 is triangle
            if (prim.indices > -1){
                tinygltf::Accessor indexAccessor = model.accessors[prim.indices];
                indexCnt += static_cast<uint32_t>(indexAccessor.count);
            } else{
                throw std::runtime_error("Is this okay? I dunno, figure it out");
                tinygltf::Accessor posAccessor = model.accessors[prim.attributes.find("POSITION")->second];
                indexCnt += static_cast<uint32_t>(posAccessor.count); 
            }
            primitives.push_back(primCnt);
            primCnt++;
        }
        m_meshToPrimMesh[meshIdx] = std::move(primitives);
    }
    indices.reserve(indexCnt);
    for (uint32_t meshIdx : usedMeshes){
        tinygltf::Mesh mesh = model.meshes[meshIdx];
        for (tinygltf::Primitive prim : mesh.primitives){
            processMesh(model, prim, requestedAttributes, mesh.name);
        }
    }

    for (int nodeIdx : scene.nodes){
        glm::mat4 matrix = glm::mat4(1.0f);
        processNode(model, nodeIdx, matrix, {0.0f, 0.0f, 0.0f});
    }

    m_meshToPrimMesh.clear();
    m_cachePrimMesh.clear();
}

void GltfLoader::processMesh(const tinygltf::Model &model, const tinygltf::Primitive &mesh, GltfAttributes requestedAttributes, const std::string &name)
{
    if (mesh.mode != 4) return; 

    GltfPrimMesh resultMesh;
    resultMesh.name = name;
    resultMesh.materialIndex = std::max(0, mesh.material);
    resultMesh.vertexOffset = static_cast<uint32_t>(positions.size());
    resultMesh.firstIndex = static_cast<uint32_t>(indices.size());

    // By caching meshes, I can skip their position loading step, but I can still give them unique materials and indices
    std::stringstream key;
    for (const auto &attrib : mesh.attributes){
        key << attrib.first << attrib.second;
    }
    bool primMeshCached = false;
    auto cacheIt = m_cachePrimMesh.find(key.str());
    if (cacheIt != m_cachePrimMesh.end()){
        primMeshCached = true;
        GltfPrimMesh cacheMesh = cacheIt->second;
        resultMesh.vertexCount = cacheMesh.vertexCount;
        resultMesh.vertexOffset = cacheMesh.vertexOffset;
    }

    if (mesh.indices > -1){
        const tinygltf::Accessor &indexAccessor = model.accessors[mesh.indices];
        resultMesh.indexCount = static_cast<uint32_t>(indexAccessor.count);

        switch (indexAccessor.componentType) {
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:{
                std::vector<uint32_t> primitiveIndices(indexAccessor.count);
                copyAccessorData(primitiveIndices, 0, model, indexAccessor, 0, indexAccessor.count);
                indices.insert(indices.end(), primitiveIndices.begin(), primitiveIndices.end());
                break;
            }
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:{
                std::vector<uint16_t> primitiveIndices(indexAccessor.count);
                copyAccessorData(primitiveIndices, 0, model, indexAccessor, 0, indexAccessor.count);
                indices.insert(indices.end(), primitiveIndices.begin(), primitiveIndices.end());
                break;
            }
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:{
                std::vector<uint8_t> primitiveIndices(indexAccessor.count);
                copyAccessorData(primitiveIndices, 0, model, indexAccessor, 0, indexAccessor.count);
                indices.insert(indices.end(), primitiveIndices.begin(), primitiveIndices.end());
                break;
            }
            default:
                throw std::runtime_error(std::string("Index component not supported. It's type is ") + std::to_string(indexAccessor.componentType));
        }
    }
    else{
        const tinygltf::Accessor accessor = model.accessors[mesh.attributes.find("POSITION")->second];
        for (size_t i = 0; i < accessor.count; i++){
            indices.push_back(i);
        }
        resultMesh.indexCount = static_cast<uint32_t>(accessor.count);
    }

    if (!primMeshCached){
        const bool hadPosition = getAttribute<glm::vec3>(model, mesh, positions, "POSITION");
        if (!hadPosition) throw std::runtime_error("The mesh doesnt have position");

        const tinygltf::Accessor accessor = model.accessors[mesh.attributes.find("POSITION")->second];
        resultMesh.vertexCount = static_cast<uint32_t>(accessor.count);
        if (!accessor.minValues.empty()){
            resultMesh.posMin = glm::vec3(accessor.minValues[0], accessor.minValues[1], accessor.minValues[2]);
        } else{
            resultMesh.posMin = glm::vec3(std::numeric_limits<float>::max());
            for (const glm::vec3 &pos : positions){
                resultMesh.posMin = glm::min(resultMesh.posMin, pos);
            }
        }
        if (!accessor.maxValues.empty()){
            resultMesh.posMax = glm::vec3(accessor.maxValues[0], accessor.maxValues[1], accessor.maxValues[2]);
        } else{
            resultMesh.posMax = glm::vec3(-std::numeric_limits<float>::max());
            for (const glm::vec3 &pos : positions){
                resultMesh.posMax = glm::max(resultMesh.posMax, pos);
            }
        }

        if (gltfAttribAnd(requestedAttributes, GltfAttributes::Normal) == GltfAttributes::Normal){
            bool normalCreated = getAttribute<glm::vec3>(model, mesh, normals, "NORMAL");
            if (!normalCreated) throw std::runtime_error("The mesh doesnt have normals");
        }
        if (gltfAttribAnd(requestedAttributes, GltfAttributes::TexCoord) == GltfAttributes::TexCoord){
            bool texCoordCreated = getAttribute<glm::vec2>(model, mesh, uvCoords, "TEXCOORD_0");
            if (!texCoordCreated)
                texCoordCreated = getAttribute<glm::vec2>(model, mesh, uvCoords, "TEXCOORD");
            if (!texCoordCreated) throw std::runtime_error("The mesh doesnt have tex coords");
        }
        if (gltfAttribAnd(requestedAttributes, GltfAttributes::Color) == GltfAttributes::Color){
            bool colorCreated = getAttribute<glm::vec4>(model, mesh, colors, "COLOR_0");
            if (!colorCreated) throw std::runtime_error("The mesh doesnt have colors");
        }
    }

    m_cachePrimMesh[key.str()] = resultMesh;
    primMeshes.emplace_back(resultMesh);
}

void GltfLoader::processNode(const tinygltf::Model &model, int nodeIdx, const glm::mat4 &parentMatrix, const glm::vec3 &parentPos)
{
    const tinygltf::Node &node = model.nodes[nodeIdx];

    transform3D transform;
    if (!node.translation.empty()){
        transform.pos = glm::vec3(node.translation[0], node.translation[1], node.translation[2]);
    }
    if (!node.rotation.empty()){
        glm::quat rotationQuaternion = glm::quat(node.rotation[0], node.rotation[1], node.rotation[2], node.rotation[3]);
        transform.rot = glm::eulerAngles(rotationQuaternion);
    }
    if (!node.scale.empty())
        transform.scale = glm::vec3(node.scale[0], node.scale[1], node.scale[2]);
    
    if (!node.matrix.empty()) throw std::runtime_error("The node has some sort of matrix. Handle it");

    glm::mat4 worldMatrix = parentMatrix * transform.transformMat();
    transform.pos += parentPos;

    if (node.mesh > -1){
        const std::vector<uint32_t> &meshes = m_meshToPrimMesh[node.mesh];
        for (uint32_t mesh : meshes){
            GltfNode dunno;
            dunno.primMesh = mesh;
            dunno.worldMatrix = worldMatrix;
            dunno.position = transform.pos;
            nodes.push_back(dunno);
        }
    }
    else if (node.camera > -1) throw std::runtime_error("The node is a camera. Do something about it");
    else if (node.extensions.find("KHR_lights_punctual") != node.extensions.end()){
        const tinygltf::Light &light = model.lights[node.light];
        GltfLight gltfLight{};
        gltfLight.position = {transform.pos.x, transform.pos.y, transform.pos.z};
        gltfLight.color = {light.color[0], light.color[1], light.color[2]};
        gltfLight.intensity = light.intensity;
        lights.push_back(gltfLight);
    }

    for (int child : node.children){
        processNode(model, child, worldMatrix, transform.pos);
    }
}

float GltfLoader::getFloat(const tinygltf::Value &value, const std::string &name)
{
    if (value.Has(name)){
        return static_cast<float>(value.Get(name).Get<double>());
    }
    throw std::runtime_error("Couldn't get float from tinygltf in scene.cpp. This probably should be handled, but I haven't figured out that yet");
}

template<class T>
void GltfLoader::copyAccessorData(  std::vector<T> &outData, size_t outFirstElement, const tinygltf::Model &model, 
                        const tinygltf::Accessor &accessor, size_t accessorFirstElement, size_t numElementsToCopy)
{
    if (outFirstElement >= outData.size()) throw std::runtime_error("Invalid outFirstElement");
    if (accessorFirstElement >= accessor.count) throw std::runtime_error("Invalid accessorFirstElement");

    const tinygltf::BufferView &bufferView = model.bufferViews[accessor.bufferView];
    const uint8_t *buffer = &model.buffers[bufferView.buffer].data[accessor.byteOffset + bufferView.byteOffset];

    const size_t maxSafeCopySize = std::min(accessor.count - accessorFirstElement, outData.size() - outFirstElement);
    numElementsToCopy = std::min(numElementsToCopy, maxSafeCopySize);

    if (bufferView.byteStride == 0) 
        memcpy(outData.data() + outFirstElement, reinterpret_cast<const T *>(buffer) + accessorFirstElement, numElementsToCopy * sizeof(T));
    else{
        for (size_t i = 0; i < numElementsToCopy; i++){
            outData.data()[outFirstElement + i] = *reinterpret_cast<const T*>(buffer + bufferView.byteStride * i);
        }
    }

    if (accessor.sparse.isSparse) throw std::runtime_error("It seems the accessor is sparse. Figure out how to handle it");
};

template<class T>
bool GltfLoader::getAccessorData(const tinygltf::Model &model, const tinygltf::Accessor &accessor, std::vector<T> &attribVec)
{
    const size_t elemCnt = accessor.count;
    const size_t oldElemCnt = attribVec.size();
    attribVec.resize(oldElemCnt + elemCnt);
    
    if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT){
        copyAccessorData<T>(attribVec, oldElemCnt, model, accessor, 0, accessor.count);
    }
    else{
        throw std::runtime_error(std::string("Yo, the accessor type isnt float. Do something about it. Its type btw is ") + std::to_string(accessor.componentType));
    }

    return true;
}
template <typename T>
bool GltfLoader::getAttribute(const tinygltf::Model &model, const tinygltf::Primitive &primitive, std::vector<T> &attribVec, const std::string &attribName)
{
    const auto &it = primitive.attributes.find(attribName);
    if (it == primitive.attributes.end()) return false;
    const tinygltf::Accessor &accessor = model.accessors[it->second];
    return getAccessorData(model, accessor, attribVec);
}

}
