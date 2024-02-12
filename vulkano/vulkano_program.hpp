#pragma once

#include "Backend/Headers/vul_buffer.hpp"
#include"Backend/Headers/vul_device.hpp"
#include"Backend/Headers/vul_descriptors.hpp"
#include"Backend/Headers/vul_GUI.hpp"
#include"Backend/Headers/vul_render_system.hpp"
#include"Backend/Headers/vul_movement_controller.hpp"
#include"Backend/Headers/vul_image.hpp"
#include"Backend/Headers/vul_settings.hpp"

#include"Backend/Headers/vul_host_device.hpp"
#include"Backend/Headers/vul_2d_object.hpp"
#include "Backend/Headers/vul_swap_chain.hpp"
#include"Backend/Headers/vul_transform.hpp"

#include<memory>
#include<array>
#include<variant>
#include <vulkan/vulkan_core.h>

namespace vul{

class Vulkano{
    private:
        vulB::VulWindow m_vulWindow;
        vulB::VulDevice m_vulDevice{m_vulWindow}; // vulDevice has to be at the top to prevent some vulkan validation layer errors

    public:
        Vulkano(uint32_t width, uint32_t height, std::string name);
        ~Vulkano();

        void initVulkano();
        void letVulkanoFinish() {vkDeviceWaitIdle(m_vulDevice.device());}

        /* These 2 lines remove the copy constructor and operator from Vulkano class.
        Because I'm using a pointer to stuff and that stuff is initialized by constructor and removed by destructor,
        copying the pointer and then removing the original pointer leaves me with pointer pointing to nothing, possibly leading to VERY nasty bugs */
        Vulkano(const Vulkano &) = delete;
        Vulkano &operator=(const Vulkano &) = delete;

        VkCommandBuffer startFrame();
        bool endFrame(VkCommandBuffer commandBuffer);

        float getFrameTime() const {return m_frameTime;}
        float getIdleTime() const {return m_idleTime;}
        float getRenderPreparationTime() const {return m_renderPreparationTime;}
        float getObjRenderTime() const {return m_objRenderTime;}
        float getGuiRenderTime() const {return m_GuiRenderTime;}
        float getRenderFinishingTime() const {return m_renderFinishingTime;}

        bool shouldShowGUI() const {return !m_cameraController.hideGUI;}
        bool windowWasResized() {return m_vulWindow.wasWindowResized();}
        
        void createSquare(float x, float y, float width, float height);
        void createTriangle(glm::vec2 corner1, glm::vec2 corner2, glm::vec2 corner3);
        void loadScene(std::string fileName)
        {
            scene.loadScene(fileName);
            hasScene = true;
        }
        
        vulB::VulDevice &getVulDevice() {return m_vulDevice;}
        VkExtent2D getSwapChainExtent() const {return m_vulRenderer.getSwapChainExtent();}

        enum class DescriptorType{
            ubo,
            ssbo,
            combinedTexSampler,
            spCombinedTexSampler,
            storageImage
        };
        enum class ShaderStage{
            frag = VK_SHADER_STAGE_FRAGMENT_BIT,
            vert = VK_SHADER_STAGE_VERTEX_BIT,
            comp = VK_SHADER_STAGE_COMPUTE_BIT
        };
        struct Descriptor{
            DescriptorType type;
            std::vector<ShaderStage> stages;
            void *content;
            uint32_t count = 1;
        };
        struct descSetReturnVal{
            vulB::VulDescriptorSet set;
            std::unique_ptr<vulB::VulDescriptorSetLayout> layout;
            bool succeeded;
        };

        descSetReturnVal createDescriptorSet(const std::vector<Descriptor> &descriptors);
        std::unique_ptr<RenderSystem> createNewRenderSystem(const std::vector<VkDescriptorSetLayout> &setLayouts, std::string vertShaderName, std::string fragShaderName, bool is2D); 

        Scene scene{m_vulDevice};
        bool hasScene = false;

        std::unique_ptr<RenderSystem> renderSystem3D;
        std::unique_ptr<RenderSystem> renderSystem2D;
        std::vector<vulB::VulDescriptorSet> mainDescriptorSets;
        std::unique_ptr<vulB::VulDescriptorSetLayout> mainSetLayout;
        std::vector<std::unique_ptr<vulB::VulBuffer>> buffers;
        bool usesDefaultDescriptorSets = false;

        std::vector<Object2D> object2Ds;
        std::array<std::shared_ptr<VulImage>, MAX_TEXTURES> images;
        uint32_t imageCount = 0u;
    private:
        double m_currentTime;
        float m_frameTime;
        float m_idleTime;
        float m_renderPreparationTime;
        float m_objRenderTime;
        float m_GuiRenderTime;
        float m_renderFinishingTime;

        VkExtent2D m_prevWindowSize;

        vulB::VulRenderer m_vulRenderer{m_vulWindow, m_vulDevice};
        vulB::VulGUI m_vulGUI;

        std::unique_ptr<vulB::VulDescriptorPool> m_globalPool{};
        std::vector<vulB::VulDescriptorSet> m_imGuiDescriptorSets;

        vulB::VulCamera m_camera{};
        transform3D m_cameraTransform;
        vulB::MovementController m_cameraController;
   };
}
