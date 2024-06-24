#include <vul_debug_tools.hpp>
#include<vul_pipeline.hpp>

#include<fstream>
#include<stdexcept>
#include<iostream>
#include <vulkan/vulkan_core.h>

namespace vul{

VulPipeline::VulPipeline(const VulDevice& device, const std::string& vertFile, const std::string& fragFile, const PipelineConfigInfo& configInfo) : m_vulDevice(device)
{
    VkShaderModule vertShaderModule = createShaderModule(m_vulDevice, vertFile);
    VkShaderModule fragShaderModule = createShaderModule(m_vulDevice, fragFile);

    std::vector<VkPipelineShaderStageCreateInfo> shaderStages(2);
    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = vertShaderModule;
    shaderStages[0].pName = "main";
    shaderStages[0].flags = 0;
    shaderStages[0].pNext = nullptr;
    shaderStages[0].pSpecializationInfo = nullptr;

    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = fragShaderModule;
    shaderStages[1].pName = "main";
    shaderStages[1].flags = 0;
    shaderStages[1].pNext = nullptr;
    shaderStages[1].pSpecializationInfo = nullptr;

    PipelineContents pipelineContents = createPipelineContents(device, shaderStages, configInfo);
    m_pipeline = pipelineContents.pipeline;
    m_layout = pipelineContents.layout;

    vkDestroyShaderModule(m_vulDevice.device(), vertShaderModule, nullptr);
    vkDestroyShaderModule(m_vulDevice.device(), fragShaderModule, nullptr);

    VUL_NAME_VK(m_pipeline)
    VUL_NAME_VK(m_layout)
}

VulPipeline::~VulPipeline() {
    vkDestroyPipeline(m_vulDevice.device(), m_pipeline, nullptr);
    vkDestroyPipelineLayout(m_vulDevice.device(), m_layout, nullptr);
}

void VulPipeline::draw( VkCommandBuffer cmdBuf, const std::vector<VkDescriptorSet> &descriptorSets, const std::vector<VkBuffer> &vertexBuffers,
                        VkBuffer indexBuffer, const std::vector<DrawData> &drawDatas)
{
    VUL_PROFILE_FUNC()
    {
        VUL_PROFILE_SCOPE("Binding the pipeline, descriptor sets, vertex buffers and the index buffer")
        vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
        if (descriptorSets.size() > 0) vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_layout, 0, descriptorSets.size(), descriptorSets.data(), 0, nullptr);

        std::vector<VkDeviceSize> offsets(vertexBuffers.size());
        for (size_t i = 0; i < vertexBuffers.size(); i++) offsets.push_back(0);
        vkCmdBindVertexBuffers(cmdBuf, 0, static_cast<uint32_t>(vertexBuffers.size()), vertexBuffers.data(), offsets.data());
        vkCmdBindIndexBuffer(cmdBuf, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    }

    {
        VUL_PROFILE_SCOPE("Pushing constants and drawing indices")
        for (const DrawData &drawData : drawDatas){
            if (drawData.pushDataSize > 0) vkCmdPushConstants(cmdBuf, m_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, drawData.pushDataSize, drawData.pPushData.get());
            vkCmdDrawIndexed(cmdBuf, drawData.indexCount, drawData.instanceCount, drawData.firstIndex, drawData.vertexOffset, drawData.firstInstance);
        }
    }
}

VulPipeline::PipelineContents VulPipeline::createPipelineContents(const VulDevice &vulDevice,
        const std::vector<VkPipelineShaderStageCreateInfo> &shaderStageCreateInfos, const PipelineConfigInfo &configInfo)
{
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(configInfo.attributeDescriptions.size());
    vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(configInfo.bindingDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = configInfo.attributeDescriptions.data();
    vertexInputInfo.pVertexBindingDescriptions = configInfo.bindingDescriptions.data();

    VkPipelineRenderingCreateInfo pipelineRenderingInfo{};
    pipelineRenderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    pipelineRenderingInfo.colorAttachmentCount = static_cast<uint32_t>(configInfo.colorAttachmentFormats.size());
    pipelineRenderingInfo.pColorAttachmentFormats = configInfo.colorAttachmentFormats.data();
    pipelineRenderingInfo.depthAttachmentFormat = configInfo.depthAttachmentFormat;

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
    inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;
    
    VkPipelineViewportStateCreateInfo viewportInfo{};
    viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportInfo.viewportCount = 1;
    viewportInfo.pViewports = nullptr;
    viewportInfo.scissorCount = 1;
    viewportInfo.pScissors = nullptr;

    VkPipelineRasterizationStateCreateInfo rasterizationInfo{};
    rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationInfo.depthClampEnable = VK_FALSE;
    rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationInfo.lineWidth = 1.0f;
    rasterizationInfo.cullMode = configInfo.cullMode;
    rasterizationInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizationInfo.depthBiasEnable = VK_FALSE;
    rasterizationInfo.depthBiasConstantFactor = 0.0f;  // Optional
    rasterizationInfo.depthBiasClamp = 0.0f;           // Optional
    rasterizationInfo.depthBiasSlopeFactor = 0.0f;     // Optional

    VkPipelineMultisampleStateCreateInfo multisampleInfo{};
    multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleInfo.sampleShadingEnable = VK_FALSE;
    multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampleInfo.minSampleShading = 1.0f;           // Optional
    multisampleInfo.pSampleMask = nullptr;             // Optional
    multisampleInfo.alphaToCoverageEnable = VK_FALSE;  // Optional
    multisampleInfo.alphaToOneEnable = VK_FALSE;       // Optional
    
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
        VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = configInfo.enableColorBlending;
    colorBlendAttachment.srcColorBlendFactor = configInfo.blendSrcFactor;
    colorBlendAttachment.dstColorBlendFactor = configInfo.blendDstFactor;
    colorBlendAttachment.colorBlendOp = configInfo.blendOp;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;   // Optional
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;  // Optional
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;              // Optional
    
    std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments(configInfo.colorAttachmentFormats.size());
    for (size_t i = 0; i < configInfo.colorAttachmentFormats.size(); i++) colorBlendAttachments[i] = colorBlendAttachment;
    VkPipelineColorBlendStateCreateInfo colorBlendInfo{};
    colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendInfo.logicOpEnable = VK_FALSE;
    colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;  // Optional
    colorBlendInfo.attachmentCount = static_cast<uint32_t>(colorBlendAttachments.size());
    colorBlendInfo.pAttachments = colorBlendAttachments.data();
    colorBlendInfo.blendConstants[0] = 0.0f;  // Optional
    colorBlendInfo.blendConstants[1] = 0.0f;  // Optional
    colorBlendInfo.blendConstants[2] = 0.0f;  // Optional
    colorBlendInfo.blendConstants[3] = 0.0f;  // Optional
    
    VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};
    depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilInfo.depthTestEnable = VK_TRUE;
    depthStencilInfo.depthWriteEnable = VK_TRUE;
    depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
    depthStencilInfo.minDepthBounds = 0.0f;  // Optional
    depthStencilInfo.maxDepthBounds = 1.0f;  // Optional
    depthStencilInfo.stencilTestEnable = VK_FALSE;
    depthStencilInfo.front = {};  // Optional
    depthStencilInfo.back = {};   // Optional

    VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
    std::vector<VkDynamicState> dynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateInfo.pDynamicStates = dynamicStateEnables.data();
    dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
    dynamicStateInfo.flags = 0;

    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = vulDevice.properties.limits.maxPushConstantsSize; 

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(configInfo.setLayouts.size());
    pipelineLayoutInfo.pSetLayouts = configInfo.setLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    VkPipelineLayout layout;
    if (vkCreatePipelineLayout(vulDevice.device(), &pipelineLayoutInfo, nullptr, &layout) != VK_SUCCESS){
        throw std::runtime_error("Failed to create pipelineLayout in RenderSystem.cpp file");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = &pipelineRenderingInfo;
    pipelineInfo.stageCount = shaderStageCreateInfos.size();
    pipelineInfo.pStages = shaderStageCreateInfos.data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
    pipelineInfo.pViewportState = &viewportInfo;
    pipelineInfo.pRasterizationState = &rasterizationInfo;
    pipelineInfo.pMultisampleState = &multisampleInfo;
    pipelineInfo.pColorBlendState = &colorBlendInfo;
    pipelineInfo.pDepthStencilState = &depthStencilInfo;
    pipelineInfo.pDynamicState = &dynamicStateInfo;

    pipelineInfo.layout = layout;
    pipelineInfo.renderPass = nullptr;
    pipelineInfo.subpass = 0;

    pipelineInfo.basePipelineIndex = -1;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    VkPipeline pipeline;
    if (vkCreateGraphicsPipelines(vulDevice.device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS){
        throw std::runtime_error("Failed to create graphics pipeline in vul_pipeline.cpp file");
    }
    return {.pipeline = pipeline, .layout = layout};
}

VkShaderModule VulPipeline::createShaderModule(const VulDevice &m_vulDevice, const std::string &filePath)
{
    std::ifstream file{filePath, std::ios::ate | std::ios::binary};
    if (!file.is_open()) throw std::runtime_error("Failed to open file in vul_pipeline readFile function. Filepath: " + filePath);

    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> code(fileSize);
    file.seekg(0);
    file.read(code.data(), fileSize);
    file.close();

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(m_vulDevice.device(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS){
        throw std::runtime_error("Failed to create shader module in vul_pipeline.cpp file");
    }
    return shaderModule;
}

}
