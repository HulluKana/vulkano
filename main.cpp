#include"vulkano/vulkano_program.hpp"
#include"vulkano/vulkano_GUI_tools.hpp"
#include"vulkano/vulkano_random.hpp"

#include<imgui.h>

#include<iostream>

int main()
{
    vul::Vulkano vulkano{};

    uint32_t width = 400;
    uint32_t height = 400;
    uint8_t* data = new uint8_t[width * height * 4];
    std::unique_ptr<vul::VulImage> vulImage = vul::VulImage::createAsUniquePtr(vulkano.getVulDevice());
    std::unique_ptr<vul::VulImage> vulImage2 = vul::VulImage::createAsUniquePtr(vulkano.getVulDevice());
    vulImage->createTextureFromData(data, width, height, true);
    vulImage2->createTextureFromData(data, width, height, true);
    vulImage->usableByImGui = true;
    vulImage2->usableByImGui = true;

    std::vector<std::unique_ptr<vul::VulImage>> vulImages;
    vulImages.push_back(std::move(vulImage));
    vulImages.push_back(std::move(vulImage2));

    vulkano.initVulkano(vulImages);

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
    uint32_t frame = 0;
    while (!stop){
        if (frame > 255) frame = 0;
        for (uint32_t i = 0; i < height; i++){
            for (uint32_t j = 0; j < width; j++){
                for (uint32_t k = 0; k < 4; k++){
                    uint32_t index = (i * width + j) * 4 + k;
                    data[index] = ((j + frame) * (k + 1)) % 256;
                }
            }
        }
        auto images = vulkano.getImagesPointer();
        images[0]->modifyTextureImage(data);
        for (uint32_t i = 0; i < height; i++){
            for (uint32_t j = 0; j < width; j++){
                for (uint32_t k = 0; k < 4; k++){
                    uint32_t index = (i * width + j) * 4 + k;
                    data[index] = ((j + frame * 3) * (k + 1)) % 256;
                }
            }
        }
        images[1]->modifyTextureImage(data);
        frame++;
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

            ImGui::Checkbox("Obj has texture", &obj[objIndex].hasTexture);
            if (obj[objIndex].hasTexture) ImGui::SliderInt("Texture index", (int *)&obj[objIndex].textureIndex, 0, vulImages.size() - 1);
        }
        ImGui::End(); 

        ImGui::Begin("Image");
        ImGui::Image(images[0]->getImGuiTextureID(), ImVec2(images[0]->getWidth(), images[0]->getHeight()));
        ImGui::Image(images[1]->getImGuiTextureID(), ImVec2(images[1]->getWidth(), images[1]->getHeight()));
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