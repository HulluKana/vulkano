#include"vulkano/vulkano_program.hpp"
#include"vulkano/vulkano_GUI_tools.hpp"
#include"vulkano/vulkano_random.hpp"

#include<imgui.h>

#include<iostream>

void GuiStuff(vul::Vulkano &vulkano, char *modelFileName, int modelFileNameLen, int &objIndex, size_t texCount, float ownStuffTime)
{
    std::unique_ptr<vul::VulImage> *images = vulkano.getImagesPointer();

    bool addObject = false;
    ImGui::Begin("ObjManager");
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
        if (obj[objIndex].hasTexture) ImGui::SliderInt("Tex index", reinterpret_cast<int *>(&obj[objIndex].textureIndex), 0, texCount - 1);
    }
    ImGui::End(); 

    ImGui::Begin("Camera controller");
    ImGui::Checkbox("Has perspective", &vul::settings::cameraProperties.hasPerspective);
    if (vul::settings::cameraProperties.hasPerspective) ImGui::DragFloat("FOV", &vul::settings::cameraProperties.fovY, 0.017f, -2.0f * M_PI, 2.0f * M_PI);
    ImGui::DragFloat("Near plane", &vul::settings::cameraProperties.nearPlane, 0.001f, 0.001f, 10.0f);
    ImGui::DragFloat("Far plane", &vul::settings::cameraProperties.farPlane, 1.0f, 1.0f, 100'000.0f);
    if (!vul::settings::cameraProperties.hasPerspective){
        ImGui::DragFloat("Left plane", &vul::settings::cameraProperties.leftPlane, 0.2f, -1'000.0f, 1'000.0f);
        ImGui::DragFloat("Right plane", &vul::settings::cameraProperties.rightPlane, 0.2f, -1'000.0f, 1'000.0f);
        ImGui::DragFloat("Top plane", &vul::settings::cameraProperties.topPlane, 0.2f, -1'000.0f, 1'000.0f);
        ImGui::DragFloat("Bottom plane", &vul::settings::cameraProperties.bottomPlane, 0.2f, -1'000.0f, 1'000.0f);
    }
    ImGui::End();

    ImGui::Begin("Image");
    ImGui::Image(images[1]->getImGuiTextureID(), ImVec2(images[1]->getWidth(), images[1]->getHeight()));
    ImGui::End();

    ImGui::Begin("Performance");
    ImGui::DragFloat("Max FPS", &vul::settings::maxFps, vul::settings::maxFps / 30.0f, 3.0f, 10'000.0f);
    ImGui::Text("Fps: %f\nTotal frame time: %fms\nObject render time: %fms\nGUI render time: %fms\nRender preparation time: %fms\nRender finishing time: %fms\nIdle time %fms\nOwn stuff: %fms",
                1.0f / vulkano.getFrameTime(), vulkano.getFrameTime() * 1000.0f, vulkano.getObjRenderTime() * 1000.0f, vulkano.getGuiRenderTime() * 1000.0f,
                vulkano.getRenderPreparationTime() * 1000.0f, vulkano.getRenderFinishingTime() * 1000.0f, vulkano.getIdleTime() * 1000.0f, ownStuffTime * 1000.0f);
    ImGui::End();

    if (addObject){
        std::string modelFileNameStr(modelFileName);
        vulkano.loadObject(modelFileNameStr);
    }
}

int main()
{
    std::string name("Vulkano");
    vul::Vulkano vulkano(1000, 800, name);

    std::unique_ptr<vul::VulImage> image1 = vul::VulImage::createAsUniquePtr(vulkano.getVulDevice());
    std::unique_ptr<vul::VulImage> image2 = vul::VulImage::createAsUniquePtr(vulkano.getVulDevice());
    image1->createTextureFromFile("texture.jpg");
    image2->createTextureFromFile("kana.jpg");
    image2->usableByImGui = true;

    std::vector<std::unique_ptr<vul::VulImage>> vulImages;
    vulImages.push_back(std::move(image1));
    vulImages.push_back(std::move(image2));

    vulkano.addImages(vulImages);
    vulkano.initVulkano();

    bool stop = false;
    int objIndex = 0;

    int modelFileNameLen = 25;
    char *modelFileName = new char[modelFileNameLen];
    for (int i = 0; i < modelFileNameLen - 1; i++){
        modelFileName[i] = '0';
    }
    modelFileName[modelFileNameLen - 2] = '\0';

    float ownStuffTime = 0.0f;
    while (!stop){

        VkCommandBuffer commandBuffer = vulkano.startFrame();
        double ownStuffStartTime = glfwGetTime();

        if (vulkano.shouldShowGUI()) GuiStuff(vulkano, modelFileName, modelFileNameLen, objIndex, vulImages.size(), ownStuffTime);
        
        ownStuffTime = glfwGetTime() - ownStuffStartTime;
        stop = vulkano.endFrame(commandBuffer);
    }

    return 0;
}