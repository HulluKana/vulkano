#pragma once

#include"vul_camera.hpp"
#include"vul_device.hpp"
#include"vul_pipeline.hpp"
#include"vul_scene.hpp"
#include"vul_2d_object.hpp"

#include<memory>

namespace vul{

class RenderSystem{
    public:
        RenderSystem(vulB::VulDevice &device);
        ~RenderSystem();

        /* These 2 lines remove the copy constructor and operator from RenderSystem class.
        Because I'm using a pointer to stuff and that stuff is initialized by constructor and removed by destructor,
        copying the pointer and then removing the original pointer leaves me with pointer pointing to nothing, possibly leading to VERY nasty bugs */
        RenderSystem(const RenderSystem &) = delete;
        RenderSystem &operator=(const RenderSystem &) = delete;
        // Move instead of reference, you know the deal
        RenderSystem(RenderSystem &&) = default;
        RenderSystem &operator=(RenderSystem &&) = default;

        void init(std::vector<VkDescriptorSetLayout> setLayouts, std::string vertShaderName, std::string fragShaderName, 
                  VkFormat colorAttachmentFormat, VkFormat depthAttachmentFormat, bool is2D);

        void render(const Scene &scene, std::vector<VkDescriptorSet> &descriptorSets, VkCommandBuffer &commandBuffer);
        void render(Object2D &obj, std::vector<VkDescriptorSet> &descriptorSets, VkCommandBuffer &commandBuffer);
        void render(std::vector<Object2D> &objs, std::vector<VkDescriptorSet> &descriptorSets, VkCommandBuffer &commandBuffer);
    private:
        void createPipelineLayout(std::vector<VkDescriptorSetLayout> setLayouts);
        void createPipeline(std::string vertShaderName, std::string fragShaderName, VkFormat colorAttachmentFormat, VkFormat depthAttachmentFormat, bool is2D);
               
        vulB::VulDevice &vulDevice;
        std::unique_ptr<vulB::VulPipeline> vulPipeline;
        VkPipelineLayout pipelineLayout;       
};
}
