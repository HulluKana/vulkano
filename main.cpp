#include"vulkano/vulkano_program.hpp"

#include<cstdlib>
#include<iostream>
#include<stdexcept>

int main()
{
    vul::Vulkano vulkano{};

    vulkano.loadObject("simpleDisplay");

    bool stop = false;
    while (!stop){
        VkCommandBuffer commandBuffer = vulkano.startFrame();
        stop = vulkano.endFrame(commandBuffer);
    }
}