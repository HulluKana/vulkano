#include"vulkano/vulkano_program.hpp"
#include"vulkano/vulkano_GUI_tools.hpp"
#include"vulkano/vulkano_random.hpp"

#include<imgui.h>

#include<iostream>

int main()
{
    std::string name("Vulkano");
    vul::Vulkano vulkano(1000, 800, name);

    vulkano.initVulkano();

    bool stop = false;
    bool addObject = false;
    int objIndex = 0;

    int modelFileNameLen = 25;
    char *modelFileName = new char[modelFileNameLen];
    for (int i = 0; i < modelFileNameLen - 1; i++){
        modelFileName[i] = '0';
    }
    modelFileName[modelFileNameLen - 2] = '\0';

    vulkano.setMaxFps(5'000.0f);
    while (!stop){
        VkCommandBuffer commandBuffer = vulkano.startFrame();
        
        ImGui::Begin("Test");
        ImGui::Text((std::string("Fps: ") + std::to_string(1.0f / vulkano.getFrameTime())).c_str());
        ImGui::Checkbox("Add object", &addObject);
        ImGui::InputText("File name", modelFileName, modelFileNameLen);
        if (vulkano.getObjCount() > 0){
            ImGui::SliderInt("Obj Index", &objIndex, 0, (int)vulkano.getObjCount() - 1);

            auto obj = vulkano.getObjectsPointer();
            vul::GUI::DragFloat3("Obj pos", obj[objIndex].transform.posOffset, 0.0f, 0.0f, 0.1f);
            vul::GUI::DragFloat3("Obj rotation", obj[objIndex].transform.rotation, 2 * M_PI, -2 * M_PI, 0.017f);
            vul::GUI::DragFloat3("Obj scale", obj[objIndex].transform.scale, 10'000.0f, -10'000.0f, 0.1f);
            vul::GUI::DragFloat3("Obj color", obj[objIndex].color, 1.0f, 0.0f, 0.005f);
            ImGui::DragFloat("Obj specular exponent", &obj[objIndex].specularExponent, (sqrt(obj[objIndex].specularExponent)) / 10.0f, 1.0f, 1'000.0f);
            ImGui::Checkbox("Obj is light", &obj[objIndex].isLight);
            if (obj[objIndex].isLight){
                vul::GUI::DragFloat3("Light color", obj[objIndex].lightColor, 1.0f, 0.0f, 0.005f);
                ImGui::DragFloat("Light intensity", &obj[objIndex].lightIntensity, (sqrt(obj[objIndex].lightIntensity) + 0.1f) / 10.0f, 0.0f, 1'000'000.0f);
            }
        }
        ImGui::End(); 

        if (addObject){
            std::string modelFileNameStr(modelFileName);
            vulkano.loadObject(modelFileNameStr);
            addObject = false;
        }

        stop = vulkano.endFrame(commandBuffer);
    }

    return 0;
}