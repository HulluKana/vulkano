#pragma once

#include"vul_camera.hpp"
#include"vul_device.hpp"
#include"vul_pipeline.hpp"
#include"vul_object.hpp"

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
    private:

        void init(std::vector<VkDescriptorSetLayout> setLayouts, VkFormat colorAttachmentFormat, VkFormat depthAttachmentFormat);
        void createPipelineLayout(std::vector<VkDescriptorSetLayout> setLayouts);
        void createPipeline(VkFormat colorAttachmentFormat, VkFormat depthAttachmentFormat);
               
        void render(std::vector<VulObject> &objects, std::vector<VkDescriptorSet> &descriptorSets, VkCommandBuffer &commandBuffer);

        uint32_t index = 0;
        
        struct DefaultPushConstantInputData{
            glm::mat4 modelMatrix{1.0f};
            glm::mat4 normalMatrix{1.0f};
            glm::vec3 color{1.0f}; 
            int isLight = false;
            int lightIndex = 0;
            float specularExponent = 0.0f;
            int texIndex = -1;
        };

        vulB::VulDevice &vulDevice;
        std::unique_ptr<vulB::VulPipeline> vulPipeline;
        VkPipelineLayout pipelineLayout;
        
        std::string vertShaderName = "../Shaders/bin/default.vert.spv";
        std::string fragShaderName = "../Shaders/bin/default.frag.spv";

        friend class Vulkano;
};
}