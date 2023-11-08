#pragma once

#include"vul_camera.hpp"
#include"vul_device.hpp"
#include"vul_pipeline.hpp"
#include"vul_object.hpp"

#include<memory>

namespace vulB{

class RenderSystem{
    public:
        RenderSystem(VulDevice &device);
        ~RenderSystem();

        void init(std::vector<VkDescriptorSetLayout> setLayouts, VkFormat colorAttachmentFormat, VkFormat depthAttachmentFormat);

        /* These 2 lines remove the copy constructor and operator from RenderSystem class.
        Because I'm using a pointer to stuff and that stuff is initialized by constructor and removed by destructor,
        copying the pointer and then removing the original pointer leaves me with pointer pointing to nothing, possibly leading to VERY nasty bugs */
        RenderSystem(const RenderSystem &) = delete;
        RenderSystem &operator=(const RenderSystem &) = delete;
        
        void renderObjects(std::vector<vul::VulObject> &objects, std::vector<VkDescriptorSet> &descriptorSets, VkCommandBuffer &commandBuffer, int maxLights);
    private:
        void createPipelineLayout(std::vector<VkDescriptorSetLayout> setLayouts);
        void createPipeline(VkFormat colorAttachmentFormat, VkFormat depthAttachmentFormat);

        struct DefaultPushConstantInputData{
            glm::mat4 modelMatrix{1.0f};
            glm::mat4 normalMatrix{1.0f};
            glm::vec3 color{1.0f}; 
            int isLight = false;
            int lightIndex = 0;
            float specularExponent = 0.0f;
            int texIndex = -1;
        };

        VulDevice &vulDevice;
        std::unique_ptr<VulPipeline> vulPipeline;
        VkPipelineLayout pipelineLayout;
};
}