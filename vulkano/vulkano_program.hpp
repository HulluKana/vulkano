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
    private:
        double m_maxFps;
        double m_currentTime;

        VulWindow m_vulWindow{WIDTH, HEIGHT, "Vulkano"};
        VulDevice m_vulDevice{m_vulWindow};
        VulRenderer m_vulRenderer{m_vulWindow, m_vulDevice};
        VulGUI m_vulGUI;

        // Please note: ORDER OF DECLARATION MATTERS
        std::unique_ptr<VulDescriptorPool> m_globalPool{};
        std::vector<VulObject::id_t> m_objectIDs;
        VulObject::Map m_objects;

        std::vector<std::unique_ptr<VulBuffer>> m_uboBuffers;
        std::vector<VkDescriptorSet> m_globalDescriptorSets;
        SimpleRenderSystem m_simpleRenderSystem{m_vulDevice};

        VulCamera m_camera{};
        VulObject m_cameraObject = VulObject::createObject();
        MovementController m_cameraController;
};
}