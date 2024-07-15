#pragma once

#include "vul_renderer.hpp"
#include<vul_buffer.hpp>
#include<vul_device.hpp>
#include<vul_descriptors.hpp>
#include<vul_GUI.hpp>
#include<vul_movement_controller.hpp>
#include<vul_image.hpp>
#include<vul_settings.hpp>
#include<vul_2d_object.hpp>
#include<vul_swap_chain.hpp>
#include<vul_transform.hpp>
#include<vul_comp_pipeline.hpp>
#include<vul_scene.hpp>
#include<vul_camera.hpp>
#include<vul_pipeline.hpp>

#include<memory>
#include<array>
#include <vulkan/vulkan_core.h>

namespace vul{

class Vulkano{
    private:
        vul::VulWindow m_vulWindow;
        vul::VulDevice m_vulDevice{m_vulWindow}; // vulDevice has to be at the top to prevent some vulkan validation layer errors

    public:
        Vulkano(uint32_t width, uint32_t height, std::string name);
        ~Vulkano();

        void letVulkanoFinish() {vkDeviceWaitIdle(m_vulDevice.device());}

        /* These 2 lines remove the copy constructor and operator from Vulkano class.
        Because I'm using a pointer to stuff and that stuff is initialized by constructor and removed by destructor,
        copying the pointer and then removing the original pointer leaves me with pointer pointing to nothing, possibly leading to VERY nasty bugs */
        Vulkano(const Vulkano &) = delete;
        Vulkano &operator=(const Vulkano &) = delete;

        VkCommandBuffer startFrame();
        bool endFrame(VkCommandBuffer commandBuffer);

        double getFrameTime() const {return m_frameTime;}
        double getIdleTime() const {return m_idleTime;}

        int getFrameIdx() const {return vulRenderer.getFrameIndex();}

        bool shouldShowGUI() const {return !cameraController.hideGUI;}
        bool windowWasResized() {return m_vulWindow.wasWindowResized();}
        bool shouldClose() {return m_vulWindow.shouldClose();}
        
        void createSquare(float x, float y, float width, float height);
        void createTriangle(glm::vec2 corner1, glm::vec2 corner2, glm::vec2 corner3);
        void loadScene(const std::string &fileName, std::string textureDirectory, Scene::WantedBuffers wantedBuffers)
        {
            scene.loadScene(fileName, textureDirectory, wantedBuffers);
            hasScene = true;
        }
        
        const vul::VulDevice &getVulDevice() const {return m_vulDevice;}
        VkExtent2D getSwapChainExtent() const {return vulRenderer.getSwapChainExtent();}

        enum class DescriptorType{
            ubo,
            ssbo,
            combinedImgSampler,
            spCombinedImgSampler,
            upCombinedImgSampler,
            rawImageInfo,
            storageImage,
            accelerationStructure 
        };
        enum class ShaderStage{
            frag = VK_SHADER_STAGE_FRAGMENT_BIT,
            vert = VK_SHADER_STAGE_VERTEX_BIT,
            comp = VK_SHADER_STAGE_COMPUTE_BIT,
            rgen = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
            rmiss = VK_SHADER_STAGE_MISS_BIT_KHR,
            rchit = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
            rahit = VK_SHADER_STAGE_ANY_HIT_BIT_KHR,
            rint = VK_SHADER_STAGE_INTERSECTION_BIT_KHR,
            mesh = VK_SHADER_STAGE_MESH_BIT_EXT,
            task = VK_SHADER_STAGE_TASK_BIT_EXT
        };
        struct RawImageDescriptorInfo {
            VkDescriptorType descriptorType;
            VkDescriptorImageInfo descriptorInfo;
        };
        struct Descriptor{
            DescriptorType type;
            std::vector<ShaderStage> stages;
            const void *content;
            uint32_t count = 1;
        };

        std::unique_ptr<vul::VulDescriptorSet> createDescriptorSet(const std::vector<Descriptor> &descriptors) const;
        VulCompPipeline createNewComputePipeline(const std::vector<VkDescriptorSetLayout> &setLayouts, const std::string &compShaderName, uint32_t maxSubmitsInFlight);        

        struct RenderData {
            bool enable = true;
            bool is3d = true;
            std::shared_ptr<vul::VulPipeline> pipeline;
            std::vector<vul::VulPipeline::DrawData> drawDatas;
            std::vector<std::shared_ptr<vul::VulBuffer>> additionalVertexBuffers;
            std::array<std::vector<std::shared_ptr<vul::VulDescriptorSet>>, vul::VulSwapChain::MAX_FRAMES_IN_FLIGHT> descriptorSets;
            std::array<std::vector<std::shared_ptr<VulImage>>, vul::VulSwapChain::MAX_FRAMES_IN_FLIGHT> attachmentImages;
            vul::VulRenderer::SwapChainImageMode swapChainImageMode = vul::VulRenderer::SwapChainImageMode::clearPreviousStoreCurrent;
            vul::VulRenderer::DepthImageMode depthImageMode = vul::VulRenderer::DepthImageMode::clearPreviousDiscardCurrent;
            glm::vec4 swapChainClearColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
            float depthClearColor = 1.0f;
            bool sampleFromDepth = false;
        };
        std::vector<RenderData> renderDatas;

        Scene scene{m_vulDevice};
        bool hasScene = false;

        std::vector<std::unique_ptr<vul::VulBuffer>> buffers;
        std::vector<Object2D> object2Ds;

        vul::VulRenderer vulRenderer{m_vulWindow, m_vulDevice};
        vul::VulCamera camera{};
        transform3D cameraTransform;
        vul::MovementController cameraController;
    private:
        double m_currentTime;
        double m_frameTime;
        double m_idleTime;

        VkExtent2D m_prevWindowSize;
        vul::VulGUI m_vulGUI;
        std::unique_ptr<vul::VulDescriptorPool> m_globalPool{};
   };
}
