#pragma once

#include"Backend/Headers/vul_device.hpp"
#include"Backend/Headers/vul_descriptors.hpp"
#include"Backend/Headers/vul_GUI.hpp"
#include"Backend/Headers/vul_simple_render_system.hpp"
#include"Backend/Headers/vul_movement_controller.hpp"

#include<memory>

namespace vul{

class Vulkano{
    public:
        static constexpr int WIDTH = 1000;
        static constexpr int HEIGHT = 800;

        Vulkano();
        ~Vulkano();

        /* These 2 lines remove the copy constructor and operator from Vulkano class.
        Because I'm using a pointer to stuff and that stuff is initialized by constructor and removed by destructor,
        copying the pointer and then removing the original pointer leaves me with pointer pointing to nothing, possibly leading to VERY nasty bugs */
        Vulkano(const Vulkano &) = delete;
        Vulkano &operator=(const Vulkano &) = delete;

        VkCommandBuffer startFrame();
        bool endFrame(VkCommandBuffer commandBuffer);
        
        void loadObject(std::string file);
        size_t getObjCount() {return m_objects.size();}
        vulB::VulObject *getObjectsPointer() {return m_objects.data();}
        
        vulB::VulDevice &getVulDevice() {return m_vulDevice;}
    private:
        double m_maxFps;
        double m_currentTime;

        vulB::VulWindow m_vulWindow{WIDTH, HEIGHT, "Vulkano"};
        vulB::VulDevice m_vulDevice{m_vulWindow};
        vulB::VulRenderer m_vulRenderer{m_vulWindow, m_vulDevice};
        vulB::VulGUI m_vulGUI;

        std::unique_ptr<vulB::VulDescriptorPool> m_globalPool{};
        std::vector<vulB::VulObject> m_objects;

        std::vector<std::unique_ptr<vulB::VulBuffer>> m_uboBuffers;
        std::vector<VkDescriptorSet> m_globalDescriptorSets;
        vulB::SimpleRenderSystem m_simpleRenderSystem{m_vulDevice};

        vulB::VulCamera m_camera{};
        vulB::VulObject m_cameraObject;
        vulB::MovementController m_cameraController;
};
}