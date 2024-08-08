#pragma once

#include "vul_command_pool.hpp"
#include"vul_renderer.hpp"
#include"vul_device.hpp"

namespace vul{

class VulGUI{
    public:
        VulGUI(GLFWwindow *window, VkDescriptorPool &descriptorPool, VulRenderer &vulRenderer, VulDevice &vulDevice, VulCmdPool &cmdPool);
        ~VulGUI();

        void startFrame();
        void endFrame(VkCommandBuffer &commandBuffer);
};

}
