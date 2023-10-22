#pragma once

#include"Backend/Headers/vul_device.hpp"
#include"Backend/Headers/vul_descriptors.hpp"
#include"Backend/Headers/vul_GUI.hpp"
#include"Backend/Headers/vul_simple_render_system.hpp"
#include"Backend/Headers/vul_movement_controller.hpp"
#include"Backend/Headers/vul_image.hpp"
#include"Backend/Headers/vul_settings.hpp"

#include<memory>

namespace vul{

class Vulkano{
    public:
        Vulkano(uint32_t width, uint32_t height, std::string &name);
        ~Vulkano();

        void addImages(std::vector<std::unique_ptr<VulImage>> &vulImages);
        void initVulkano();

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
        
        void loadObject(std::string file);
        size_t getObjCount() {return m_objects.size();}
        VulObject *getObjectsPointer() {return m_objects.data();}

        std::unique_ptr<VulImage> *getImagesPointer() {return m_images.data();}
        
        vulB::VulDevice &getVulDevice() {return m_vulDevice;}
    private:
        double m_currentTime;
        float m_frameTime;
        float m_idleTime;
        float m_renderPreparationTime;
        float m_objRenderTime;
        float m_GuiRenderTime;
        float m_renderFinishingTime;

        vulB::VulWindow m_vulWindow;
        vulB::VulDevice m_vulDevice{m_vulWindow};
        vulB::VulRenderer m_vulRenderer{m_vulWindow, m_vulDevice};
        vulB::VulGUI m_vulGUI;

        std::unique_ptr<vulB::VulDescriptorPool> m_globalPool{};
        std::vector<VulObject> m_objects;
        std::vector<std::unique_ptr<VulImage>> m_images;

        std::vector<std::unique_ptr<vulB::VulBuffer>> m_uboBuffers;
        std::vector<VkDescriptorSet> m_globalDescriptorSets;
        vulB::SimpleRenderSystem m_simpleRenderSystem{m_vulDevice};

        vulB::VulCamera m_camera{};
        VulObject m_cameraObject;
        vulB::MovementController m_cameraController;
};
}