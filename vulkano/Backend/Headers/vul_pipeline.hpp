#pragma once

#include"vul_device.hpp"
#include<string>
#include<vector>

namespace vulB{

struct PipelineConfigInfo {
    VkPipelineViewportStateCreateInfo viewportInfo;
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
    VkPipelineRasterizationStateCreateInfo rasterizationInfo;
    VkPipelineMultisampleStateCreateInfo multisampleInfo;
    VkPipelineColorBlendAttachmentState colorBlendAttachment;
    VkPipelineColorBlendStateCreateInfo colorBlendInfo;
    VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
    std::vector<VkDynamicState> dynamicStateEnables;
    VkPipelineDynamicStateCreateInfo dynamicStateInfo;
    VkPipelineLayout pipelineLayout = nullptr;
    VkRenderPass renderPass = nullptr;
    uint32_t subpass = 0;
};

class VulPipeline{
    public:
        VulPipeline(VulDevice& device, const std::string& vertFile, const std::string& fragFile, const PipelineConfigInfo& configInfo, VkFormat colorAttachmentFormat);
        ~VulPipeline();

        /* These 2 lines remove the copy constructor and operator from VulPipeline class.
        Because I'm using a pointer to some stuff and that stuff is initialized by constructor and removed by destructor,
        copying the pointer and then removing the original pointer leaves me with pointer pointing to nothing, possibly leading to VERY nasty bugs */
        VulPipeline(const VulPipeline &) = delete;
        VulPipeline &operator=(const VulPipeline &) = delete;

        void bind(VkCommandBuffer commandBuffer);

        static void defaultPipeLineConfigInfo(PipelineConfigInfo &configInfo);
    
    private:
        static std::vector<char> readFile(const std::string& filePath);

        void createGraphicsPipeline(const std::string& vertFile, const std::string& fragFile, const PipelineConfigInfo& configInfo, VkFormat colorAttachmentFormat);
        void createShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule);

        VulDevice& vulDevice;
        VkPipeline graphicsPipeline;
        VkShaderModule vertShaderModule;
        VkShaderModule fragShaderModule;
};
}