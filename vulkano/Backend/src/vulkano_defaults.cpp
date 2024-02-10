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
        vulkano.usesDefaultDescriptorSets = true;
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
    
}
