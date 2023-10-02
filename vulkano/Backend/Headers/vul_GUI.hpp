#pragma once

#include"vul_renderer.hpp"
#include"vul_device.hpp"

namespace vulB{

class VulGUI{
    public:
        void initImGui(GLFWwindow *window, VkDescriptorPool &descriptorPool, VulRenderer &vulRenderer, VulDevice &vulDevice);

        void startFrame();
        void endFrame(VkCommandBuffer &commandBuffer);

        void destroyImGui();
};

}