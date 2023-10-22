#pragma once

#include"vul_camera.hpp"
#include"vul_device.hpp"
#include"vul_pipeline.hpp"
#include"vul_object.hpp"

#include<memory>

namespace vulB{

class SimpleRenderSystem{
    public:
        SimpleRenderSystem(VulDevice &device);
        ~SimpleRenderSystem();

        void init(VkRenderPass renderPass, std::vector<VkDescriptorSetLayout> setLayouts, const std::string &shadersFolder, VkFormat colorAttachmentFormat, VkFormat depthAttachmentFormat);

        /* These 2 lines remove the copy constructor and operator from SimpleRenderSystem class.
        Because I'm using a pointer to stuff and that stuff is initialized by constructor and removed by destructor,
        copying the pointer and then removing the original pointer leaves me with pointer pointing to nothing, possibly leading to VERY nasty bugs */
        SimpleRenderSystem(const SimpleRenderSystem &) = delete;
        SimpleRenderSystem &operator=(const SimpleRenderSystem &) = delete;
        
        void renderObjects(std::vector<vul::VulObject> &objects, std::vector<VkDescriptorSet> &descriptorSets, VkCommandBuffer &commandBuffer, int maxLights);
    private:
        void createPipelineLayout(std::vector<VkDescriptorSetLayout> setLayouts);
        void createPipeline(VkRenderPass renderPass, const std::string &shadersFolder, VkFormat colorAttachmentFormat, VkFormat depthAttachmentFormat);

        VulDevice &vulDevice;
        std::unique_ptr<VulPipeline> vulPipeline;
        VkPipelineLayout pipelineLayout;
};
}