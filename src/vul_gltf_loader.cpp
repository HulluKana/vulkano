#include "vul_command_pool.hpp"
#include "vul_device.hpp"
#include <GLFW/glfw3.h>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/ext/quaternion_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <ratio>
#include <set>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <iostream>
#include <future>

#include <thread>
#include <unordered_set>
#include<vul_gltf_loader.hpp>
#include<vul_transform.hpp>
#include <vulkan/vulkan_core.h>

#include <tiny_gltf.h>


namespace vul
{

GltfLoader::GltfLoader(std::string fileName)
{
    tinygltf::TinyGLTF context;
    std::string warn, err;
    
    if (!context.LoadASCIIFromFile(&m_model, &err, &warn, fileName)) 
        throw std::runtime_error("Failed to load scene from file: " + err);
}

void GltfLoader::importMaterials()
{
    materials.reserve(m_model.materials.size());

    for (const tinygltf::Material &tmat : m_model.materials){
        Material omat{};
        omat.name = tmat.name;
        if (tmat.alphaMode == "OPAQUE" || tmat.alphaMode.length() == 0) omat.alphaMode = GltfAlphaMode::opaque;
        else if (tmat.alphaMode == "MASK") omat.alphaMode = GltfAlphaMode::mask;
        else if (tmat.alphaMode == "BLEND") omat.alphaMode = GltfAlphaMode::blend;
        else throw std::runtime_error("Unsupported alpha mode while importing materials. Alpha mode: " + tmat.alphaMode);
        
        omat.emissiveFactor = (tmat.emissiveFactor.size() == 3) ? glm::vec3(tmat.emissiveFactor[0], tmat.emissiveFactor[1], tmat.emissiveFactor[2]) : glm::vec3(0.0f);
        
        tinygltf::PbrMetallicRoughness pbr = tmat.pbrMetallicRoughness;
        omat.colorFactor = glm::vec4(pbr.baseColorFactor[0], pbr.baseColorFactor[1], pbr.baseColorFactor[2], pbr.baseColorFactor[3]);
        omat.roughness = pbr.roughnessFactor;
        omat.metalliness = pbr.metallicFactor;
        omat.roughnessMetallinessTextureIndex = pbr.metallicRoughnessTexture.index;
        omat.colorTextureIndex = pbr.baseColorTexture.index;
        omat.normalTextureIndex = tmat.normalTexture.index;

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
        Material mat{};
        materials.emplace_back(mat);
    }
}

void GltfLoader::importFullTexturesSync(const std::string &textureDirectory, const VulDevice &device, VulCmdPool &cmdPool)
{
    if (m_model.images.size() == 0) return;
    importTextures(textureDirectory, 0, std::thread::hardware_concurrency(), device, cmdPool);
    for (size_t i = 0; i < images.size(); i++) images[i]->deleteCpuData();
}

void GltfLoader::importPartialTexturesAsync(AsyncImageLoadingInfo &asyncImageLoadingInfo, const std::string &textureDirectory, uint32_t maxMipCount, const VulDevice &device, VulCmdPool &transferPool, VulCmdPool &destinationPool)
{
    if (m_model.images.size() == 0) return;
    importTextures(textureDirectory, maxMipCount, std::thread::hardware_concurrency(), device, destinationPool);

    asyncImageLoadingInfo.oldVkImageStuff.reserve(images.size());
    std::atomic_bool stopUpdatingImages = false;
    std::function<void(std::vector<std::shared_ptr<vul::VulImage>>)> imgUpdaterFunc = [&device, &transferPool, &destinationPool, maxMipCount, &asyncImageLoadingInfo]
        (std::vector<std::shared_ptr<vul::VulImage>> images) {

        std::unordered_set<std::string> uniqueImagePaths;
        for (std::shared_ptr<vul::VulImage> &img : images) {
            if (uniqueImagePaths.find(img->name) != uniqueImagePaths.end()) img = nullptr;
            else uniqueImagePaths.insert(img->name);
        }

        std::atomic_uint32_t imgIdx = 0;
        std::vector<std::atomic_bool> imgHasFinished(images.size());
        std::function<void(std::stop_token)> loadData = [maxMipCount, &imgIdx, &images, &imgHasFinished](std::stop_token stoken) {
            while (!stoken.stop_requested()) {
                const uint32_t idx = imgIdx++;
                if (idx >= images.size()) break;
                const std::shared_ptr<vul::VulImage> &img = images[idx];
                if (img == nullptr) {
                    imgHasFinished[idx] = true;
                    continue;
                }
                vul::VulImage::KtxCompressionFormat fromat;
                if (img->getFormat() == VK_FORMAT_BC1_RGB_SRGB_BLOCK) fromat = vul::VulImage::KtxCompressionFormat::bc1rgbNonLinear;
                else if (img->getFormat() == VK_FORMAT_BC7_SRGB_BLOCK) fromat = vul::VulImage::KtxCompressionFormat::bc7rgbaNonLinear;
                else fromat = vul::VulImage::KtxCompressionFormat::bc7rgbaLinear;
                img->addMipLevelsToStartFromCompressedKtxFile(img->name, fromat, maxMipCount);
                imgHasFinished[idx] = true;
            }
        };
        std::vector<std::jthread> threads(std::max(std::jthread::hardware_concurrency() - 2, 1u));
        for (size_t i = 0; i < threads.size(); i++) threads[i] = std::jthread(loadData);

        uint32_t finishedImgCount = 0;
        while (!asyncImageLoadingInfo.stopLoadingImagesSignal && finishedImgCount < images.size()) {
            int idx = -1;
            for (size_t i = 0; i < imgHasFinished.size(); i++) if (imgHasFinished[i]) {
                imgHasFinished[i] = false;
                idx = i;
                break;
            }
            if (idx < 0) {
                using namespace std::chrono_literals;
                std::this_thread::sleep_for(10ms);
                continue;
            }
            std::shared_ptr<vul::VulImage> &img = images[idx];
            if (img != nullptr) {
                std::scoped_lock lock(asyncImageLoadingInfo.pauseMutex);
                VkCommandBuffer commandBuffer = transferPool.getPrimaryCommandBuffer();
                asyncImageLoadingInfo.oldVkImageStuff.push_back(img->createCustomImage(VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                            VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, commandBuffer));
                img->transitionQueueFamily(device.getQueueFamilies().transferFamily, device.getQueueFamilies().mainFamily, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, commandBuffer);
                VkSemaphore semaphore = transferPool.submitAndSynchronize(commandBuffer, VK_NULL_HANDLE, true, false);
                commandBuffer = destinationPool.getPrimaryCommandBuffer();
                img->transitionQueueFamily(device.getQueueFamilies().transferFamily, device.getQueueFamilies().mainFamily, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, commandBuffer);
                img->transitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, commandBuffer);
                destinationPool.submitAndSynchronize(commandBuffer, semaphore, false, true);
            }
            asyncImageLoadingInfo.fullyProcessedImageCount++;
            finishedImgCount++;
        }
    };

    asyncImageLoadingInfo.pauseMutex.unlock();
    asyncImageLoadingInfo.asyncLoadingThread = std::thread(imgUpdaterFunc, images);
}

void GltfLoader::importTextures(std::string textureDirectory, uint32_t mipCount, uint32_t threadCount, const VulDevice &device, VulCmdPool &cmdPool)
{
    if (textureDirectory[textureDirectory.length() - 1] != '/') textureDirectory += '/';

    std::set<int> transparentColorTextures;
    std::set<int> opaqueColorTextures;
    std::set<int> normalMaps;
    std::set<int> roughnessMetallicTextures;
    for (const tinygltf::Material &mat : m_model.materials) {
        if (mat.alphaMode == "OPAQUE") opaqueColorTextures.insert(m_model.textures[mat.pbrMetallicRoughness.baseColorTexture.index].source);
        else transparentColorTextures.insert(m_model.textures[mat.pbrMetallicRoughness.baseColorTexture.index].source);
        normalMaps.insert(m_model.textures[mat.normalTexture.index].source);
        roughnessMetallicTextures.insert(m_model.textures[mat.pbrMetallicRoughness.metallicRoughnessTexture.index].source);
    }

    std::vector<std::thread> threads(threadCount);
    std::vector<std::unique_ptr<VulCmdPool>> cmdPools(threads.size());
    std::vector<VkCommandBuffer> cmdBufs(threads.size());
    for (size_t i = 0; i < threads.size(); i++) {
        cmdPools[i] = std::make_unique<VulCmdPool>(vul::VulCmdPool::QueueType::main, 0, 0, device);
        cmdBufs[i] = cmdPools[i]->getSecondaryCommandBuffer();
    }

    std::vector<std::shared_ptr<VulImage>> imgSources(m_model.images.size());
    std::atomic<uint32_t> atomImgIdx = 0;
    std::function<void(uint32_t)> importTexture = [&](uint32_t threadIdx)
    {
        while (true) {
            uint32_t imgIdx = atomImgIdx++; 
            if (imgIdx >= imgSources.size()) break;

            VulImage::KtxCompressionFormat fromat{};
            if (transparentColorTextures.count(imgIdx) > 0) fromat = VulImage::KtxCompressionFormat::bc7rgbaNonLinear;
            else if (opaqueColorTextures.count(imgIdx) > 0) fromat = VulImage::KtxCompressionFormat::bc1rgbNonLinear;
            else if (normalMaps.count(imgIdx) > 0) fromat = VulImage::KtxCompressionFormat::bc7rgbaLinear;
            else if (roughnessMetallicTextures.count(imgIdx) > 0) fromat = VulImage::KtxCompressionFormat::bc7rgbaLinear;

            const tinygltf::Image &image = m_model.images[imgIdx];
            imgSources[imgIdx] = std::make_shared<VulImage>(device);
            imgSources[imgIdx]->name = textureDirectory + image.uri;
            imgSources[imgIdx]->loadCompressedKtxFromFile(textureDirectory + image.uri, fromat, mipCount, 69);
            imgSources[imgIdx]->createCustomImage(VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, cmdBufs[threadIdx]);
        }
    };

    for (size_t i = 0; i < threads.size(); i++) threads[i] = std::thread(importTexture, i);
    for (size_t i = 0; i < threads.size(); i++) {
        threads[i].join();
        cmdPools[i]->endCommandBuffer(cmdBufs[i]);
    }

    VkCommandBuffer cmdBuf = cmdPool.getPrimaryCommandBuffer();
    vkCmdExecuteCommands(cmdBuf, cmdBufs.size(), cmdBufs.data());
    cmdPool.submit(cmdBuf, true);

    images.resize(m_model.textures.size());
    std::shared_ptr<VulSampler> sampler = VulSampler::createDefaultTexSampler(device);
    for (size_t i = 0; i < images.size(); i++) {
        images[i] = imgSources[m_model.textures[i].source];
        images[i]->vulSampler = sampler;
        images[i]->deleteStagingResources();
    }
}

void GltfLoader::importDrawableNodes(GltfAttributes requestedAttributes)
{
    const int defaultScene = m_model.defaultScene > -1 ? m_model.defaultScene : 0;    
    const auto &scene = m_model.scenes[defaultScene];

    std::set<uint32_t> usedMeshes;
    std::function<void(int)> findUsedMeshes = [&] (int nodeIdx)
    {
        const tinygltf::Node node = m_model.nodes[nodeIdx];
        if (node.mesh >= 0) usedMeshes.insert(node.mesh);
        for (int childIdx : node.children) findUsedMeshes(childIdx);
    };
    for (int nodeIdx : scene.nodes){
        findUsedMeshes(nodeIdx);
    }

    uint32_t indexCnt = 0;
    uint32_t primCnt = 0;
    for (uint32_t meshIdx : usedMeshes){
        tinygltf::Mesh mesh = m_model.meshes[meshIdx]; 
        std::vector<uint32_t> primitives;
        for (tinygltf::Primitive prim: mesh.primitives){
            if (prim.mode != 4) throw std::runtime_error("Why is the primitive mode not 4? Find an answer to that."); // I think the mode 4 is triangle
            if (prim.indices > -1){
                tinygltf::Accessor indexAccessor = m_model.accessors[prim.indices];
                indexCnt += static_cast<uint32_t>(indexAccessor.count);
            } else{
                throw std::runtime_error("Is this okay? I dunno, figure it out");
                tinygltf::Accessor posAccessor = m_model.accessors[prim.attributes.find("POSITION")->second];
                indexCnt += static_cast<uint32_t>(posAccessor.count); 
            }
            primitives.push_back(primCnt);
            primCnt++;
        }
        m_meshToPrimMesh[meshIdx] = std::move(primitives);
    }
    indices.reserve(indexCnt);
    for (uint32_t meshIdx : usedMeshes){
        tinygltf::Mesh mesh = m_model.meshes[meshIdx];
        for (tinygltf::Primitive prim : mesh.primitives){
            processMesh(prim, requestedAttributes, mesh.name);
        }
    }

    for (int nodeIdx : scene.nodes){
        glm::quat quat{};
        quat.x = 0.0f;
        quat.y = 0.0f;
        quat.z = 0.0f;
        quat.w = 1.0f;
        processNode(nodeIdx, {0.0f, 0.0f, 0.0f}, quat, {1.0f, 1.0f, 1.0f});
    }

    m_meshToPrimMesh.clear();
    m_cachePrimMesh.clear();
}

void GltfLoader::processMesh(const tinygltf::Primitive &mesh, GltfAttributes requestedAttributes, const std::string &name)
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
        const tinygltf::Accessor &indexAccessor = m_model.accessors[mesh.indices];
        resultMesh.indexCount = static_cast<uint32_t>(indexAccessor.count);

        switch (indexAccessor.componentType) {
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:{
                std::vector<uint32_t> primitiveIndices(indexAccessor.count);
                copyAccessorData(primitiveIndices, 0, indexAccessor, 0, indexAccessor.count);
                indices.insert(indices.end(), primitiveIndices.begin(), primitiveIndices.end());
                break;
            }
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:{
                std::vector<uint16_t> primitiveIndices(indexAccessor.count);
                copyAccessorData(primitiveIndices, 0, indexAccessor, 0, indexAccessor.count);
                indices.insert(indices.end(), primitiveIndices.begin(), primitiveIndices.end());
                break;
            }
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:{
                std::vector<uint8_t> primitiveIndices(indexAccessor.count);
                copyAccessorData(primitiveIndices, 0, indexAccessor, 0, indexAccessor.count);
                indices.insert(indices.end(), primitiveIndices.begin(), primitiveIndices.end());
                break;
            }
            default:
                throw std::runtime_error(std::string("Index component not supported. It's type is ") + std::to_string(indexAccessor.componentType));
        }
    }
    else{
        const tinygltf::Accessor accessor = m_model.accessors[mesh.attributes.find("POSITION")->second];
        for (size_t i = 0; i < accessor.count; i++){
            indices.push_back(i);
        }
        resultMesh.indexCount = static_cast<uint32_t>(accessor.count);
    }

    if (!primMeshCached){
        const bool hadPosition = getAttribute<glm::vec3>(mesh, positions, "POSITION");
        if (!hadPosition) throw std::runtime_error("The mesh doesnt have position");

        const tinygltf::Accessor accessor = m_model.accessors[mesh.attributes.find("POSITION")->second];
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
            bool normalCreated = getAttribute<glm::vec3>(mesh, normals, "NORMAL");
            if (!normalCreated) throw std::runtime_error("The mesh doesnt have normals");
        }
        if (gltfAttribAnd(requestedAttributes, GltfAttributes::Tangent) == GltfAttributes::Tangent) {
            if (!getAttribute<glm::vec4>(mesh, tangents, "TANGENT")) {
                std::cout << "The mesh doesnt have tangents. Name: " << name << "\n";
                createTangents(resultMesh.vertexCount);
            }
        }
        if (gltfAttribAnd(requestedAttributes, GltfAttributes::TexCoord) == GltfAttributes::TexCoord){
            bool texCoordCreated = getAttribute<glm::vec2>(mesh, uvCoords, "TEXCOORD_0");
            if (!texCoordCreated)
                texCoordCreated = getAttribute<glm::vec2>(mesh, uvCoords, "TEXCOORD");
            if (!texCoordCreated) throw std::runtime_error("The mesh doesnt have tex coords");
        }
        if (gltfAttribAnd(requestedAttributes, GltfAttributes::Color) == GltfAttributes::Color){
            bool colorCreated = getAttribute<glm::vec4>(mesh, colors, "COLOR_0");
            if (!colorCreated) throw std::runtime_error("The mesh doesnt have colors");
        }
    }

    m_cachePrimMesh[key.str()] = resultMesh;
    primMeshes.emplace_back(resultMesh);
}

void GltfLoader::processNode(int nodeIdx, const glm::vec3 &parentPos, const glm::quat &parentRot, const glm::vec3 &parentScale)
{
    const tinygltf::Node &node = m_model.nodes[nodeIdx];

    transform3D transform{};
    transform.pos = parentPos;
    transform.scale = parentScale;
    glm::quat rotationQuaternion = parentRot;

    if (!node.translation.empty())
        transform.pos += glm::vec3(node.translation[0], node.translation[1], node.translation[2]);
    if (!node.rotation.empty()) {
        glm::quat localQuat{};
        localQuat.x = node.rotation[0];
        localQuat.y = node.rotation[1];
        localQuat.z = node.rotation[2];
        localQuat.w = node.rotation[3];
        rotationQuaternion *= localQuat;
    }
    transform.rot = -glm::eulerAngles(rotationQuaternion); // Don't even ask about the minus sign
    if (!node.scale.empty())
        transform.scale *= glm::vec3(node.scale[0], node.scale[1], node.scale[2]); 
    if (!node.matrix.empty()) throw std::runtime_error("The node has some sort of matrix. Handle it");

    if (node.mesh > -1){
        const std::vector<uint32_t> &meshes = m_meshToPrimMesh[node.mesh];
        for (uint32_t mesh : meshes){
            GltfNode dunno;
            dunno.name = node.name;
            dunno.primMesh = mesh;
            dunno.worldMatrix = transform.transformMat();
            dunno.normalMatrix = transform.normalMat();
            dunno.position = transform.pos;
            nodes.push_back(dunno);
        }
    }
    else if (node.camera > -1) throw std::runtime_error("The node is a camera. Do something about it");
    else if (node.extensions.find("KHR_lights_punctual") != node.extensions.end()){
        const tinygltf::Light &light = m_model.lights[node.light];
        GltfLight gltfLight{};
        gltfLight.name = light.name;
        gltfLight.position = transform.pos;
        gltfLight.color = {light.color[0], light.color[1], light.color[2]};
        gltfLight.intensity = light.intensity;
        gltfLight.range = light.range;
        if (gltfLight.range < 0.01f) gltfLight.range = 10'000.0f;
        lights.push_back(gltfLight);
    }

    for (int child : node.children){
        processNode(child, transform.pos, rotationQuaternion, transform.scale);
    }
}

void GltfLoader::createTangents(size_t amount)
{
    tangents.reserve(amount);
    for (size_t i = 0; i < amount; i++) {
        tangents.push_back({});     
    }
}

float GltfLoader::getFloat(const tinygltf::Value &value, const std::string &name)
{
    if (value.Has(name)){
        return static_cast<float>(value.Get(name).Get<double>());
    }
    throw std::runtime_error("Couldn't get float from tinygltf. This probably should be handled, but I haven't figured out that yet");
}

template<class T>
void GltfLoader::copyAccessorData(  std::vector<T> &outData, size_t outFirstElement, 
                        const tinygltf::Accessor &accessor, size_t accessorFirstElement, size_t numElementsToCopy)
{
    if (outFirstElement >= outData.size()) throw std::runtime_error("Invalid outFirstElement");
    if (accessorFirstElement >= accessor.count) throw std::runtime_error("Invalid accessorFirstElement");

    const tinygltf::BufferView &bufferView = m_model.bufferViews[accessor.bufferView];
    const uint8_t *buffer = &m_model.buffers[bufferView.buffer].data[accessor.byteOffset + bufferView.byteOffset];

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
bool GltfLoader::getAccessorData(const tinygltf::Accessor &accessor, std::vector<T> &attribVec)
{
    const size_t elemCnt = accessor.count;
    const size_t oldElemCnt = attribVec.size();
    attribVec.resize(oldElemCnt + elemCnt);
    
    if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT){
        copyAccessorData<T>(attribVec, oldElemCnt, accessor, 0, accessor.count);
    }
    else{
        throw std::runtime_error(std::string("Yo, the accessor type isnt float. Do something about it. Its type btw is ") + std::to_string(accessor.componentType));
    }

    return true;
}
template <typename T>
bool GltfLoader::getAttribute(const tinygltf::Primitive &primitive, std::vector<T> &attribVec, const std::string &attribName)
{
    const auto &it = primitive.attributes.find(attribName);
    if (it == primitive.attributes.end()) return false;
    const tinygltf::Accessor &accessor = m_model.accessors[it->second];
    return getAccessorData(accessor, attribVec);
}

}
