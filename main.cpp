#include"vulkano/vulkano_program.hpp"
#include"vulkano/vulkano_GUI_tools.hpp"

#include<imgui.h>

#include<iostream>

int main()
{
    vul::Vulkano vulkano{};

    bool stop = false;
    bool addObject = false;
    int objIndex = 0;
    while (!stop){
        VkCommandBuffer commandBuffer = vulkano.startFrame();
        
        ImGui::Begin("Test");
        ImGui::Checkbox("Add object", &addObject);
        if (vulkano.getObjCount() > 0){
            ImGui::SliderInt("Obj Index", &objIndex, 0, (int)vulkano.getObjCount() - 1);

            auto obj = vulkano.getObjectsPointer();
            vul::GUI::DragFloat3("Obj pos", obj[objIndex].transform.posOffset);
        }
        ImGui::End(); 

        if (addObject){
            vulkano.loadObject("cube");
            addObject = false;
        }

        stop = vulkano.endFrame(commandBuffer);
    }
}