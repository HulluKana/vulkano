#include"../../vulkano_defaults.hpp"

#include <cstring>
#include <memory>
#include<stdexcept>

using namespace vulB;
namespace vul
{

void defaults::createDefaultDescriptors(Vulkano &vulkano)
{
    for (int i = 0; i < VulSwapChain::MAX_FRAMES_IN_FLIGHT; i++){
        std::unique_ptr<VulBuffer> globalBuffer;
        globalBuffer = std::make_unique<VulBuffer>  (vulkano.getVulDevice(), sizeof(GlobalUbo), 1, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
                                                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 0, vulkano.getVulDevice().properties.limits.minUniformBufferOffsetAlignment);
        globalBuffer->map();
        vulkano.buffers.push_back(std::move(globalBuffer));
    }

    for (int i = 0; i < VulSwapChain::MAX_FRAMES_IN_FLIGHT; i++){
        std::vector<Vulkano::Descriptor> descs;
        Vulkano::Descriptor ubo{}; 
        ubo.type = Vulkano::DescriptorType::ubo;
        ubo.content = vulkano.buffers[i].get();
        ubo.stages = {Vulkano::ShaderStage::vert, Vulkano::ShaderStage::frag};
        descs.push_back(ubo);

        Vulkano::Descriptor image{};
        image.type = Vulkano::DescriptorType::spCombinedTexSampler;
        image.content = &vulkano.images[0];
        image.count = MAX_TEXTURES;
        image.stages = {Vulkano::ShaderStage::frag};
        descs.push_back(image);

        if (vulkano.hasScene){
            Vulkano::Descriptor matBuf{};
            matBuf.type = Vulkano::DescriptorType::ssbo;
            matBuf.content = vulkano.scene.materialBuffer.get();
            matBuf.stages = {Vulkano::ShaderStage::frag};
            descs.push_back(matBuf);
        }
        Vulkano::descSetReturnVal retVal = vulkano.createDescriptorSet(descs);
        if (!retVal.succeeded) throw std::runtime_error("Failed to create default descriptor sets");
        vulkano.mainDescriptorSets.push_back(std::move(retVal.set));
        vulkano.mainSetLayout = std::move(retVal.layout);
    }
}

void defaults::createDefault3dRenderSystem(Vulkano &vulkano)
{
    vulkano.renderSystem3D = vulkano.createNewRenderSystem({vulkano.mainSetLayout->getDescriptorSetLayout()}, "default3D.vert.spv", "default3D.frag.spv", false);
}

void defaults::createDefault2dRenderSystem(Vulkano &vulkano)
{
    vulkano.renderSystem2D = vulkano.createNewRenderSystem({vulkano.mainSetLayout->getDescriptorSetLayout()}, "default2D.vert.spv", "default2D.frag.spv", true);
}

void defaults::updateDefault3dInputValues(Vulkano &vulkano)
{
    Scene &scene = vulkano.scene;

    GlobalUbo ubo{};
    ubo.projectionMatrix = vulkano.camera.getProjection();
    ubo.viewMatrix = vulkano.camera.getView();
    ubo.cameraPosition = glm::vec4(vulkano.cameraTransform.pos, 0.0f);

    ubo.ambientLightColor = glm::vec4(1.0f, 1.0f, 1.0f, 0.02f);
    ubo.numLights = scene.lights.size();
    for (int i = 0; i < std::min(static_cast<int>(scene.lights.size()), MAX_LIGHTS); i++){
        ubo.lightPositions[i] = glm::vec4(scene.lights[i].position, 69.0f);
        ubo.lightColors[i] = glm::vec4(scene.lights[i].color, scene.lights[i].intensity);
    }

    vulkano.buffers[vulkano.getFrameIdx()]->writeToBuffer(&ubo, sizeof(ubo));

    struct DefaultPushConstantInputData{
        glm::mat4 modelMatrix;
        glm::mat4 normalMatrix;
        int matIdx;
    };
    
    scene.pushDataSize = sizeof(DefaultPushConstantInputData);
    for (size_t i = 0; i < scene.nodes.size(); i++){
        const GltfLoader::GltfNode &node = scene.nodes[i];
        const GltfLoader::GltfPrimMesh &mesh = scene.meshes[node.primMesh];
        std::vector<std::shared_ptr<void>> &pPushDatas = scene.pPushDatas;
        if (i >= pPushDatas.size() || pPushDatas[i] == nullptr) pPushDatas.push_back(std::shared_ptr<void>(new DefaultPushConstantInputData));

        DefaultPushConstantInputData *pushData = static_cast<DefaultPushConstantInputData *>(pPushDatas[i].get());
        pushData->modelMatrix = node.worldMatrix;
        pushData->normalMatrix = glm::mat4(1.0f);
        pushData->matIdx = mesh.materialIndex;
    }
}
    
}
