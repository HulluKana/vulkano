#pragma once

#include<vulkano_program.hpp>
#include <memory>

namespace vul
{
namespace defaults
{
    struct Default3dInputData{
        std::shared_ptr<VulImage> multipleBounce2dImage = nullptr;
        std::shared_ptr<VulImage> multipleBounce1dImage = nullptr;
    };
    Default3dInputData createDefault3dInputData(Vulkano &vulkano);

    struct DefaultRenderDataInputData {
        size_t renderDataIdx;
        size_t descriptorSetLayoutIdx;
    };
    DefaultRenderDataInputData createDefaultDescriptors(Vulkano &vulkano, Default3dInputData inputData);
    void createDefault3dRenderSystem(Vulkano &vulkano, DefaultRenderDataInputData inputData);
    void updateDefault3dInputValues(Vulkano &vulkano, DefaultRenderDataInputData inputData);
}
}
